/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    distribute_merge_index.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2021-11-12 13:03
 * @brief
 ***********************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <sys/stat.h>
//#include <boost/algorithm/string.hpp>
//#include <boost/lexical_cast.hpp>
//#include <base/strings/string_split.h>
//#include <base/base64.h>
//#include "base/base64.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "gflags/puck_gflags.h"
#include "hierarchical_cluster/hierarchical_cluster_index.h"
#include "string_split.h"
DEFINE_string(hadoop_ugi, "your ugi", "hadoop_ugi");

// hadoop 计数器
void incrcounter(std::string countername) {
    std::cerr << "reporter:counter:puck_buildindex," << countername << ",1" << std::endl;
}

struct MergeInfo {
    std::string doc_key;
    std::string bs_id;
    std::string cell_id;
    std::pair<float, std::vector<unsigned char>> pq_data;
    std::pair<float, std::vector<unsigned char>> filter_data;
    std::vector<float> init_fea;
    std::string other_info;
};

int parse_quantization_fea(const std::string& input_stream, const uint32_t nsq,
                           std::pair<float, std::vector<unsigned char>>& quantizated) {

    std::vector<std::string> sub_str;
    //boost::split(sub_str, input_stream, boost::is_any_of("|"));
    puck::s_split(input_stream, "|", sub_str);
    if (sub_str.size() < 2) {
        LOG(FATAL) << "parse_quantization_fea error.";
        return -1;
    }
    quantizated.first = std::atof(sub_str[0].c_str());


    std::vector<std::string> fea_str;
    //boost::split(fea_str, sub_str[1], boost::is_any_of(" "));
    puck::s_split(sub_str[1], " ", fea_str);
    if (fea_str.size() != nsq) {
        return -1;
    }

    quantizated.second.resize(nsq);

    for (uint32_t idx = 0; idx < nsq; ++idx) {
        quantizated.second[idx] = (unsigned char)std::atoi(fea_str[idx].c_str());
    }

    return 0;
}

int parse_input_stream(std::string& input_stream,
                       MergeInfo& info,
                       puck::IndexConf& conf) {
    std::vector<std::string> sub_str;
    //boost::split(sub_str, input_stream, boost::is_any_of("\t"));
    puck::s_split(input_stream, "\t", sub_str);
    if (sub_str.size() < 4) {
        LOG(FATAL) << "parse_input_stream error.";
        return -1;
    }
    /// bs_id + cell id
    {
        std::vector<std::string> sub_info;
        //boost::split(sub_info, sub_str[0], boost::is_any_of(" "));
        puck::s_split(sub_str[0], " ", sub_info);
        if (sub_info.size() < 2) {
            return -1;
        }

        info.bs_id = sub_info[0];
        info.cell_id = sub_info[1];
    }
    
    info.doc_key = sub_str[2];

    if (sub_str.size() == 5) {
        info.other_info = sub_str[4];
    }

    auto& build_fea = sub_str[3];

    if (conf.whether_filter) {
        // offVal | 空格分割fea , xxx
        std::vector<std::string> sub_fea_str;
        //boost::split(sub_fea_str, build_fea, boost::is_any_of(","));
        puck::s_split(build_fea, ",", sub_fea_str);
        
        if (sub_fea_str.size() < 2) {
            LOG(FATAL) << "filer fea of parse_input_stream error.";
            return -1;
        }

        if (parse_quantization_fea(sub_fea_str[0], conf.filter_nsq, info.filter_data) != 0) {
            return -2;
        }

        if (conf.whether_pq) {
            if(parse_quantization_fea(sub_fea_str[1], conf.nsq, info.pq_data) != 0) {
                return -2;
            }
        } else {
            std::vector<std::string> fea_str;
            //boost::split(fea_str, sub_fea_str[1], boost::is_any_of(" "));
            puck::s_split(sub_fea_str[1], " ", fea_str);
            if (fea_str.size() != conf.feature_dim) {
                return -2;
            }

            info.init_fea.resize(conf.feature_dim);

            for (uint32_t idx = 0; idx < conf.feature_dim; ++idx) {
                info.init_fea[idx] = std::atof(fea_str[idx].c_str());
            }
        }
    } else if (conf.whether_pq) {
        LOG(FATAL) << "NOT SUPPORT THIS FORMAT.";
        return -1;
    } else {
        std::vector<std::string> fea_str;
        //boost::split(fea_str, build_fea, boost::is_any_of(" "));
        puck::s_split(build_fea, " ", fea_str);
        if (fea_str.size() != conf.feature_dim) {
            return -2;
        }

        info.init_fea.resize(conf.feature_dim);

        for (uint32_t idx = 0; idx < conf.feature_dim; ++idx) {
            info.init_fea[idx] = std::atof(fea_str[idx].c_str());
        }
    }

    return 0;
}

