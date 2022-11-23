/***************************************************************************
 *
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/



/**
 * @file gnoimi_gflags.cpp
 * @author huangben(com@baidu.com)
 * @date 2018/7/07 15:59:36
 * @brief
 *
 **/

#include "gflags/puck_gflags.h"
#include <thread>
namespace puck {
DEFINE_int32(context_initial_pool_size, std::thread::hardware_concurrency(), "initial pool size");
//索引文件，默认不需要修改
DEFINE_string(index_path, "puck_index", "lib of index files");
DEFINE_string(feature_file_name, "all_data.feat.bin", "feature data");
DEFINE_string(coarse_code_book_file_name, "GNOIMI_coarse.dat", "coarse codebook");
DEFINE_string(fine_code_book_file_name, "GNOIMI_fine.dat", "fine codebook");

DEFINE_string(cell_assign_file_name, "cell_assign.dat", "cell assign");
DEFINE_string(pq_codebook_file_name, "learn_codebooks.dat", "pq codebook");
DEFINE_string(key_file_name, "all_data.url", "all key of docs");

//检索相关参数
//数据集相关，需要配置
DEFINE_int32(feature_dim, 256, "feature dim");
DEFINE_int32(nsq, FLAGS_feature_dim, "the count of pq sub space");
DEFINE_bool(whether_pq, true, "whether pq");
DEFINE_bool(whether_norm, true, "whether norm");

//检索相关
DEFINE_int32(gnoimi_search_cells, 200, "search fine clusters in top-N coarse clusters");

DEFINE_int32(neighbors_count, 40000, "search docs count, default value is 4w");
DEFINE_int32(topk, 100, "return top-k");

//加载文件相关
DEFINE_int32(gnoimi_coarse_cells_count, 2000, "the number of coarse clusters");
DEFINE_int32(gnoimi_fine_cells_count, 2000, "the number of fine clusters");
DEFINE_int32(threads_count, std::thread::hardware_concurrency(), "threads count");


//filter
DEFINE_bool(whether_filter, true, "whether filer");
DEFINE_int32(filter_nsq, FLAGS_feature_dim / 4, "the count of pq sub space");
DEFINE_int32(filter_topk, FLAGS_topk * 11, "filer top-k");
DEFINE_string(filter_codebook_file_name, "filter_codebook.dat", "filter_data_file_name");
DEFINE_string(filter_data_file_name, "filter_data.dat", "filter_data_file_name");

DEFINE_int32(ip2cos, 0, "Convert ip to cos, 0-NA, 1-directly，2-need transform");
//new format
DEFINE_string(index_file_name, "index.dat", "store index format information");
DEFINE_string(pq_data_file_name, "pq_data.dat", "pq_data_file_name, use in new index struct");

//tinker
DEFINE_bool(using_tinker, false, "");
DEFINE_string(tinker_file_name, "tinker_relations.dat", "tinker_file_name");
DEFINE_int32(tinker_neighborhood, 16, "neighborhood conut");
DEFINE_int32(tinker_construction, 600, "tinker_construction");
DEFINE_int32(tinker_search_range, FLAGS_topk * 5, "tinker search param, tinker_search_range");

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100 */
