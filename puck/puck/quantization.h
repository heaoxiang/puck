/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    quantization.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2021-07-20 11:17
 * @brief
 ***********************************************************************/
#pragma once
#include <string>
#include <glog/logging.h>
#include <memory>
#include <functional>
#include <numeric>
#include <math.h>
#include <vector>
#include "index_conf.h"

namespace puck {

struct QuantizationParams {
    uint32_t ks;
    uint32_t dim;
    uint32_t nsq;
    uint32_t lsq;
    int init(const IndexConf& conf, bool is_filter = false) {
        if (conf.ks != 256 || conf.feature_dim < conf.nsq) {
            return -1;
        }

        ks = conf.ks;
        dim = conf.feature_dim;
        nsq = (is_filter == false) ? conf.nsq : conf.filter_nsq;
        lsq = std::ceil(1.0 * dim / nsq);
        LOG(INFO) << "QuantizationParams.dim = " << dim << ", QuantizationParams.ks = " << ks <<
                    ", QuantizationParams.lsq = " << lsq << ", QuantizationParams.nsq = " << nsq;
        return 0;
    }
};

///仅支持KS=256 && 分层索引量化后的残差
class Quantization {
public:
    Quantization(const QuantizationParams& params, uint32_t point_count);
    ~Quantization() {}
    /*
    * @brief 初始化量化特征所需的内存
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int init_quantized_feature_memory();
    /*
    * @brief 计算pq table
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int get_dist_table(const float*  query, float* dist_table) const;
    /*
    * @brief 获取量化码本
    * @@return float* : 码本指针
    **/
    inline float* get_coodbooks() const {
        return _coodbooks.get();
    }
    /*
    * @brief 获取指定空间的量化码本
    * @@return float* : 子空间码本指针
    **/
    inline float* get_sub_coodbooks(uint64_t n = 0, uint64_t k = 0) const {
        return _coodbooks.get() + (u_int64_t)n * _params.ks * _params.lsq + k * _params.lsq;
    }
    /*
    * @brief 获取某个样本的量化特征（偏移值+PQ量化特征）
    * @@return float* : 量化特征
    **/
    inline unsigned char* get_quantized_feature(uint64_t idx = 0) const {
        return _quantized_feature.get() + (u_int64_t)idx * _per_fea_len;
    }
    /*
    * @brief 获取PQ量化特征的偏移量
    * @@return int : PQ量化特征的偏移量
    **/
    int get_fea_offset() const {
        return _fea_offset;
    }
    /*
    * @brief 获取某个样本的量化特征的总长度（偏移值+PQ量化特征）
    * @@return int : 每个样本量化特征的长度
    **/
    int get_per_fea_len() const {
        return _per_fea_len;
    }

    /*
    * @brief 更新某个样本的偏移值
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int set_static_value_of_formula(uint64_t idx, float*);
    /*
    * @brief 加载量化索引
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int load(const std::string& codebook_file, const std::string& quantized_feafile,
             const uint32_t* local_2memory_idx = nullptr);
    /*
    * @brief 加载量化码本
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int load_codebooks(const std::string& codebook_file);
    /*
    * @brief 获取配置信息
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    const QuantizationParams& get_quantization_params() const {
        return _params;
    }
private:
    /*
    * @brief 计算码本长度
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    size_t get_coodbook_length() {
        LOG(INFO) << "get_coodbook_length = " << _params.nsq* _params.ks* _params.lsq* sizeof(float);
        return _params.nsq * _params.ks * _params.lsq * sizeof(float);
    }
    /*
    * @brief 计算量化特征长度
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    size_t get_quantized_feature_length() {
        return _per_fea_len * _total_point_count * sizeof(unsigned char);
    }

    /*
    * @brief decode
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    std::unique_ptr<float[]> decode(uint64_t idx) const;
    /*
    * @brief 加载索引
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int load_quantized_feature(const std::string& quantized_feafile,
                               const uint32_t* cnts_index);
    /*
    * @brief 初始化码本所需内存
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int init_codebooks_memory();
private:
    const int _per_subspace_len;
    const int _fea_offset;
    uint32_t _total_point_count;
    int _per_fea_len;
    QuantizationParams _params;
    std::unique_ptr<float[]> _coodbooks;
    std::unique_ptr<unsigned char[]> _quantized_feature;
};

}//namespace puck

