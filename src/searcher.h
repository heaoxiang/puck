/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    search_interface.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-26 12:56
 * @brief
 ***********************************************************************/
#ifndef BAIDU_MMS_GRAPH_GNOIMI_SEARCH_INTERFACE_H
#define BAIDU_MMS_GRAPH_GNOIMI_SEARCH_INTERFACE_H

#include "hierarchical_cluster/hierarchical_cluster.h"

namespace puck {

class Searcher {
public:
    int init();

    /*
     * @brief 检索最近的topk个样本
     * @@param [in] request : request
     * @@param [out] response : response
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int search(Request* request, Response* response) {
        return _index->search(request, response);
    }
    const IndexConf get_conf() {
        return _index->_conf;
    }
    /*
     * @brief local idx 获取对应的lable
     **/
    inline std::string get_label(uint32_t label_id) {
        if (_is_contsign) {
            return std::to_string(_contsign[label_id]);
        }

        return _labels[label_id];
    }

private:
    int read_labels();
private:
    bool _is_contsign;
    std::unique_ptr<HierarchicalCluster> _index;
    std::vector<std::string> _labels;
    std::vector<uint64_t> _contsign;
};

}//gnoimi
#endif

