/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    conf.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-28 14:23
 * @brief
 ***********************************************************************/

#include <base/logging.h>
#include "hierarchical_cluster/index_conf.h"
#include "gflags/puck_gflags.h"
namespace puck {

IndexConf::IndexConf() {
    //数据集相关参数
    ks = 256;                               //现在PQ量化仅支持ks=256;
    ip2cos = FLAGS_ip2cos;
    feature_dim = FLAGS_feature_dim;

    if (ip2cos == 1) {
        feature_dim++;
    }

    //如果没有指定nsq, nsq = feature_dim
    google::CommandLineFlagInfo info;

    if (google::GetCommandLineFlagInfo("nsq", &info) && info.is_default) {
        std::string feature_dim_str = std::to_string(feature_dim);
        google::SetCommandLineOption("nsq", feature_dim_str.c_str());
    } else if (FLAGS_feature_dim == FLAGS_nsq && ip2cos == 1) {
        std::string feature_dim_str = std::to_string(feature_dim);
        google::SetCommandLineOption("nsq", feature_dim_str.c_str());
    }

    nsq = FLAGS_nsq;

    lsq = feature_dim / nsq;

    whether_pq = FLAGS_whether_pq;
    whether_norm = FLAGS_whether_norm;

    if (ip2cos > 0) {
        whether_norm = false;
    }

    total_point_count = 0;
    gnoimi_search_cells = std::min(FLAGS_gnoimi_search_cells, FLAGS_gnoimi_coarse_cells_count);

    neighbors_count = FLAGS_neighbors_count;
    topk = FLAGS_topk;

    //加载文件相关参数
    threads_count = FLAGS_threads_count;
    gnoimi_coarse_cells_count = FLAGS_gnoimi_coarse_cells_count;
    gnoimi_fine_cells_count = FLAGS_gnoimi_fine_cells_count;

    //索引文件
    index_path = FLAGS_index_path;
    feature_file_name = index_path + "/" + FLAGS_feature_file_name;
    coarse_code_book_file_name = index_path + "/" + FLAGS_coarse_code_book_file_name;
    fine_code_book_file_name = index_path + "/" + FLAGS_fine_code_book_file_name;

    cell_assign_file_name = index_path + "/" + FLAGS_cell_assign_file_name;

    pq_codebook_file_name = index_path + "/" + FLAGS_pq_codebook_file_name;
    key_file_name = index_path + "/" + FLAGS_key_file_name;

    cell_keep_ratio = 1;

    filter_nsq = FLAGS_filter_nsq;
    filter_topk = FLAGS_filter_topk;
    whether_filter = FLAGS_whether_filter;
    filter_data_file_name = index_path + "/" + FLAGS_filter_data_file_name;
    filter_codebook_file_name = index_path + "/" + FLAGS_filter_codebook_file_name;

    index_file_name = index_path + "/" + FLAGS_index_file_name;

    pq_data_file_name = index_path + "/" + FLAGS_pq_data_file_name;

    if (FLAGS_using_tinker == true) {
        index_version = 2;
    } else {
        index_version = 1;
    }

    tinker_search_range = FLAGS_tinker_search_range;
}

void IndexConf::adaptive_train_param() {
    //tinker
    if (index_version == 2) {
        whether_filter = false;
        whether_pq = false;
        return;
    }

    if (index_version == 1) {

        google::CommandLineFlagInfo info;
        bool unset_whether_filter = google::GetCommandLineFlagInfo("whether_filter", &info) && info.is_default;

        if (!unset_whether_filter) {
            return;
        }

        //filter_nsq 未设置
        whether_filter = true;
        bool unset_filter_nsq = google::GetCommandLineFlagInfo("filter_nsq", &info) && info.is_default;

        if (!unset_filter_nsq) {
            return ;
        }

        if (unset_filter_nsq && ip2cos == 0) {
            uint32_t lsq = 4;

            do {
                if (feature_dim % lsq == 0) {
                    filter_nsq = feature_dim / lsq;
                    show();
                    return;
                }
            } while (++lsq <= feature_dim / 2);
        }

        if (unset_filter_nsq) {
            uint32_t lsq = 4;

            do {
                uint32_t n = std::ceil(1.0 * feature_dim / lsq);

                if (n * lsq - feature_dim < lsq) {
                    filter_nsq = n;
                    return;
                }
            } while (++lsq <= feature_dim / 2);
        }

        whether_filter = false;
    }
}
int IndexConf::adaptive_search_param() {
    //检索参数检查
    if (index_version == 2) {
        tinker_search_range = FLAGS_tinker_search_range;
        google::CommandLineFlagInfo info;

        //如果没有设置，更新检索参数（与topK有关）
        if (google::GetCommandLineFlagInfo("tinker_search_range", &info) && info.is_default) {
            tinker_search_range = 5 * topk;
        }

        //Tinker的检索一级聚类中心的个数很少，20足矣
        if (google::GetCommandLineFlagInfo("gnoimi_search_cells", &info) && info.is_default) {
            gnoimi_search_cells = std::min(20, (int)gnoimi_coarse_cells_count);
        }
    } else {
        google::CommandLineFlagInfo info;

        //如果没有设置filter_topk，更新filter_topk，取值与topk有关
        if (google::GetCommandLineFlagInfo("filter_topk", &info) && info.is_default) {
            filter_topk = topk * 11;
        }

        //shoud be topk <= filter_topk <= neighbors_count;
        if (neighbors_count < filter_topk) {
            LOG(ERROR) << "neighbors_count should >= filter_topk, should update the search params";
            return -1;
        }

        if (filter_topk < topk) {
            LOG(ERROR) << "filter_topk should >= topk, should update the search params";
            return -1;
        }
    }

    return 0;
}

void IndexConf::show() {
    LOG(NOTICE) << "feature_dim = " << feature_dim;
    LOG(NOTICE) << "whether_norm = " << whether_norm;
    LOG(NOTICE) << "index_version = " << index_version;

    //filer
    if (whether_filter) {
        LOG(NOTICE) << "whether_filter = " << whether_filter;
        LOG(NOTICE) << "filter_nsq = " << filter_nsq;
        LOG(NOTICE) << "ks = " << ks;
    }

    //pq
    if (whether_pq) {
        LOG(NOTICE) << "whether_pq = " << whether_pq;
        LOG(NOTICE) << "nsq = " << nsq;
        LOG(NOTICE) << "ks = " << ks;
    }

    LOG(NOTICE) << "gnoimi_coarse_cells_count = " << gnoimi_coarse_cells_count;
    LOG(NOTICE) << "gnoimi_fine_cells_count = " << gnoimi_fine_cells_count;

    LOG(NOTICE) << "gnoimi_search_cells = " << gnoimi_search_cells;

    if (index_version == 2) {
        LOG(NOTICE) << "tinker_neighborhood = " << FLAGS_tinker_neighborhood;
        LOG(NOTICE) << "tinker_construction = " << FLAGS_tinker_construction;
        LOG(NOTICE) << "tinker_search_range = " << tinker_search_range;
    }

    LOG(NOTICE) << "neighbors_count = " << neighbors_count;
    LOG(NOTICE) << "topk = " << topk;

    if (ip2cos) {
        LOG(NOTICE) << "ip2cos = " << ip2cos;
    }

    LOG(NOTICE) << key_file_name;
}

} //namesapce gnoimi
