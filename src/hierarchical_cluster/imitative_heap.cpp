/***********************************************************************
 * Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
 * @file    imitative_heap.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2019-07-12 14:54
 * @brief
 ***********************************************************************/
#include <algorithm>
#include "imitative_heap.h"
#include "hierarchical_cluster/hierarchical_cluster.h"
namespace puck {

ImitativeHeap::ImitativeHeap(const uint32_t neighbors_count, DistanceInfo& cell_distance) :
    _cell_distance(cell_distance) {
    _top_idx = 0;
    _heap_size = _cell_distance.size();
    _contain_doc_cnt = 0;
    _neighbors_count = neighbors_count;
    _pivot = std::sqrt(std::numeric_limits<float>::max());
    _min_distance = _pivot;
}

uint32_t ImitativeHeap::push(const float distance, FineCluster* cell) {
    //当前距离大于堆顶，返回0
    if (_pivot < distance) {
        return 0;
    }

    _cell_distance[_top_idx] = {distance, cell};
    _min_distance = std::min(_min_distance, distance);
    ++_top_idx;
    _contain_doc_cnt += cell->get_point_cnt();

    //新入堆的doc个数>=_neighbors_count * 1.4 或 有越界风险,进行堆调整
    if (_contain_doc_cnt >= _neighbors_count * 1.4
            || _top_idx >= _heap_size) {
        //尽量快的找到合适的堆顶
        _top_idx = imitative_heap_partition();
    }
    //入队1个cell
    return 1;
}

uint32_t ImitativeHeap::imitative_heap_partition() {
    if (_contain_doc_cnt < _neighbors_count) {
        return _top_idx;
    }

    auto first = _cell_distance.begin();
    auto last = _cell_distance.begin() + _top_idx;
    float pivot = _min_distance + (_neighbors_count * 1.0 / _contain_doc_cnt) * (_pivot - _min_distance);

    auto middle = std::partition(first, last, [pivot](const std::pair<float, FineCluster*>& a) {
        return a.first <= pivot;
    });

    //LOG(NOTICE)<<"imitative_heap_partition "<<_pivot;
    std::sort(middle, last);
    uint32_t tail_idx = _top_idx - 1;
    uint32_t tail_min = std::distance(first, middle);

    while (tail_idx > tail_min) {
        auto cur_doc_cnt = _cell_distance[tail_idx].second->get_point_cnt();

        if (_contain_doc_cnt - cur_doc_cnt >= _neighbors_count) {
            _contain_doc_cnt -= cur_doc_cnt;
            --tail_idx;
        } else {
            _pivot = _cell_distance[tail_idx].first;
            return tail_idx + 1;
        }
    }

    _pivot = pivot;
    return std::distance(first, middle);

}

}
