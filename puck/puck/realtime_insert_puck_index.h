/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    realtime_insert_puck.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-12-30 16:06
 * @brief
 ***********************************************************************/
#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include "puck/puck/puck_index.h"
namespace puck {
struct InsertFineCluster;
struct IndexFileHandle;
struct InsertRequest;
struct InsertDataMemory;

class RealtimeInsertPuckIndex: public PuckIndex {
public:
    RealtimeInsertPuckIndex();
    ~RealtimeInsertPuckIndex();
    /*
     * @brief 根据配置文件修改conf、初始化内存、加载索引文件
     **/
    virtual int init();
    /*
     * @brief insert
     * @@param [in] request : query的特征向量 + label
     * @@return 0 => 正常 非0 => 错误
     **/
    int insert(InsertRequest* request);


    /*
     * @brief 检索最近的topk个样本
     * @@param [in] request : 请求数据
     * @@param [in] response : 返回结果
     * @@return 0 => 正常 非0 => 错误
     **/
    int search(const Request* request, Response* response);
    const IndexConf get_conf() {
        return _conf;
    }
    //实时入库需要提供每个样本的lable
    std::string get_label(uint32_t local_id);
protected:

    /*
     * @brief insert会导致文件长度变化，重新整理索引文件
     **/
    int reorganize_inserted_index();

    /*
     * @brief 处理单个索引文件
     **/
    int reorganize(const std::string& filename, uint64_t file_length);
    int append_insert_memory(uint32_t group_id);

    /*
     * @brief insert的样本写索引文件
     **/
    int append_index(InsertRequest* request, puck::BuildInfo& build_info, uint32_t insert_id);
protected:
    int read_labels();
    /*
    * @brief 计算query与top_coarse_cnt个一级聚类中心的下所有二级聚类中心的距离
    * @@param [in\out] context : context由内存池管理
    * @@param [in] feature : query的特征向量
    * @@return (int) : 正常返回保留的cell个数(>0)，错误返回值<0
    **/
    int search_nearest_fine_cluster(SearchContext* context, const float* feature);

protected:
    /*
     * @brief 计算query与某个cell下所有样本的距离（样本的filter量化特征）
     * @@param [in\out] context : context由内存池管理
     * @@param [in] cell_idx : 某个cell的id
     * @@param [in] pq_dist_table : pq_dist_table
     * @@param [in] result_heap : 堆结构，存储query与样本的topk
     * @@return 0 => 正常 非0 => 错误
     **/
    virtual int compute_quantized_distance(SearchContext* context, const int cell_idx,
                                           const float* pq_dist_table, MaxHeap& result_heap);
    virtual int rank_topN_docs(SearchContext* context, const float* feature, const uint32_t filter_topk,
                               MaxHeap& result_heap);
protected:
    //Insert points of each cell
    std::unique_ptr<InsertFineCluster> _insert_fine_cluster;
    //样本的Insert的顺序，递增
    std::atomic<size_t> _insert_id;
    //insert的数据分组存储，每个分组最大存储数量
    const uint32_t _max_insert_point = 50000000;
    //负责更新索引文件
    std::unique_ptr<IndexFileHandle> _index_file_handle;
    //insert成功的数据在内存的存储格式
    std::vector<InsertDataMemory*> _insert_data_memorys;
    //申请内存需要加锁
    std::mutex _update_memory_mutex;
    //存量数据的labels
    std::vector<std::string> _labels;
    //insert数据的labels
    std::vector<std::vector<std::string>*> _insert_labels;
};//class RealtimeInsertPuckIndex

struct InsertDataMemory {
    //filter量化特征、PQ量化特征(whether_pq=1)
    std::vector<Quantization*> quantizations;
    //原始特征(whether_pq=0)
    std::vector<float> init_feature;
    //insert的顺序与local idx的映射关系
    std::vector<uint32_t> memory_to_local;
};

struct InsertPoint {
    uint32_t insert_id;
    InsertPoint* next;
    InsertPoint(uint32_t id) {
        insert_id = id;
        next = nullptr;
    }
};

struct InsertFineCluster {
    uint32_t point_cnt;
    InsertPoint* insert_points;
    std::mutex update_mutex;
    InsertFineCluster() {
        point_cnt = 0;
        insert_points = nullptr;
    }
    uint32_t get_point_cnt() const {
        //LOG(INFO) << "get_point_cnt() " << point_cnt;
        return point_cnt;
    }
    ~InsertFineCluster();
};

struct InsertRequest {
    std::string label;               // label 必须
    const float* feature;            //query feature
    uint32_t dim;
    InsertRequest() : feature(nullptr) {
    }
};

//Insert写索引文件用到的所有文件句柄
struct IndexFileHandle {
    IndexFileHandle(IndexConf& index_conf): conf(index_conf) {}
    ~IndexFileHandle();
    int init();

    bool open_handle(std::ofstream& file_stream, std::string& file_name);
    bool close_handle(std::ofstream& file_stream);
    IndexConf& conf;

    std::ofstream cell_assign_file;
    std::ofstream label_file;
    std::ofstream pq_quantization_file;
    std::ofstream filter_quantization_file;
    std::ofstream all_feature_file;
    std::mutex update_file_mutex;
};

}//namespace

