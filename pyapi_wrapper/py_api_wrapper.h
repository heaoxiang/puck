/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    py_puck_api_wrapper.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2021-08-18 14:30
 * @brief
 ***********************************************************************/
#pragma once

#include <iostream>
#include "puck/hierarchical_cluster/hierarchical_cluster_index.h"
namespace py_puck_api {

void update_gflag(const char* gflag_key, const char* gflag_val);
class PySearcher {
public:
    PySearcher();
    void show();
    int init();
    int build(uint32_t n);
    int search(uint32_t n, const float* query_fea,const uint32_t topk, float* distance, uint32_t* labels);
    //int each_search(const float* query_fea, float* distance, uint32_t* labels);
    void update_params(uint32_t topk, uint32_t search_cells, uint32_t neighbors, uint32_t filter_topk);
    ~PySearcher();
private:
    std::unique_ptr<puck::Index> _index;
    uint32_t _dim;
};
};//namespace py_puck_api

