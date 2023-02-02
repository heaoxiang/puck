/***************************************************************************
*
* Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
*
************************************************************************/

/**
* @File Name : distribute_build_puck.cpp
* @Author : yinjie06
* @Mail : yinjie06@baidu.com
* @Created Time : 2019-04-16 15:36:06
* @Brief :
**/

#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <cstdlib>
#include <glog/logging.h>
#include <gflags/gflags.h>
//#include <boost/algorithm/string.hpp>
//#include <boost/lexical_cast.hpp>
#include "gflags/puck_gflags.h"
#include "puck/puck_index.h"
#include "string_split.h"

void incrcounter(std::string countername) {
    std::cerr << "reporter:counter:puck_buildindex," << countername << ",1" << std::endl;
}
puck::IndexConf conf;

int parse_input_stream(const std::string& input_str, const u_int32_t dim,
                       puck::PuckBuildInfo& build_info) {

    std::vector<std::string> splited_str;

    if (puck::s_split(input_str, "\t", splited_str) < 2) {
        LOG(WARNING) << "parse_input_stream error";
        return -1;
    }

    build_info.other_str_data = "";

    if (splited_str.size() == 3) {
        build_info.other_str_data = splited_str[2];
    }

    build_info.lable = splited_str[0];
    std::vector<std::string> splited_fea;
    
    if (puck::s_split(splited_str[1], " ", splited_fea) != dim) {

        for (uint32_t idx =0;idx < splited_fea.size();++idx){
            LOG(WARNING) << idx<<" "<<splited_fea[idx];
        }
        LOG(WARNING) << "parse_input_stream error "<<splited_fea.size();
        return -2;
    }

    build_info.feature.resize(dim);

    for (u_int32_t i = 0; i < splited_fea.size(); ++i) {
        build_info.feature[i] = atof(splited_fea[i].c_str());
    }

    return 0;
}

void compute_single_assigns_subset(const char* line, const u_int32_t dim, void* para) {
    std::string line_str = line;
    puck::PuckBuildInfo build_info;
    int ret = parse_input_stream(line_str, dim, build_info);

    if (ret != 0) {
        incrcounter("paser_input_error");
        LOG(WARNING) << line;
        return;
    }

    puck::PuckIndex* p = (puck::PuckIndex*)para;
    ret = p->single_build(&build_info);

    if (ret != 0) {
        incrcounter("cmp_build_info_error");
        LOG(WARNING) << line;
        return;
    }

    std::vector<std::string> quantizated_feature_str(build_info.quantizated_feature.size());
    for(uint32_t idx = 0; idx < quantizated_feature_str.size(); ++idx){
        auto& cur_feature = build_info.quantizated_feature[idx];
        auto& cur_feature_str = quantizated_feature_str[idx];
        float offset_val = cur_feature.first;
        cur_feature_str="";
        for (uint32_t i = 0; i < cur_feature.second.size(); ++i) {
            if(cur_feature_str.length() > 0){
                cur_feature_str += " ";
            }
            cur_feature_str += std::to_string((int)cur_feature.second[i]);
        }
        cur_feature_str = std::to_string(offset_val) + "|" + cur_feature_str;
    }

    if(conf.whether_pq == false){
        std::string flat_feature_str("");
        for (u_int32_t i = 0; i < build_info.feature.size(); ++i) {
            if(flat_feature_str.length() > 0){
                flat_feature_str += " ";
            }
            flat_feature_str += std::to_string(build_info.feature[i]);
        }
        quantizated_feature_str.push_back(flat_feature_str);
    }
    
    std::string fea_str;
    for(uint32_t idx = 0; idx < quantizated_feature_str.size(); ++idx){
        fea_str += quantizated_feature_str[idx] + ",";
    }

    printf("%d\t%s\t%s\t%s\n", build_info.nearest_cell.cell_id,
           build_info.lable.c_str(),
           fea_str.c_str(),
           build_info.other_str_data.c_str());
}

int main(int argc, char** argv) {
    //从索引加载文件
    //com_loadlog("./conf", "puck_log.conf");
    
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    std::unique_ptr<puck::PuckIndex> cluster;
    
    conf = puck::load_index_conf_file();

    cluster.reset(new puck::PuckIndex());
    
    int ret = cluster->init_single_build();
    if (ret != 0) {
        std::cout << "init error:" << ret << std::endl;
        return -1;
    }

   
    const u_int32_t feature_dim = conf.feature_dim;

    //开始读取数据流
    std::string last_key("");
    int cnt = 0;

    while (1) {

        std::string line;

        if (!std::getline(std::cin, line)) {
            LOG(WARNING) << "read to end or empty line";
            break;
        }

        std::size_t found = line.find_first_of("\t", 0);
        std::string cur_doc_key = line.substr(0, found);

        if (cur_doc_key == last_key) {
            incrcounter("duplicate_key");
            continue;
        }

        last_key = cur_doc_key;
        compute_single_assigns_subset(line.c_str(),
                                      feature_dim, cluster.get());
        if (cnt > 10000) {
            cnt = 0;
            std::string cmd = "ps u -p " + std::to_string(getpid());
            FILE* fp = nullptr;

            if ((fp = popen(cmd.c_str(), "r")) != nullptr) {
                char temp_ret[1024];
                cmd = "\n";

                while (fgets(temp_ret, sizeof(temp_ret), fp) != nullptr) {
                    cmd += temp_ret;
                }

                pclose(fp);
                LOG(WARNING) << cmd;
            }
        }

    }
    return 0;
}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

