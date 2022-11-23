/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    utils_simd.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2021-08-23 17:49
 * @brief
 ***********************************************************************/
#ifndef BAIDU_MMS_GRAPH_GNOIMI_UTILS_SIMD_H
#define BAIDU_MMS_GRAPH_GNOIMI_UTILS_SIMD_H
#include <cmath>
#include <cassert>
#ifdef __SSE__
#include <immintrin.h>
#endif

#ifdef __aarch64__
#include  <arm_neon.h>
#endif
namespace puck {

/// Squared L2 distance between two vectors
float fvec_L2sqr(
    const float* x,
    const float* y,
    size_t d);

/* same without SSE */
inline float fvec_L2sqr_ref(const float* x,
                            const float* y,
                            size_t d) {
    float res = 0;

    for (size_t i = 0; i < d; i++) {
        const float tmp = x[i] - y[i];
        res += tmp * tmp;
    }

    return res;
}
/*
#ifdef __SSE__

inline void L2SqrSIMD4Ext(const float* pVect1, const float* pVect2, __m128& sum) {
    __m128 diff, v1, v2;

    v1 = _mm_loadu_ps(pVect1);
    v2 = _mm_loadu_ps(pVect2);
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
};

#endif
*/

}
#endif
