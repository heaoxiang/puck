/***************************************************************************
*
* Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
*
************************************************************************/

/**
* @File Name : train.cpp
* @Author : yinjie06
* @Mail : yinjie06@baidu.com
* @Created Time : 2019-04-16 15:35:30
* @Brief :
**/

#include <iostream>
#include <string>
#include <cstdlib>
#include <glog/logging.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include "puck/tinker/tinker_index.h"
#include "puck/gflags/puck_gflags.h"
#include "puck/puck/puck_index.h"

DEFINE_int32(index_type, 1, "");
//获取文件行数，index初始化时候通过key file确定样本总个数
int getFileLineCnt(const char* fileName) {
    struct stat st;

    if (stat(fileName, &st) != 0) {
        return 0;
    }

    char buff[1024];
    sprintf(buff, "wc -l %s", fileName);

    FILE* fstream = nullptr;
    fstream = popen(buff, "r");
    int total_line_cnt = -1;

    if (fstream) {
        memset(buff, 0x00, sizeof(buff));

        if (fgets(buff, sizeof(buff), fstream)) {
            int index = strchr((const char*)buff, ' ') - buff;
            buff[index] = '\0';
            total_line_cnt =  atoi(buff);
        }
    }

    if (fstream) {
        pclose(fstream);
    }

    return total_line_cnt;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << "start to train\n";
    puck::InitializeLogger(0);
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

    if(index->train() != 0){
        LOG(ERROR) << "train Fail.\n";
        return -1;
    }

    LOG(INFO) << "train Suc.\n";
    return 0;
}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

