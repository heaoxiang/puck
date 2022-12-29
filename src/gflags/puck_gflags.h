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

#ifndef BAIDU_MMS_GRAPH_GNOIMI_GFLAGS_H
#define BAIDU_MMS_GRAPH_GNOIMI_GFLAGS_H

#include <gflags/gflags.h>

namespace puck {

//内存池初始化大小
//DECLARE_int32(context_initial_pool_size);

//索引文件，默认不需要修改
DECLARE_string(index_path);
DECLARE_string(feature_file_name);
DECLARE_string(coarse_code_book_file_name);
DECLARE_string(fine_code_book_file_name);

DECLARE_string(cell_assign_file_name);
DECLARE_string(pq_codebook_file_name);

DECLARE_string(key_file_name);

//检索相关参数
//数据集相关,需要配置
DECLARE_int32(nsq);
DECLARE_int32(feature_dim);
DECLARE_bool(whether_pq);
DECLARE_bool(whether_norm);

//检索相关
DECLARE_int32(gnoimi_search_cells);
DECLARE_int32(neighbors_count);
DECLARE_int32(topk);

//加载文件相关参数
DECLARE_int32(gnoimi_coarse_cells_count);
DECLARE_int32(gnoimi_fine_cells_count);
DECLARE_int32(threads_count);

//一些功能开关
DECLARE_int32(ip2cos);

//filer
DECLARE_bool(whether_filter);
DECLARE_int32(filter_nsq);
DECLARE_int32(filter_topk);
DECLARE_string(filter_codebook_file_name);
DECLARE_string(filter_data_file_name);

//new format
DECLARE_string(index_file_name);
DECLARE_string(pq_data_file_name);

//tinker
DECLARE_bool(using_tinker);
DECLARE_string(tinker_file_name);
DECLARE_int32(tinker_neighborhood);
DECLARE_int32(tinker_construction);
DECLARE_int32(tinker_search_range);

}

#endif //BAIDU_MMS_GRAPH_GNOIMI_GFLAGS_H
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100 */
