/***************************************************************************
 *
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved *
 **************************************************************************/ /**
 * @file   search_context.h
 * @author huangben(huangben@baidu.com)
 * @date   2018年08月13日 星期一 19时50分32秒
 * @brief
 *
 **/
#pragma once
#include <vector>
#include "puck/index_conf.h"
#include "puck/index.h"
namespace puck {
class FineCluster;
//typedef std::vector<std::pair<float, FineCluster*>>
//        DistanceInfo; //存储query到二级类聚中心距离的结构
typedef std::vector<std::pair<float, std::pair<FineCluster*, uint32_t>>> DistanceInfo;

struct SearchCellData {
    float* query_norm;                  //存储归一后的query,长度=feature_dim
    float* cluster_inner_product;       //query与聚类中心的内积,长度=max(coarse_cluster_count, fine_cluster_count)
    float* coarse_distance;             //query与一级聚类中心的距离，和coarse_tag一起在最大堆调整时使用,长度=search_coarse_count
    uint32_t* coarse_tag;              //与query距离最近的一级聚类中心的id,长度=search_coarse_count
    float* fine_distance;
    uint32_t* fine_tag;
    DistanceInfo cell_distance;
    SearchCellData() {
        init();
    }
    void init() {
        query_norm = nullptr;
        cluster_inner_product = nullptr;
        coarse_distance = nullptr;
        coarse_tag = nullptr;
        fine_distance = nullptr;
        fine_tag = nullptr;
    }
};

struct SearchDocData {
    float* result_distance;         //query与doc的距离,长度=topk
    uint32_t* result_tag;          //doc的local index,通过local index查cnts,长度=topk
    float* pq_dist_table;
    uint32_t* query_sorted_tag;
    float* query_sorted_dist;
    SearchDocData() {
        init();
    }
    void init() {
        result_distance = nullptr;
        result_tag = nullptr;
        pq_dist_table = nullptr;
        query_sorted_tag = nullptr;
        query_sorted_dist = nullptr;

    }
};
struct Request;
class SearchContext {
public:
    SearchContext();
    virtual ~SearchContext();

    uint64_t get_logid() {
        return _logid;
    }
    uint64_t set_logid(uint64_t logid) {
        return _logid = logid;
    }
    void set_request(const Request* request) {
        _request = request;
    }
    const Request* get_request() {
        return _request;
    }
    /**
     * @brief push notice
     */
    inline void log_push(const char* key, const char* fmt, ...);

    const std::string& get_log_string() {
        return _log_string;
    }

    /**
     * @brief 使Context对象恢复初始状态（以便重复使用）
     *
     * @return  void
     **/
    int reset(const IndexConf& conf);


    SearchCellData& get_search_cell_data() {
        return _search_cell_data;
    }

    SearchDocData& get_search_doc_data() {
        return _search_doc_data;
    }

    void set_debug_mode(bool debug) {
        _debug = debug;
    }
    bool debug() {
        return _debug;
    }

private:
    uint64_t _logid;
    const Request* _request;
    bool _debug;
    bool _inited;
    char* _model;
    IndexConf _init_conf;
    std::string _log_string;
    SearchCellData _search_cell_data;
    SearchDocData _search_doc_data;
    //DISALLOW_COPY_AND_ASSIGN(SearchContext);
};
/*
inline void SearchContext::log_push(const char* key, const char* fmt, ...) {
    if (!_debug) {
        return;
    }

    char tmp[256];
    tmp[0] = '\0';
    snprintf(tmp, 256, "[%s: %s] ", key, fmt);

    va_list args;
    va_start(args, fmt);
    base::string_vappendf(&_log_string, tmp, args);
    va_end(args);
}
*/
} //namesapce puck
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

