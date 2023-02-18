/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    py_puck_api_wrapper.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2021-08-18 14:30
 * @brief
 ***********************************************************************/
#include <gflags/gflags.h>
#include "index.h"
#include "py_api_wrapper.h"
#include "gflags/puck_gflags.h"
#include "puck/puck_index.h"
#include "tinker/tinker_index.h"
namespace py_puck_api {

void update_gflag(const char* gflag_key, const char* gflag_val) {
    google::SetCommandLineOption(gflag_key, gflag_val);
}

void PySearcher::show() {

    std::cout << "1\n";
}
PySearcher::PySearcher() {};

int PySearcher::build(uint32_t total_cnt) {
    std::cout << "start to train\n";

    puck::IndexConf conf;

    std::unique_ptr<puck::HierarchicalClusterIndex> cluster;

    if (conf.index_type == puck::IndexType::TINKER) { //Tinker
        LOG(INFO) << "init index of Tinker";
        _index.reset(new puck::TinkerIndex());
    } else if (conf.index_type == puck::IndexType::PUCK) { //PUCK
        LOG(INFO) << "init index of Puck";
        _index.reset(new puck::PuckIndex());
    } else if (conf.index_type == puck::IndexType::HIERARCHICAL_CLUSTER) {
        _index.reset(new puck::HierarchicalClusterIndex());
        LOG(INFO) << "init index of Flat";
    } else {
        LOG(INFO) << "init index of Error, Nan type";
        return -1;
    }

    cluster->train();

    std::cout << "train Suc.\n";
    std::cout << "start to build\n";

    cluster->build();

    return 0;
}

int PySearcher::init() {

    puck::IndexConf conf = puck::load_index_conf_file();

    if (conf.index_type == puck::IndexType::TINKER) { //Tinker
        LOG(INFO) << "init index of Tinker";
        _index.reset(new puck::TinkerIndex());
    } else if (conf.index_type == puck::IndexType::PUCK) { //PUCK
        LOG(INFO) << "init index of Puck";
        _index.reset(new puck::PuckIndex());
    } else if (conf.index_type == puck::IndexType::HIERARCHICAL_CLUSTER) {
        _index.reset(new puck::HierarchicalClusterIndex());
        LOG(INFO) << "init index of Flat";
    } else {
        LOG(INFO) << "init index of Error, Nan type";
        return -1;
    }

    int ret = _index->init();
    _dim = conf.feature_dim;

    if (conf.ip2cos) {
        --_dim;
    }

    return ret;
}

template <class Function>
inline void ParallelFor(size_t start, size_t end, size_t numThreads, Function fn) {
    std::vector<std::thread> threads;
    std::atomic<size_t> current(start);

    std::exception_ptr lastException = nullptr;
    std::mutex lastExceptMutex;

    for (size_t threadId = 0; threadId < numThreads; ++threadId) {
        threads.push_back(std::thread([&, threadId] {
            while (true) {
                size_t id = current.fetch_add(1);

                if ((id >= end)) {
                    break;
                }

                try {
                    fn(id, threadId);
                } catch (...) {
                    std::unique_lock<std::mutex> lastExcepLock(lastExceptMutex);
                    lastException = std::current_exception();
                    /*
                    * This will work even when current is the largest value that
                    * size_t can fit, because fetch_add returns the previous value
                    * before the increment (what will result in overflow
                    * and produce 0 instead of current + 1).
                    */
                    current = end;
                    break;
                }
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (lastException) {
        std::rethrow_exception(lastException);
    }
}

int PySearcher::search(uint32_t n, const float* query_fea, const uint32_t topk, float* distance,
                       uint32_t* labels) {

    ParallelFor(0, n, puck::FLAGS_threads_count, [&](int id, int threadId) {
        (void)threadId;
        puck::Request request;
        puck::Response response;
        request.topk = topk;
        request.feature = query_fea + id * _dim;

        response.distance = distance + id * topk;
        response.local_idx = labels + id * topk;
        _index->search(&request, &response);
    });

    return 0;
}

void PySearcher::update_params(uint32_t topk, uint32_t search_cells, uint32_t neighbors,
                               uint32_t filter_topk) {
    update_gflag("topk", std::to_string(topk).c_str());
    update_gflag("search_coarse_count", std::to_string(search_cells).c_str());
    update_gflag("neighbors_count", std::to_string(neighbors).c_str());
    update_gflag("filter_topk", std::to_string(filter_topk).c_str());
    update_gflag("tinker_search_range", std::to_string(neighbors).c_str());
    init();
}

PySearcher::~PySearcher() {};
};//namespace py_puck_api
