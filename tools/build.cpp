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

DEFINE_bool(using_tinker, false, "");
int main(int argc, char** argv) {
    //com_loadlog("./conf", "puck_log.conf");
    google::ParseCommandLineFlags(&argc, &argv, true);
    //google::InitGoogleLogging("build");
    LOG(INFO)<<"FLAGS_log_dir = "<<FLAGS_log_dir;
    std::unique_ptr<puck::Index> index;
    
    if (FLAGS_using_tinker) {
        index.reset(new puck::TinkerIndex());
    } else if (puck::FLAGS_whether_filter == true) {
        index.reset(new puck::PuckIndex());
    } else if (puck::FLAGS_whether_filter == false) {
        index.reset(new puck::HierarchicalClusterIndex());
    }

    if(index->build() != 0){
        LOG(ERROR)<<"build Fail.\n";
        return -1;
    }
    LOG(ERROR) << "build Suc.\n";
    return 0;
}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

