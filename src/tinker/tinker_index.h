/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    tinker_index.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-05-16 10:34
 * @brief
 ***********************************************************************/
#pragma once

#include <vector>
#include "hierarchical_cluster/hierarchical_cluster.h"
#include "tinker/method/hnsw.h"
#include "tinker/space/space_lp.h"

namespace puck {

//内存索引结构
class TinkerIndex : public puck::HierarchicalCluster {
public:
    /*
     * @brief 默认构造函数，检索配置根据gflag参数确定(推荐使用)
     **/
    TinkerIndex();
    ~TinkerIndex() {}
    
    /*
     * @brief 检索最近的topk个样本
     * @@param [in] request : request
     * @@param [out] response : response
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int search(Request* request, Response* response);

    /*
    * @brief 读取索引配置文件（index.dat）、初始化内存、建库（计算样本最近的1个聚类中心）、写索引文件
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int build();
protected:
    /*
    * @brief 计算query与一级聚类中心的距离并排序，返回top-1
    * @@param [in\out] context : context由内存池管理
    * @@param [in] feature : query的特征向量
    * @@return (int) : top1个cell的id
    **/
    int search_top1_fine_cluster(puck::SearchContext* context, const float* feature);
    /*
     * @brief 计算query与一级聚类中心的距离并排序
     * @@param [in\out] context : context由内存池管理
     * @@param [in] feature : query的特征向量
     * @@param [out] distance : topk个样本的距离
     * @@param [out] labels : topk个样本的idx
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int tinker_topN_docs(puck::SearchContext* context, const float* feature, float* distance,
                         uint32_t* labels_idx);
private:
    /*
    * @brief 加载与样本相关的索引文件
    * @@param [in] local_to_memory_idx : 每个样本local_idx 与 memory_idx的映射关系
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int read_feature_index(uint32_t* local_to_memory_idx = nullptr);

    DISALLOW_COPY_AND_ASSIGN(TinkerIndex);
    //Tinker _tinker;
    std::unique_ptr<similarity::AnyParams> _any_params;
    std::unique_ptr<similarity::Hnsw<float>> _tinker_index;
    std::unique_ptr<similarity::SpaceLp<float>> _space;
};


}//tinker


