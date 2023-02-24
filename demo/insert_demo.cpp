/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    insert_demo.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-11-11 17:04
 * @brief
 ***********************************************************************/

//#include <boost/algorithm/string.hpp>
//#include <boost/lexical_cast.hpp>
#include "puck/puck/puck_index.h"
#include "puck/puck/realtime_insert_puck_index.h"
#include "tools/string_split.h"


int read_feature_data(std::string& input_file, std::vector<std::string>& pic_name,
                      std::vector<std::vector<float> >& doc_feature) {
    std::ifstream fin;
    fin.open(input_file.c_str(), std::ios::binary);

    if (!fin.good()) {
        LOG(ERROR) << "cann't open output file:" << input_file.c_str();
        return -1;
    }

    int ret = 0;
    std::string line;

    pic_name.clear();
    doc_feature.clear();

    while (std::getline(fin, line)) {
        std::vector<std::string> split_str;

        if (puck::s_split(line, "\t", split_str) < 2) {
            LOG(ERROR) << "id:" << pic_name.size() << " get name error.";
            ret = -1;
            break;
        }

        pic_name.push_back(split_str[0]);
        std::string feature_str = split_str[1];

        puck::s_split(feature_str, " ", split_str);

        std::vector<float> cur_feature;

        for (u_int32_t idx = 0; idx < split_str.size(); ++idx) {
            cur_feature.push_back(std::atof(split_str[idx].c_str()));
        }

        doc_feature.push_back(cur_feature);
    }

    fin.close();
    LOG(INFO) << "total query cnt = " << pic_name.size();
    return ret;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    //1. load index
    std::unique_ptr<puck::RealtimeInsertPuckIndex> searcher;

    searcher.reset(new puck::RealtimeInsertPuckIndex());

    if (searcher == nullptr) {
        LOG(ERROR) << "create new SearchInterface error.";
        return -2;
    }

    int ret = searcher->init();

    if (ret != 0) {
        LOG(ERROR) << "SearchInterface init error " << ret;
        return -3;
    }

    //2. read input
    std::string input(argv[1]);
    std::string output(argv[2]);
    std::vector<std::vector<float> > in_data;
    std::vector<std::string> pic_name;

    ret = read_feature_data(input, pic_name, in_data);

    if (ret != 0) {
        LOG(ERROR) << "read_feature_data error:" << ret;
        return -4;
    } else {
        LOG(INFO) << "read_feature_data item:" << in_data.size();
    }


    //return 0;

    //3. search
    const int item_count = in_data.size();

    FILE* pf = nullptr;
    {
        std::string temp_output = output + "_old";

        pf = fopen(temp_output.c_str(), "w");

        if (nullptr == pf) {
            LOG(ERROR) << "open outfile[" << output.c_str() << "] error.";
            return -1;
        }
    }

    char buff[1024] = {0};
    puck::Request request;
    puck::Response response;
    request.topk = 100;

    response.distance = new float[request.topk];
    response.local_idx = new uint32_t[request.topk];

    for (int i = 0; i < item_count; ++i) {
        request.feature = in_data[i].data();
        ret = searcher->search(&request, &response);

        if (ret != 0) {
            LOG(ERROR) << "search item " << i << " error" << ret;
            break;
        }

        for (int j = 0; j < (int)request.topk; j ++) {
            char* p = buff;
            std::string lable = searcher->get_label(response.local_idx[j]);
            //std::string lable = std::to_string(response.local_idx[j]);
            snprintf(p, 1024, "%s\t%s\t%f", pic_name[i].c_str(),
                     lable.c_str(),
                     response.distance[j]);

            fprintf(pf, "%s\n", buff);
        }
    }

    fclose(pf);

    puck::InsertRequest insert_request;

    for (int i = 0; i < in_data.size(); ++i) {
        insert_request.feature = in_data[i].data();
       
        insert_request.label = pic_name[i];
        int ret = searcher->insert(&insert_request);

        if (ret != 0) {
            LOG(ERROR) << "insert item " << i << " error" << ret;
            return -1;
        }
    }

    {
        std::string temp_output = output + "_new";

        pf = fopen(temp_output.c_str(), "w");

        if (nullptr == pf) {
            LOG(ERROR) << "open outfile[" << output.c_str() << "] error.";
            return -1;
        }
    }

    for (int i = 0; i < item_count; ++i) {
        request.feature = in_data[i].data();

        ret = searcher->search(&request, &response);

        if (ret != 0) {
            LOG(ERROR) << "search item " << i << " error" << ret;
            break;
        }

        for (int j = 0; j < (int)request.topk; j ++) {
            char* p = buff;
            std::string lable = searcher->get_label(response.local_idx[j]);
            //std::string lable = std::to_string(response.local_idx[j]);
            snprintf(p, 1024, "%s\t%s\t%d\t%f", pic_name[i].c_str(),
                     lable.c_str(),
                     response.local_idx[j],
                     response.distance[j]);

            fprintf(pf, "%s\n", buff);
        }
    }

    fclose(pf);

    delete [] response.distance;
    delete [] response.local_idx;

    return 0;
}
