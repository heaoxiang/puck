/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    conf.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-28 14:23
 * @brief
 ***********************************************************************/
#include <math.h>
#include <glog/logging.h>
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
    search_coarse_count = std::min(FLAGS_search_coarse_count, FLAGS_coarse_cluster_count);

    neighbors_count = FLAGS_neighbors_count;
    topk = FLAGS_topk;

    //加载文件相关参数
    threads_count = FLAGS_threads_count;
    coarse_cluster_count = FLAGS_coarse_cluster_count;
    fine_cluster_count = FLAGS_fine_cluster_count;

    //索引文件
    index_path = FLAGS_index_path;
    feature_file_name = index_path + "/" + FLAGS_feature_file_name;
    coarse_codebook_file_name = index_path + "/" + FLAGS_coarse_codebook_file_name;
    fine_codebook_file_name = index_path + "/" + FLAGS_fine_codebook_file_name;

    cell_assign_file_name = index_path + "/" + FLAGS_cell_assign_file_name;

    pq_codebook_file_name = index_path + "/" + FLAGS_pq_codebook_file_name;
    label_file_name = index_path + "/" + FLAGS_label_file_name;

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
        if (google::GetCommandLineFlagInfo("search_coarse_count", &info) && info.is_default) {
            search_coarse_count = std::min(20, (int)coarse_cluster_count);
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
    LOG(INFO) << "feature_dim = " << feature_dim;
    LOG(INFO) << "whether_norm = " << whether_norm;
    LOG(INFO) << "index_version = " << index_version;

    //filer
    if (whether_filter) {
        LOG(INFO) << "whether_filter = " << whether_filter;
        LOG(INFO) << "filter_nsq = " << filter_nsq;
        LOG(INFO) << "ks = " << ks;
    }

    //pq
    if (whether_pq) {
        LOG(INFO) << "whether_pq = " << whether_pq;
        LOG(INFO) << "nsq = " << nsq;
        LOG(INFO) << "ks = " << ks;
    }

    LOG(INFO) << "coarse_cluster_count = " << coarse_cluster_count;
    LOG(INFO) << "fine_cluster_count = " << fine_cluster_count;

    LOG(INFO) << "search_coarse_count = " << search_coarse_count;

    if (index_version == 2) {
        LOG(INFO) << "tinker_neighborhood = " << FLAGS_tinker_neighborhood;
        LOG(INFO) << "tinker_construction = " << FLAGS_tinker_construction;
        LOG(INFO) << "tinker_search_range = " << tinker_search_range;
    }

    LOG(INFO) << "neighbors_count = " << neighbors_count;
    LOG(INFO) << "topk = " << topk;

    if (ip2cos) {
        LOG(INFO) << "ip2cos = " << ip2cos;
    }

    LOG(INFO) << label_file_name;
}

} //namesapce puck