struct IndexFileStruct {
    puck::IndexConf conf;
    std::ofstream* out_cell_assigns;
    std::ofstream* out_all_keys;
    std::ofstream* out_pq_fea_vocab;
    std::ofstream* out_filter_fea_vocab;
    std::ofstream* out_init_fea_vocab;
    
    void init() {
        out_cell_assigns = nullptr;
        out_all_keys = nullptr;
        out_pq_fea_vocab = nullptr;
        out_filter_fea_vocab = nullptr;
        out_init_fea_vocab = nullptr;
    }

    void close() { 
        if (out_cell_assigns != nullptr){
            out_cell_assigns->close();
        }
        
        if (out_all_keys != nullptr){
            out_all_keys->close();
        }
        
        if (out_pq_fea_vocab != nullptr){
            out_pq_fea_vocab->close();
        }
        if (out_filter_fea_vocab != nullptr){
            out_filter_fea_vocab->close();
        }
        if (out_init_fea_vocab != nullptr){
            out_init_fea_vocab->close();
        }
        init();
    }

    void open(std::string& bs_base_path) {
        if (0 != mkdir(bs_base_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            LOG(ERROR) << "hadoop local bshome mkdir failed !"<<bs_base_path;
            exit(1);
        }

        std::string cur_index_path = bs_base_path + "/" + conf.index_path;

        if (0 != mkdir(cur_index_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            LOG(ERROR) << "hadoop local puck_index mkdir failed !";
            exit(2);
        }

        std::string cell_assign_file_name = bs_base_path + "/" + conf.cell_assign_file_name;
        LOG(INFO)<<cell_assign_file_name;
        out_cell_assigns = new std::ofstream(cell_assign_file_name .c_str(), std::ios::binary | std::ios::out);

        std::string label_file_name = bs_base_path + "/" + conf.label_file_name;
        LOG(INFO)<<label_file_name;
        out_all_keys = new std::ofstream(label_file_name.c_str(), std::ios::out);

        if (conf.whether_pq) {
            std::string pq_data_file_name = bs_base_path + "/" + conf.pq_data_file_name;
            LOG(INFO)<<pq_data_file_name;
            out_pq_fea_vocab = new std::ofstream(pq_data_file_name.c_str(), std::ios::binary | std::ios::out);
        } else {
            std::string feature_file_name = bs_base_path + "/" + conf.feature_file_name;
            LOG(INFO)<<feature_file_name;
            out_init_fea_vocab = new std::ofstream(feature_file_name.c_str(), std::ios::binary | std::ios::out);
        }

        if (conf.whether_filter) {
            
            std::string filter_data_file_name = bs_base_path + "/" + conf.filter_data_file_name;
            LOG(INFO)<<filter_data_file_name;
            out_filter_fea_vocab = new std::ofstream(filter_data_file_name.c_str(), std::ios::binary | std::ios::out);
        }
    }
    void write_index(MergeInfo& info) {
        info.doc_key += "\n";
        out_all_keys->write(info.doc_key.c_str(), info.doc_key.length());
        incrcounter("write_all_cnts_succ");

        int cell_id = std::atoi(info.cell_id.c_str());
        out_cell_assigns->write((char*)&cell_id, sizeof(int));
        incrcounter("write_cell_assigns_succ");

        if (conf.whether_filter) {
            out_filter_fea_vocab->write((char*)&info.filter_data.first, sizeof(float));
            out_filter_fea_vocab->write((char*)info.filter_data.second.data(), sizeof(unsigned char) * conf.filter_nsq);
            incrcounter("write_filer_fea_succ");
        }
        if (conf.whether_pq) {
            out_pq_fea_vocab->write((char*)&info.pq_data.first, sizeof(float));
            out_pq_fea_vocab->write((char*)info.pq_data.second.data(), sizeof(unsigned char) * conf.nsq);
            incrcounter("write_pq_fea_succ");
        } else {
            int dim = conf.feature_dim;
            out_init_fea_vocab->write((char*)&dim, sizeof(int));
            out_init_fea_vocab->write((char*)info.init_fea.data(), sizeof(float) * dim);
            incrcounter("write_init_fea_succ");
        }
        
    }
};

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    //get true conf
    puck::IndexConf conf = puck::load_index_conf_file();
    conf.show();

    // 初始化hadoop环境变量
    const char * hadoop_output_dir = getenv("mapred_work_output_dir");
    const char * hadoop_home = getenv("HADOOP_HOME");
    std::stringstream ss;
    ss << hadoop_output_dir;
    std::string local_output = ss.str();

    // 处理hash bsid至reduce不均等，一个reduce可能处理多个bsid的情况，但是一定有序
    std::string lastbsidstr = "";

    LOG(INFO) << "start to process stdin lines";

    std::string ugi = FLAGS_hadoop_ugi;

    IndexFileStruct file_struct;
    file_struct.init();
    file_struct.conf = conf;

    LOG(INFO) << "hadoop_ugi: " << ugi;

    while (1){
        std::string line;
        if (!std::getline(std::cin, line)) {
            LOG(WARNING) << "read to end or empty line";
            break;
        }
        MergeInfo info;

        // 此处暂时不处理重复key的情况
        int parseret = parse_input_stream(line, info, conf);
        if (-1 == parseret) {
            incrcounter("error_line");
            continue;
        }
        if (-2 == parseret) {
            incrcounter("parse_error");
            continue;
        }

        incrcounter("parse_succ");
        auto& bs_id = info.bs_id;
        if (bs_id != lastbsidstr) {
            if (lastbsidstr != "") {
                file_struct.close();

                // 上传文件目录包括fastsame和puckindex
                std::string uploadcomand = "/bin/hadoop fs -D hadoop.job.ugi=" + ugi + " -put ";
                std::stringstream ss;
                ss << hadoop_home;
                ss << uploadcomand;
                ss << lastbsidstr;
                ss << " ";
                ss << hadoop_output_dir;

                LOG(INFO) << ss.str();

                if (system(ss.str().c_str()) != 0) {
                    LOG(ERROR) << "puck index file upload failed !";
                    incrcounter("upload_fail");
                    return -1;
                }
            }

            // 创新新的fp-puck文件对象, fp-fastsame文件对象
            std::string bshome = bs_id;
            file_struct.open(bshome);
        }

        // 上一个bs的索引merge完成，上传至hadoop输出目录
        lastbsidstr = bs_id;
        std::cout << bs_id << "\t" << info.doc_key << std::endl;
        file_struct.write_index(info);
    }
    LOG(INFO) << "finish to process stdin lines";
    // 上一个bs的索引merge完成，上传至hadoop输出目录
    if (lastbsidstr != "") {
        file_struct.close();
        LOG(INFO) << "finish to process stdin lines";
        // 上传文件
        std::string uploadcomand = "/bin/hadoop fs -D hadoop.job.ugi=" + ugi + " -put ";
        std::stringstream ss;
        ss << hadoop_home;
        ss << uploadcomand;
        ss << lastbsidstr;
        ss << " ";
        ss << hadoop_output_dir;
        LOG(INFO)<<ss.str();
        // 计算一下本地生成的包的大小
        std::string check_size = "du -b ";
        std::stringstream sc;
        sc << check_size;
        sc << lastbsidstr;
        LOG(INFO) << sc.str();
        if (system(sc.str().c_str()) != 0) {
            LOG(ERROR) << "check size failed !";
            return -1;
        }

        LOG(INFO) << ss.str();

        if (system(ss.str().c_str()) != 0) {
            LOG(ERROR) << "puck index file upload failed !";
            incrcounter("upload_fail");
            return -1;
        }
    }

    LOG(INFO) << "finish to process stdin lines with upload indexs";

    return 0;
}
