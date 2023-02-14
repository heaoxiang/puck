/***************************************************************************
*
* Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
*
************************************************************************/

/**
* @File Name : build.cpp
* @Author : yinjie06
* @Mail : yinjie06@baidu.com
* @Created Time : 2019-04-16 15:36:06
* @Brief :
**/

#include <iostream>
#include <string>
#include <cstdlib>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "tinker/tinker_index.h"
#include "gflags/puck_gflags.h"
#include "puck/puck_index.h"

DEFINE_int32(index_type, 1, "");
int main(int argc, char** argv) {
    //com_loadlog("./conf", "puck_log.conf");
    google::ParseCommandLineFlags(&argc, &argv, true);
    //google::InitGoogleLogging("build");
    LOG(INFO)<<"FLAGS_log_dir = "<<FLAGS_log_dir;
    std::unique_ptr<puck::Index> index;
    
    if (FLAGS_index_type == int(puck::IndexType::TINKER)) {
        index.reset(new puck::TinkerIndex());
    } else if (FLAGS_index_type == int(puck::IndexType::PUCK)) {
        index.reset(new puck::PuckIndex());
    } else if (FLAGS_index_type == int(puck::IndexType::HIERARCHICAL_CLUSTER)) {
        index.reset(new puck::HierarchicalClusterIndex());
    } else {
        LOG(ERROR) << "index type error.\n";
        return -1;
    }

    if(index->build() != 0){
        LOG(ERROR) << "build Fail.\n";
        return -1;
    }
    LOG(INFO) << "build Suc.\n";
    return 0;
}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

