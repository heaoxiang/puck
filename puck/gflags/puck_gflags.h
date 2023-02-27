/***************************************************************************
 *
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/

/**
 * @file envoy_gflags.h
 * @author huangben(com@baidu.com)
 * @date 2018/7/07 15:59:36
 * @brief
 *
 **/

#pragma once

#include <gflags/gflags.h>

namespace puck {

DECLARE_bool(need_log_file);
DECLARE_string(puck_log_file);

/*****训练&建库参数******/
//通用参数
DECLARE_string(index_path);
DECLARE_string(index_file_name);
DECLARE_string(feature_file_name);
DECLARE_string(coarse_codebook_file_name);
DECLARE_string(fine_codebook_file_name);

DECLARE_string(cell_assign_file_name);
DECLARE_string(label_file_name);

DECLARE_int32(feature_dim);
DECLARE_bool(whether_norm);

DECLARE_int32(coarse_cluster_count);
DECLARE_int32(fine_cluster_count);
DECLARE_int32(threads_count);

DECLARE_int32(ip2cos);

//puck 
DECLARE_bool(whether_pq);
DECLARE_int32(nsq);
DECLARE_string(pq_codebook_file_name);
DECLARE_string(pq_data_file_name);
DECLARE_bool(whether_filter);
DECLARE_int32(filter_nsq);
DECLARE_string(filter_codebook_file_name);
DECLARE_string(filter_data_file_name);

//tinker
DECLARE_string(tinker_file_name);
DECLARE_int32(tinker_neighborhood);
DECLARE_int32(tinker_construction);

/***********检索参数*********/
//检索时，初始化内存池的size
DECLARE_int32(context_initial_pool_size);
//检索通用参数
DECLARE_int32(search_coarse_count);
DECLARE_int32(topk);

//puck 
DECLARE_int32(filter_topk);
DECLARE_int32(neighbors_count);

//tinker
DECLARE_int32(tinker_search_range);

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100 */
