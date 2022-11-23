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
#include <malloc.h>
#include <base/logging.h>
#include "search_context.h"
//#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)

namespace puck {
extern const u_int64_t cache_offset_size;
SearchContext::SearchContext() :
    _logid(0),
    //_topk(0),
    _debug(false),
    _inited(false),
    _model(nullptr),
    _log_string("") {
}

SearchContext::~SearchContext() {
    if (_model) {
        free(_model);
        _search_cell_data.init();
        _search_doc_data.init();
    }

}

int SearchContext::reset(const IndexConf& conf) {
    _logid = 0;
    _log_string = "";
    _debug = false;

    if (_inited == false ||  _init_conf.filter_topk < conf.filter_topk
            || _init_conf.gnoimi_search_cells < conf.gnoimi_search_cells
            || _init_conf.neighbors_count < conf.neighbors_count) {
        _inited = false;
        _init_conf = conf;

        if (_model) {
            free(_model);
            _search_cell_data.init();
            _search_doc_data.init();
        }
    }

    //_topk = conf.topk;
    //二级聚类中心最多需要top-neighbors_count个cell，空间多申请一些可避免频繁更新tag idx
    unsigned int all_cells_cnt = 1;

    if (conf.index_version == 1) {
        all_cells_cnt = conf.neighbors_count * 1.1;
    }

    if (all_cells_cnt > conf.gnoimi_search_cells * conf.gnoimi_fine_cells_count) {
        all_cells_cnt = conf.gnoimi_search_cells * conf.gnoimi_fine_cells_count;
    }

    if (_search_cell_data.cell_distance.size() != all_cells_cnt) {
        _search_cell_data.cell_distance.resize(all_cells_cnt);
    }

    if (_inited) {
        return 0;
    }

    size_t model_size = 0;
    //每个过程需要的内存
    //coarse
    size_t coarse_ip_dist = sizeof(float) * conf.gnoimi_coarse_cells_count;
    size_t coarse_heap_size = (sizeof(float) + sizeof(uint32_t)) * conf.gnoimi_search_cells;
    size_t stage_coarse = coarse_ip_dist + coarse_heap_size;

    model_size = std::max(model_size, stage_coarse);
    //fine
    size_t fine_ip_dist = sizeof(float) * conf.gnoimi_fine_cells_count;
    size_t fine_ip_heap_size = (sizeof(float) + sizeof(uint32_t)) * conf.gnoimi_fine_cells_count;

    size_t stage_fine = coarse_heap_size + fine_ip_dist + fine_ip_heap_size;
    model_size = std::max(model_size, stage_fine);

    if (conf.whether_filter) {
        //filter
        size_t filter_dist_table = sizeof(float) * conf.filter_nsq * conf.ks;
        size_t filter_heap = (sizeof(float) + sizeof(uint32_t)) * conf.filter_topk;
        size_t pq_reorder = (sizeof(float) + sizeof(uint32_t)) * conf.filter_nsq;
        size_t stage_filter = pq_reorder + filter_dist_table + filter_heap;
        model_size = std::max(model_size, stage_filter);


        if (conf.whether_pq) {
            //rank
            size_t pq_dist_table = sizeof(float) * conf.nsq * conf.ks;
            size_t pq_stage = pq_dist_table + filter_heap;
            model_size = std::max(model_size, pq_stage);
        }
    }

    model_size += sizeof(float) * conf.feature_dim;

    void* memb = nullptr;
    int32_t pagesize = getpagesize();

    size_t size = model_size + (pagesize - model_size % pagesize);
    //LOG(NOTICE) << pagesize << " " << model_size << " " << size;
    int err = posix_memalign(&memb, pagesize, size);

    if (err != 0) {
        std::runtime_error("alloc_aligned_mem_failed errno=" + errno);
        return -1;
    }

    _model = reinterpret_cast<char*>(memb);
    _search_cell_data.query_norm = (float*)_model;
    char* temp = _model + sizeof(float) * conf.feature_dim;

    _search_cell_data.coarse_distance = (float*)temp;
    temp +=  sizeof(float) * conf.gnoimi_search_cells;

    _search_cell_data.coarse_tag = (uint32_t*)temp;
    temp +=  sizeof(uint32_t) * conf.gnoimi_search_cells;

    _search_cell_data.cluster_inner_product = (float*)temp;
    temp += sizeof(float) * std::max(conf.gnoimi_fine_cells_count, conf.gnoimi_coarse_cells_count);

    _search_cell_data.fine_distance = (float*)temp;
    temp += sizeof(float) * conf.gnoimi_fine_cells_count;

    _search_cell_data.fine_tag = (uint32_t*)temp;


    temp = _model + sizeof(float) * conf.feature_dim;

    if (conf.whether_filter) {
        _search_doc_data.result_distance = (float*)temp;
        temp += sizeof(float) * conf.filter_topk;

        _search_doc_data.result_tag = (uint32_t*)temp;
        temp += sizeof(uint32_t) * conf.filter_topk;

        _search_doc_data.pq_dist_table = (float*)temp;
        temp += sizeof(uint32_t) * conf.filter_nsq * conf.ks;

        _search_doc_data.query_sorted_tag = (uint32_t*)temp;
        temp += sizeof(uint32_t) * conf.filter_nsq;

        _search_doc_data.query_sorted_dist = (float*)temp;
    }

    _inited = true;

    return 0;
}

} //namesapce gnoimi
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

