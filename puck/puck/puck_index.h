/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    puck_index.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-27 15:56
 * @brief
 ***********************************************************************/
#pragma once
#include <vector>
#include <string>
#include <memory>

#include <mutex>
#include <fstream>
#include "puck/puck/quantization.h"
#include "puck/hierarchical_cluster/hierarchical_cluster_index.h"
#include <stdlib.h>
//#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
namespace puck {


//内存索引结构
class PuckIndex : public puck::HierarchicalClusterIndex {
public:
    /*
     * @brief 默认构造函数，检索配置根据gflag参数确定(推荐使用)
     **/
    PuckIndex();
    virtual ~PuckIndex();

    /*
     * @brief 检索最近的topk个样本
     * @@param [in] request : request
     * @@param [out] response : response
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int search(const Request* request, Response* response);

    /*
    * @brief 初始化内存、训练码本（计算一二级聚类中心）、写码本文件
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int train();
    /*
     * @brief 读取索引配置文件（index.dat）、初始化内存、加载码本；第一次调研single_build时，用来初始化
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int init_single_build();
    /*
    * @brief 读取索引配置文件（index.dat）、初始化内存、1个样本建库（MR建库和实时入库时会调用该函数）
    * @@param [in/out] build_info : build_info包含样本建库所需的所有信息
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int single_build(BuildInfo* build_info);

protected:
    /*
     * @brief 写码本文件
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int save_coodbooks() const;
    /*
    * @brief 读码本文件
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int read_coodbooks();
    /*
    * @brief 写索引文件(建库的产出，与建库样本相关)
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int save_index();
    /*
     * @brief 初始化内存
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int init_model_memory();

    /*
     * @brief 建库的计算过程
     * @@param [int] total_cnt : 建库样本总数
     * @@param [int] feature_file_name : 建库样本的文件名
     * @@param [out] cell_assign : 存储每个样本最近的cell id
     **/
    virtual void batch_assign(const uint32_t total_cnt, const std::string& feature_file_name,
                              uint32_t* cell_assign);
    /*
     * @brief 计算query与某个cell下所有样本的距离（样本的filter量化特征）
     * @@param [in\out] context : context由内存池管理
     * @@param [in] cell_idx : 某个cell的id
     * @@param [in] pq_dist_table : pq_dist_table
     * @@param [in] result_heap : 堆结构，存储query与样本的topk
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int compute_quantized_distance(SearchContext* context, const int cell_idx,
                                           const float* pq_dist_table, MaxHeap& result_heap);
    /*
     * @brief 计算query与top-N个cell下所有样本的距离（样本的原始特征）
     * @@param [in\out] context : context由内存池管理
     * @@param [in] feature : query的特征向量
     * @@param [in] search_cell_cnt : 需要计算的cell的个数
     * @@param [in] result_heap : 堆结构，存储query与样本的topk
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int filter_topN_docs(SearchContext* context, const float* feature, const int search_cell_cnt,
                         MaxHeap& result_heap);
    virtual int rank_topN_docs(SearchContext* context, const float* feature, const uint32_t filter_topk, MaxHeap& result_heap);
    /*
    * @brief 检索过程中会按某种规则调整样本在内存的顺序（memory_idx），计算对应的信息
    * @@param [out] cell_start_memory_idx : 每个cell下样本中最小的memory_idx
    * @@param [out] local_to_memory_idx : 每个样本local_idx 与 memory_idx的映射关系
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int convert_local_to_memory_idx(uint32_t* cell_start_memory_idx, uint32_t* local_to_memory_idx);

    /*
    * @brief 加载与样本相关的索引文件
    * @@param [in] local_to_memory_idx : 每个样本local_idx 与 memory_idx的映射关系
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int read_feature_index(uint32_t* local_to_memory_idx = nullptr);
    /*
    * @brief 计算部分样本的量化特征
    * @@param [in] thread_params : 线程信息
    * @@param [out] cell_assign : 存储样本最近的cell id
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int puck_assign(const ThreadParams& thread_params, uint32_t* cell_assign) const;
    /*
    * @brief 计算1个样本的量化特征
    * @@param [in/out] build_info : build_info包含样本建库所需的所有信息
    * @@param [in] quantizations : 需要计算的量化数组
    * @@param [in] idx : 在quantizations中样本的idx
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int puck_single_assign(BuildInfo* build_info, std::vector<Quantization*>& quantizations, uint32_t idx);

protected:
    DISALLOW_COPY_AND_ASSIGN(PuckIndex);
    //通常取1/4量化，对过滤出的小规模样本重新排序时使用
    std::unique_ptr<Quantization> _pq_quantization;
    //大尺度量化，用来过滤
    std::unique_ptr<Quantization> _filter_quantization;
};

struct PuckBuildInfo : public BuildInfo {
    typedef std::pair<float, std::vector<unsigned char>> QuantizatedFeature;
    std::vector<QuantizatedFeature> quantizated_feature;
};
} //namesapce puck


