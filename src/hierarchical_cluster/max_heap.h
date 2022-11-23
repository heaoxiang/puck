/***********************************************************************
 * Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
 * @file    hierarchical_cluster/max_heap.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2019-08-20 10:43
 * @brief
 ***********************************************************************/
#ifndef BAIDU_MMS_GRAPH_GNOIMI_GNOIMI_HEAP_H
#define BAIDU_MMS_GRAPH_GNOIMI_GNOIMI_HEAP_H

#include <cstdio>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

namespace puck {


class MaxHeap {
public:
    MaxHeap(const uint32_t size, float* val, uint32_t* tag);
    ~MaxHeap() {}
    /*
     * @brief 小于堆顶元素时，需要更新堆
     * @@param [in] new_val : 距离
     * @@param [in] new_tag : 标记
     **/
    void max_heap_update(const float new_val, const uint32_t new_tag);
    /*
     * @brief 排序
     **/
    void reorder();
    /*
     * @brief 获取堆内元素个数
     **/
    uint32_t get_heap_size() {
        return _heap_size - _default_point_cnt;
    }
    /*
     * @brief 获取堆顶val的指针
     **/
    float* get_top_addr() const {
        return _heap_val + 1;
    }

private:
    MaxHeap();
    /*
     * @brief 在某个范围内查找合适的位置，插入新的值
     * @@param [in] heap_size : 堆的大小
     * @@param [in] father_idx : 开始查找的节点位置
     * @@param [in] new_val : 距离
     * @@param [in] new_tag : 标记
     **/
    void insert(uint32_t heap_size, uint32_t father_idx, float  new_val, uint32_t new_tag);

    void pop_top(uint32_t heap_size);
private:
    float* _heap_val;
    uint32_t* _heap_tag;
    uint32_t _default_point_cnt;
    const uint32_t _heap_size;
};

}
#endif

