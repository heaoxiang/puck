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

int main(int argc, char** argv) {
    //com_loadlog("./conf", "puck_log.conf");
    google::ParseCommandLineFlags(&argc, &argv, true);
    //google::InitGoogleLogging("build");
    LOG(INFO)<<"FLAGS_log_dir = "<<FLAGS_log_dir;
    std::unique_ptr<puck::HierarchicalCluster> cluster;
    
    //cluster->read_model_file();
    auto conf = puck::load_index_conf_file();
    
    if (conf.index_version == 2) { //Tinker
        LOG(INFO) << "init index of Tinker";
        cluster.reset(new puck::TinkerIndex());
    } else if (conf.index_version == 1 && conf.whether_filter == true) { //PUCK
        LOG(INFO) << "init index of Puck";
        cluster.reset(new puck::PuckIndex());
    } else if (conf.index_version == 1 && conf.whether_filter == false) {
        //Flat
        cluster.reset(new puck::HierarchicalCluster());
        LOG(INFO) << "init index of Flat";
    } else {
        LOG(INFO) << "init index of Error, Nan type";
        return -1;
    }

    cluster->build();
    //cluster->save_index();

    return 0;
}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

