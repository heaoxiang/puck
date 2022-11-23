/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    search_interface.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-26 12:56
 * @brief
 ***********************************************************************/
#include <boost/lexical_cast.hpp>
#include "gflags/puck_gflags.h"
#include "searcher.h"
#include "tinker/tinker_index.h"
#include "puck/puck_index.h"
namespace puck {
DEFINE_bool(is_contsign, false, "type of label is uint64_t");
DEFINE_bool(has_key, true, "using user defined key of point");

int Searcher::init() {
    IndexConf conf = puck::load_index_conf_file();

    if (conf.index_version == 2) { //Tinker
        LOG(NOTICE) << "init index of Tinker";
        _index.reset(new puck::TinkerIndex());
    } else if (conf.index_version == 1 && conf.whether_filter == true) { //PUCK
        LOG(NOTICE) << "init index of Puck";
        _index.reset(new puck::PuckIndex());
    } else if (conf.index_version == 1 && conf.whether_filter == false) {
        _index.reset(new puck::HierarchicalCluster());
        LOG(NOTICE) << "init index of Flat";
    } else {
        LOG(NOTICE) << "init index of Error, Nan type";
        return -1;
    }

    if (read_labels() != 0) {
        LOG(ERROR) << "load labels has error";
        return 1;
    }

    if (_index->init() != 0) {
        LOG(ERROR) << "init index failed";
        return 1;
    }

    return 0;
}

//内置非指针类型转换
template <class FromType, class ToType>
inline ToType my_converted(const FromType& from) {
    ToType to;

    try {
        to = boost::lexical_cast<ToType>(from);
    } catch (const boost::bad_lexical_cast& exception) {
        LOG(FATAL) << "lexical_cast error msg : " << exception.what();
    }

    return to;
}

int Searcher::read_labels() {
    if (FLAGS_has_key == false) {
        return 0;
    }

    //load url file, 数据总数从url file中获得
    LOG(NOTICE) << "start load index file  " << _index->_conf.key_file_name;
    base::Timer all_cost;
    all_cost.start();
    std::ifstream in_url(_index->_conf.key_file_name);
    std::string url_line;

    if (!in_url.good()) {
        LOG(FATAL) << "open index file " << _index->_conf.key_file_name << " error.";
        return -1;
    }

    _is_contsign = FLAGS_is_contsign;

    _labels.clear();
    _contsign.clear();

    while (std::getline(in_url, url_line)) {
        if (_is_contsign) {
            _contsign.push_back(my_converted<std::string, uint64_t>(url_line));
        } else {
            _labels.push_back(url_line);
        }
    }

    uint32_t total_point_count = _is_contsign ? _contsign.size() : _labels.size();
    _index->_conf.total_point_count = total_point_count;
    LOG(NOTICE) << "_conf.total_point_count = " << _index->_conf.total_point_count;
    in_url.close();
    return 0;
}

}