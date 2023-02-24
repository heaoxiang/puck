/***************************************************************************
 *
 * Copyright (c) 2017 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/

/**
 * @file   hnsw_distfunc_opt.cc
 * @author huangben(huangben@baidu.com)
 * @author yinjie06(yinjie06@baidu.com)
 * @date   2017年11月05日 星期一 20时17分38秒
 * @brief
 *
 **/

#include "puck/tinker/method/hnsw.h"
#include "puck/tinker/method/hnsw_distfunc_opt_impl_inline.h"
//#include "knnquery.h"
//#include "rangequery.h"

#include "puck/tinker/portable_prefetch.h"
#include "puck/tinker/space.h"

//#include "sort_arr_bi.h"
#define MERGE_BUFFER_ALGO_SWITCH_THRESHOLD 100

#include <algorithm> // std::min
#include <limits>
#include <vector>

//#define DIST_CALC
namespace similarity {

template <typename dist_t>
void
Hnsw<dist_t>::SearchOld_level0(const float* pVectq, const size_t feature_dim, const int topk,
                               const std::vector<int>& enterpointIds, std::priority_queue<std::pair<float, int>>& closestDistQueuei) {

    TMP_RES_ARRAY(TmpRes);
    size_t qty = feature_dim;
    VisitedList* vl = visitedlistpool->getFreeVisitedList();
    vl_type* massVisited = vl->mass;
    vl_type currentV = vl->curV;

    std::priority_queue<EvaluatedMSWNodeInt<dist_t>>
            candidateQueuei; // the set of elements which we can use to evaluate

    //priority_queue<EvaluatedMSWNodeInt<dist_t>> closestDistQueuei; // The set of closest found elements
    int distance_computations = 0;

    for (auto& enterpointId : enterpointIds) {

        int curNodeNum = enterpointId;
        dist_t curdist = (fstdistfunc_(
                              pVectq, (float*)(data_level0_memory_ + curNodeNum * memoryPerObject_ + offsetData_ + objectDataOffset_), qty, TmpRes));
        //LOG(INFO)<<curNodeNum<<"\t"<<curdist;
        ++distance_computations;
        // EvaluatedMSWNodeInt<dist_t> evi(curdist, curNodeNum);
        //closestDistQueuei.emplace(curdist, curNodeNum);
        massVisited[curNodeNum] = currentV;
        if (closestDistQueuei.size() < (size_t)topk || curdist < closestDistQueuei.top().first) {
            candidateQueuei.emplace(-curdist, curNodeNum);
            closestDistQueuei.emplace(curdist, curNodeNum);
            if (closestDistQueuei.size() > (size_t)topk){
                closestDistQueuei.pop();
            }
        }
    }

    while (!candidateQueuei.empty()) {
        EvaluatedMSWNodeInt<dist_t> currEv = candidateQueuei.top(); // This one was already compared to the query

        if (closestDistQueuei.size() >= (size_t)topk && (-currEv.getDistance()) > closestDistQueuei.top().first) {
            //LOG(INFO)<<closestDistQueuei.size()<<" break";
            break;
        }
        /*
        if (!(massVisited[currEv.element] == currentV)) {
            if (closestDistQueuei.size() < (size_t)topk || closestDistQueuei.top().first > -currEv.getDistance()) {
                closestDistQueuei.emplace(-currEv.getDistance(), currEv.element);

                if (closestDistQueuei.size() > (size_t)topk) {
                    closestDistQueuei.pop();
                }
            }

            massVisited[currEv.element] = currentV;
        }*/

        candidateQueuei.pop();
        int curNodeNum = currEv.element;
        int* data = (int*)(data_level0_memory_ + curNodeNum * memoryPerObject_ + offsetLevel0_);
        int size = *data;
        PREFETCH((char*)(massVisited + * (data + 1)), _MM_HINT_T0);
        PREFETCH((char*)(massVisited + * (data + 1) + 64), _MM_HINT_T0);
        PREFETCH(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
        PREFETCH((char*)(data + 2), _MM_HINT_T0);

        for (int j = 1; j <= size; j++) {
            int tnum = *(data + j);
            PREFETCH((char*)(massVisited + * (data + j + 1)), _MM_HINT_T0);
            PREFETCH(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);

            if (!(massVisited[tnum] == currentV)) {
                ++distance_computations;
                massVisited[tnum] = currentV;
                char* currObj1 = (data_level0_memory_ + tnum * memoryPerObject_ + offsetData_);
                dist_t d = (fstdistfunc_(pVectq, (float*)(currObj1 + objectDataOffset_), qty, TmpRes));

                if (closestDistQueuei.top().first > d || closestDistQueuei.size() < (size_t)topk) {
                    candidateQueuei.emplace(-d, tnum);
                    //PREFETCH(data_level0_memory_ + candidateQueuei.top().element * memoryPerObject_ + offsetLevel0_,
                    //             _MM_HINT_T0);
                    // query->CheckAndAddToResult(d, new Object(currObj1));
                    closestDistQueuei.emplace(d, tnum);

                    if (closestDistQueuei.size() > (size_t)topk) {
                        closestDistQueuei.pop();
                    }
                }
            }
        }
    }

    //LOG(INFO)<<distance_computations<<" CMP TIMES";

    visitedlistpool->releaseVisitedList(vl);
}


template class Hnsw<float>;
template class Hnsw<int>;
}
