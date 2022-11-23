/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    search_client.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-10-09 15:24
 * @brief
 ***********************************************************************/

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "searcher.h"

uint32_t s_split(const std::string& input_stream, const std::string& delim_1, std::vector<std::string>& ret) {
    boost::split(ret, input_stream, boost::is_any_of(delim_1));
    int true_size = ret.size() - 1;

    while (true_size >= 0) {
        if (ret[true_size].length() < 1) {
            --true_size;
        } else {
            break;
        }
    }

    ret.resize(true_size + 1);
    return ret.size();
}

int read_feature_data(std::string& input_file, const u_int32_t dim, std::vector<std::string>& pic_name,
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

        if (s_split(line, "\t", split_str) < 2) {
            LOG(ERROR) << "id:" << pic_name.size() << " get name error.";
            ret = -1;
            break;
        }

        pic_name.push_back(split_str[0]);
        std::string feature_str = split_str[1];

        if (s_split(feature_str, " ", split_str) != dim) {
            LOG(ERROR) << "id:" << doc_feature.size() << " get feature error.";
            ret = -2;
            break;
        }

        std::vector<float> cur_feature;

        for (u_int32_t idx = 0; idx < dim; ++idx) {
            cur_feature.push_back(std::atof(split_str[idx].c_str()));
        }

        doc_feature.push_back(cur_feature);
    }

    fin.close();
    LOG(NOTICE) << "total query cnt = " << pic_name.size();
    return ret;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    //1. load index
    std::unique_ptr<puck::Searcher> searcher(new puck::Searcher());

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
    const int dim = searcher->get_conf().feature_dim;
    std::string input(argv[1]);
    std::string output(argv[2]);
    std::vector<std::vector<float> > in_data;
    std::vector<std::string> pic_name;

    ret = read_feature_data(input, dim, pic_name, in_data);

    if (ret != 0) {
        LOG(ERROR) << "read_feature_data error:" << ret;
        return -4;
    } else {
        LOG(NOTICE) << "read_feature_data item:" << in_data.size();
    }

    //3. search
    const int item_count = in_data.size();
    FILE* pf = fopen(output.c_str(), "w");

    if (nullptr == pf) {
        LOG(ERROR) << "open outfile[" << output.c_str() << "] error.";
        return -1;
    }

    char buff[1024] = {0};
    std::unique_ptr<float[]> distance(new float[searcher->get_conf().topk]);
    std::unique_ptr<uint32_t[]> labels(new uint32_t[searcher->get_conf().topk]);

    for (int i = 0; i < item_count; ++i) {
        ret = searcher->search(in_data[i].data(), searcher->get_conf().topk, distance.get(), labels.get());

        if (ret != 0) {
            LOG(ERROR) << "search item " << i << " error" << ret;
            break;
        }

        for (int j = 0; j < (int)searcher->get_conf().topk; j ++) {
            char* p = buff;
            std::string lable = searcher->get_label(labels[j]);
            snprintf(p, 1024, "%s\t%s\t%f", pic_name[i].c_str(),
                     //labels[j],
                     lable.c_str(),
                     distance[j]);

            fprintf(pf, "%s\n", buff);
        }
    }

    fclose(pf);

    return 0;
}