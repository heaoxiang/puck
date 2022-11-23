/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    index.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-10-25 11:49
 * @brief
 ***********************************************************************/
#ifndef BAIDU_MMS_GRAPH_GNOIMI_INDEX_H
#define BAIDU_MMS_GRAPH_GNOIMI_INDEX_H
namespace puck {
class Index {
public:
    /*
    * @brief 根据配置文件修改conf、初始化内存、加载索引文件
    * @@return 0 => 正常 非0 => 错误
    **/
    virtual int init() = 0;

    /*
     * @brief 检索最近的topk个样本
     * @@param [in] query_fea : query的特征向量
     * @@param [in] topk : 检索topk个最近的样本
     * @@param [out] distance : 返回样本与query的距离
     * @@param [out] local_idx :  返回样本的local idx
     * @@return 0 => 正常 非0 => 错误
     **/
    virtual int search(const float* feature, const int topk, float* distance, uint32_t* local_idx) = 0;

    /*
    * @brief 训练
    * @@return 0 => 正常 非0 => 错误
    **/
    virtual int train() = 0;
    /*
     * @brief 建库
     * @@return 0 => 正常 非0 => 错误
     **/
    virtual int build() = 0;
};
}//namespace puck
#endif

