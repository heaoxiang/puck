/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    puck_index.cpp
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-27 15:56
 * @brief
 ***********************************************************************/

#include <thread>
#include <omp.h>
#include <functional>
#include <malloc.h>
#include <numeric>
#include <unistd.h>
#include <memory>
#include <glog/logging.h>
#include "puck/gflags/puck_gflags.h"
#include "puck/hierarchical_cluster/max_heap.h"
#include "puck/search_context.h"

//#include "imitative_heap.h"
#include "puck/tinker/method/hnsw_distfunc_opt_impl_inline.h"
#include "puck/puck/puck_index.h"
namespace puck {

DEFINE_int32(pq_train_points_count, 1000000, "used for puck train pq codebooks");
DEFINE_string(train_pq_file_name, "mid-data/train_pq.dat", "random sampling for puck train pq codebooks");

PuckIndex::PuckIndex() {
    _conf.index_type = IndexType::PUCK;
}

PuckIndex::~PuckIndex() {

}

void write_sequential_format(const char* file_name, const void* wrt_vocab, const size_t vocab_len) {
    std::ofstream out_fvec_init;
    out_fvec_init.open(file_name, std::ios::binary | std::ios::out);

    out_fvec_init.write((char*)wrt_vocab, vocab_len);
    out_fvec_init.close();
}

int PuckIndex::save_coodbooks() const {
    this->HierarchicalClusterIndex::save_coodbooks();
    LOG(INFO) << "PuckIndex save coodbooks";
    std::vector<std::thread> writers;

    if (_conf.whether_pq) {
        //写pq codebook文件
        auto& cur_params = _pq_quantization->get_quantization_params();
        std::thread write_pq_codebook_data(write_fvec_format, _conf.pq_codebook_file_name.c_str(),
                                           _pq_quantization->get_coodbooks(), cur_params.nsq * cur_params.ks, cur_params.lsq);
        writers.push_back(std::move(write_pq_codebook_data));
    }

    auto& cur_params = _filter_quantization->get_quantization_params();
    LOG(INFO) << cur_params.dim << " " << cur_params.lsq << " " << cur_params.nsq << " " << cur_params.ks;
    std::thread write_filer_codebook_data(write_fvec_format, _conf.filter_codebook_file_name.c_str(),
                                          _filter_quantization->get_coodbooks(), cur_params.nsq * cur_params.ks, cur_params.lsq);
    writers.push_back(std::move(write_filer_codebook_data));

    for (auto& t : writers) {
        t.join();
    }

    return 0;
}

int PuckIndex::save_index() {
    //this->HierarchicalClusterIndex::save_index();
    std::vector<std::thread> writers;

    if (_conf.whether_pq) {
        LOG(INFO) << "save_index pq";
        std::thread write_pq_assign_data(write_sequential_format, _conf.pq_data_file_name.c_str(),
                                         _pq_quantization->get_quantized_feature(0),
                                         _conf.total_point_count * _pq_quantization->get_per_fea_len());
        writers.push_back(std::move(write_pq_assign_data));
    }

    LOG(INFO) << "save_index filer";
    std::thread write_filer_assign_data(write_sequential_format, _conf.filter_data_file_name.c_str(),
                                        _filter_quantization->get_quantized_feature(0),
                                        _conf.total_point_count * _filter_quantization->get_per_fea_len());
    writers.push_back(std::move(write_filer_assign_data));

    for (auto& t : writers) {
        t.join();
    }

    return 0;
}

int PuckIndex::read_coodbooks() {
    LOG(INFO) << "PuckIndex read_coodbooks Start.";

    if (this->HierarchicalClusterIndex::read_coodbooks() != 0) {
        LOG(ERROR) << "read HierarchicalClusterIndex codebook fail";
        return -1;
    }

    if (_filter_quantization->load_codebooks(_conf.filter_codebook_file_name) != 0) {
        LOG(ERROR) << "read filter quantization codebook fail";
        return -2;
    }

    //PQ量化的时候
    if (_conf.whether_pq) {
        if (_pq_quantization->load_codebooks(_conf.pq_codebook_file_name) != 0) {
            LOG(ERROR) << "read pq quantization codebook fail";
            return -2;
        }
    }

    LOG(INFO) << "PuckIndex read_coodbooks Suc.";
    return 0;
}

int PuckIndex::read_feature_index(uint32_t* local_to_memory_idx) {
    if (_conf.whether_filter == false) {
        return -1;
    }

    if (_filter_quantization->load(_conf.filter_codebook_file_name, _conf.filter_data_file_name,
                                   local_to_memory_idx) != 0) {
        LOG(ERROR) << "read_feature_index filter quantization Error.";
        return -1;
    }

    LOG(INFO) << "read_feature_index filter quantization Suc.";

    if (_conf.whether_pq) {
        if (_pq_quantization->load(_conf.pq_codebook_file_name, _conf.pq_data_file_name,
                                   local_to_memory_idx) != 0) {
            LOG(ERROR) << "read_feature_index PQ quantization Error.";
            return -1;
        }

        LOG(INFO) << "read_feature_index PQ quantization Suc.";
    } else {
        if (this->HierarchicalClusterIndex::read_feature_index(local_to_memory_idx) != 0) {
            LOG(ERROR) << "read_feature_index HierarchicalClusterIndex Error.";
            return -1;
        }

        LOG(INFO) << "read_feature_index HierarchicalClusterIndex Suc";
    }

    LOG(INFO) << "PuckIndex::read_quantization_index Suc.";
    return 0;
}

int PuckIndex::convert_local_to_memory_idx(uint32_t* cell_start_memory_idx, uint32_t* local_to_memory_idx) {
    LOG(INFO) << "convert_local_to_memory_idx";
    std::unique_ptr<uint32_t[]> cell_assign(new uint32_t[_conf.total_point_count]);

    if (load_cell_assign(cell_assign.get()) != 0) {
        LOG(INFO) << "load_cell_assign has error.";
        return -1;
    }

    std::unique_ptr<Quantization> filter_quantization;

    QuantizationParams q_param;
    q_param.init(_conf, true);
    filter_quantization.reset(new Quantization(q_param, _conf.total_point_count));

    if (filter_quantization->load(_conf.filter_codebook_file_name, _conf.filter_data_file_name) != 0) {
        LOG(ERROR) << "load " << _conf.filter_codebook_file_name << " error.";
        return -1;
    }

    typedef std::pair<uint32_t, std::pair<float, uint32_t> > MemoryOrder;

    std::vector<MemoryOrder> doc_reorder(_conf.total_point_count);

    for (uint32_t i = 0; i < _conf.total_point_count; ++i) {
        doc_reorder[i].first = cell_assign[i];

        if (cell_assign[i] >= _conf.coarse_cluster_count * _conf.fine_cluster_count) {
            LOG(ERROR) << i << " " << cell_assign[i];
            return -1;
        }

        doc_reorder[i].second.first = 0;
        doc_reorder[i].second.second = i;

        if (filter_quantization) {
            float* offset = (float*)filter_quantization->get_quantized_feature(i);
            doc_reorder[i].second.first = *offset;
        }
    }

    std::stable_sort(doc_reorder.begin(), doc_reorder.end());

    uint32_t cellcount = 0;
    cell_start_memory_idx[cellcount] = 0;

    for (uint32_t i = 0; i < _conf.total_point_count; ++i) {
        uint32_t local_idx = doc_reorder[i].second.second;
        local_to_memory_idx[local_idx] = i;
        _memory_to_local[i] = local_idx;

        while (cellcount + 1 <= doc_reorder[i].first) {
            ++cellcount;
            cell_start_memory_idx[cellcount] = i;
        }
    }

    for (uint32_t i = 0; i < _conf.fine_cluster_count * _conf.coarse_cluster_count; ++i) {
        int start_doc_id = cell_start_memory_idx[i];
        int end_doc_id = cell_start_memory_idx[i + 1];

        int coarse_id = i / _conf.fine_cluster_count;
        int fine_id = i % _conf.fine_cluster_count;

        if (start_doc_id > end_doc_id) {
            LOG(INFO) << "start_doc_id = " << start_doc_id << ", end_doc_id = " << end_doc_id;
            LOG(INFO) << "load index error";
            return -1;
        }

        FineCluster& cur_fine_cluster = _coarse_clusters[coarse_id].fine_cell_list[fine_id];
        cur_fine_cluster.memory_idx_start = start_doc_id;

        if (start_doc_id == end_doc_id) {
            cur_fine_cluster.stationary_cell_dist = std::sqrt(std::numeric_limits<float>::max());
        }

        _coarse_clusters[coarse_id].min_dist_offset = std::min(_coarse_clusters[coarse_id].min_dist_offset,
                cur_fine_cluster.stationary_cell_dist);
    }

    LOG(INFO) << "convert_local_to_memory_idx Suc.";
    return 0;
}

int PuckIndex::init_model_memory() {
    this->HierarchicalClusterIndex::init_model_memory();

    QuantizationParams q_param;

    if (q_param.init(_conf, true) != 0) {
        LOG(ERROR) << "QuantizationParams of filter has Error.";
        return -1;
    }

    _filter_quantization.reset(new Quantization(q_param, _conf.total_point_count));

    //PQ量化的时候
    if (_conf.whether_pq) {
        QuantizationParams q_param;

        if (q_param.init(_conf, false) != 0) {
            LOG(ERROR) << "QuantizationParams of PQ has Error.";
            return 1;
        }

        _pq_quantization.reset(new Quantization(q_param, _conf.total_point_count));

    }

    return 0;
}

int PuckIndex::compute_quantized_distance(SearchContext* context, const int cell_idx,
        const float* pq_dist_table, MaxHeap& result_heap) {
    SearchCellData& search_cell_data = context->get_search_cell_data();
    const FineCluster* cur_fine_cluster = search_cell_data.cell_distance[cell_idx].second.first;

    float* result_distance = result_heap.get_top_addr();

    ///docinfo存储的量化特征对应的参数
    auto& quantization_params = _filter_quantization->get_quantization_params();

    uint32_t* query_sorted_tag = context->get_search_doc_data().query_sorted_tag;
    auto doc_cnt = cur_fine_cluster->get_point_cnt();

    for (uint32_t i = 0; i < doc_cnt; ++i) {

        const unsigned char* feature = _filter_quantization->get_quantized_feature(
                                           cur_fine_cluster->memory_idx_start + i);
        float temp_dist = 2.0 * search_cell_data.cell_distance[cell_idx].first + ((float*)feature)[0];

        if (temp_dist >= result_distance[0]) {
            break;
        }

        const unsigned char* pq_feature = (unsigned char*)feature + _filter_quantization->get_fea_offset();

        for (uint32_t m = 0; m < (uint32_t)quantization_params.nsq; ++m) {
            uint32_t idx = query_sorted_tag[m];
            temp_dist += (pq_dist_table + idx * quantization_params.ks)[pq_feature[idx]];

            //当PQ子空间累计距离已经大于当前最大值，不再计算
            if (temp_dist > result_distance[0]) {
                break;
            }
        }

        if (temp_dist < result_distance[0]) {
            result_heap.max_heap_update(temp_dist, cur_fine_cluster->memory_idx_start + i);
        }
    }

    return doc_cnt;
}

int PuckIndex::filter_topN_docs(SearchContext* context, const float* feature, const int search_cell_cnt,
                                MaxHeap& result_heap) {
    SearchDocData& search_doc_data = context->get_search_doc_data();

    float* result_distance = search_doc_data.result_distance;
    uint32_t* result_tag = search_doc_data.result_tag;
    MaxHeap filter_heap(_conf.filter_topk, result_distance, result_tag);
    {

        uint32_t* query_sorted_tag = search_doc_data.query_sorted_tag;
        float* query_sorted_dist = search_doc_data.query_sorted_dist;
        //初始化最大堆。
        auto& params = _filter_quantization->get_quantization_params();

        MaxHeap sorted_heap(params.nsq, query_sorted_dist, query_sorted_tag);

        for (uint32_t i = 0; i < (uint32_t)params.nsq; ++i) {
            float temp_val = 0;

            for (uint32_t m = 0; m < (uint32_t)params.lsq; ++m) {
                temp_val += -fabs(feature[i * params.lsq + m]);
            }

            sorted_heap.max_heap_update(temp_val, i);
        }

        sorted_heap.reorder();
    }

    uint32_t found = 0;

    float* pq_dist_table = search_doc_data.pq_dist_table;
    auto* cur_quantization = _filter_quantization.get();

    cur_quantization->get_dist_table(feature, pq_dist_table);

    for (int idx = 0; idx < search_cell_cnt && found < _conf.neighbors_count; idx++) {
        found += compute_quantized_distance(context, idx, pq_dist_table, filter_heap);
    }

    if (filter_heap.get_heap_size() < _conf.filter_topk) {
        filter_heap.reorder();
    }

    return rank_topN_docs(context, feature, filter_heap.get_heap_size(), result_heap);
}

int PuckIndex::rank_topN_docs(SearchContext* context, const float* feature, const uint32_t filter_topk,
                              MaxHeap& result_heap) {

    //LOG(INFO)<<"PuckIndex::rank_topN_docs";
    SearchDocData& search_doc_data = context->get_search_doc_data();

    float* result_distance = search_doc_data.result_distance;
    uint32_t* result_tag = search_doc_data.result_tag;

    float query_norm = cblas_sdot(_conf.feature_dim, feature, 1, feature, 1);
    ////真正会返回的结果存储的位置
    float* true_result_distance = result_heap.get_top_addr();

    if (_conf.whether_pq) {
        const Quantization* pq_quantization = _pq_quantization.get();

        float* pq_dist_table = context->get_search_doc_data().pq_dist_table;
        pq_quantization->get_dist_table(feature, pq_dist_table);

        for (uint32_t idx = 0; idx < filter_topk; ++idx) {
            int point_id = _memory_to_local[result_tag[idx]];
            //int point_id = _memory_to_local[filer_vect[idx].second];
            const unsigned char* pq_feature = (unsigned char*)pq_quantization->get_quantized_feature(result_tag[idx]);

            float temp_dist = result_distance[idx] - query_norm + ((float*)pq_feature)[0];

            pq_feature += pq_quantization->get_fea_offset();

            for (uint32_t m = 0; m < (uint32_t)pq_quantization->get_quantization_params().nsq
                    && temp_dist < true_result_distance[0]; ++m) {

                temp_dist += (pq_dist_table + m * pq_quantization->get_quantization_params().ks)[pq_feature[m]];
            }

            if (temp_dist < true_result_distance[0]) {
                result_heap.max_heap_update(temp_dist, point_id);
            }

        }
    } else {
        size_t qty_16 = 16;
        _mm_prefetch((char*)(feature), _MM_HINT_T0);

        for (uint32_t idx = 0; idx < filter_topk; ++idx) {
            uint32_t m = 0;
            float PORTABLE_ALIGN32 TmpRes[8];
            const float* exhaustive_feature = _all_feature + (uint64_t)result_tag[idx] * _conf.feature_dim;
            float temp_dist = 0;

            for (; (m + qty_16) <= _conf.feature_dim && temp_dist < true_result_distance[0];) {
                temp_dist += similarity::L2Sqr16Ext(exhaustive_feature + m, feature + m, qty_16, TmpRes);
                m += qty_16;
            }

            if (temp_dist < true_result_distance[0]) {
                size_t left_dim = _conf.feature_dim - m;

                if (left_dim > 0) {
                    temp_dist += similarity::L2SqrExt(exhaustive_feature + m, feature + m, left_dim, TmpRes);
                }

                result_heap.max_heap_update(temp_dist, _memory_to_local[result_tag[idx]]);
            }
        }
    }

    result_heap.reorder();
    return 0;
}

int PuckIndex::search(const Request* request, Response* response) {
    if (request->topk > _conf.topk || request->feature == nullptr) {
        LOG(ERROR) << "topk should <= topk, topk = " << _conf.topk << ", or feature is nullptr";
        return -1;
    }

    DataHandler<SearchContext> context(_context_pool);

    if (0 != context->reset(_conf)) {
        return -1;
    }

    const float* feature = normalization(context.get(), request->feature);
    if (feature == nullptr){
        return -1;
    }
    //输出query与一级聚类中心的top-search-cell个ID和距离
    int ret = search_nearest_coarse_cluster(context.get(), feature,
                                            _conf.search_coarse_count);//, coarse_distance, coarse_tag);

    if (ret != 0) {
        LOG(INFO) << "search_nearest_coarse_cluster error " << ret;
        return ret;
    }

    //计算query与二级聚类中心的距离并排序
    int search_cell_cnt = search_nearest_fine_cluster(context.get(), feature);//, coarse_tag, coarse_distance);

    MaxHeap result_heap(request->topk, response->distance, response->local_idx);

    ret = filter_topN_docs(context.get(), feature, search_cell_cnt, result_heap);
    if(ret == 0){
        response->result_num = result_heap.get_heap_size();
    }
    return ret;
}

int PuckIndex::puck_assign(const ThreadParams& thread_params, uint32_t* cell_assign) const {
    base::Timer tm_cost;
    tm_cost.start();

    std::unique_ptr<float[]>  chunk_points(new float[FLAGS_thread_chunk_size * _conf.feature_dim]);
    std::unique_ptr<int[]>  pq_assign(new int[FLAGS_thread_chunk_size]);
    std::unique_ptr<float[]>  pq_distance(new float[FLAGS_thread_chunk_size]);
    int coarse_assign  = -1;
    int fine_assign = -1;
    std::vector<Quantization*> quantizations;

    if (_conf.whether_filter) {
        quantizations.push_back(_filter_quantization.get());
    }

    if (_conf.whether_pq) {
        quantizations.push_back(_pq_quantization.get());
    }

    for (uint32_t cid = 0; cid < thread_params.chunks_count; ++cid) {
        uint32_t real_thread_chunk_size = std::min(FLAGS_thread_chunk_size,
                                          (int)(thread_params.points_count - cid * FLAGS_thread_chunk_size));
        fvecs_fread(thread_params.learn_stream, chunk_points.get(), real_thread_chunk_size,
                    _conf.feature_dim);
        int cur_start_doc_id = thread_params.start_id + cid * FLAGS_thread_chunk_size;

        //计算doc的召回特征与所属于的cell的残差向量
        for (uint32_t point_id = 0; point_id < real_thread_chunk_size; ++point_id) {
            int true_doc_id = cur_start_doc_id + point_id;

            int cell_id = cell_assign[true_doc_id];

            coarse_assign = cell_id / _conf.fine_cluster_count;
            fine_assign = cell_id % _conf.fine_cluster_count;

            float* cur_residual = chunk_points.get() + point_id * _conf.feature_dim;
            cblas_saxpy(_conf.feature_dim, -1.0,
                        _coarse_vocab + coarse_assign * _conf.feature_dim, 1,
                        cur_residual, 1);
            cblas_saxpy(_conf.feature_dim, -1.0,
                        _fine_vocab + fine_assign * _conf.feature_dim, 1,
                        cur_residual, 1);
        }

        for (auto quantized : quantizations) {
            auto& cur_params = quantized->get_quantization_params();
            std::unique_ptr<float[]> sub_residual(new float[FLAGS_thread_chunk_size * cur_params.lsq]);

            for (uint32_t k = 0; k < (uint32_t)cur_params.nsq; ++k) {
                uint32_t cur_lsq = std::min(cur_params.lsq, cur_params.dim - k * cur_params.lsq);
                memset(sub_residual.get(), 0, FLAGS_thread_chunk_size * cur_params.lsq * sizeof(float));

                for (uint32_t i = 0; i < real_thread_chunk_size; i++) {
                    const float* cur_doc_fea = chunk_points.get() + i * _conf.feature_dim;
                    memcpy(sub_residual.get() + i * cur_params.lsq, cur_doc_fea + k * cur_params.lsq,
                           sizeof(float) * cur_lsq);
                }

                int distance_type = 2;

                float* cur_pq_centroids = quantized->get_sub_coodbooks(k);

                knn_full_thread(distance_type, real_thread_chunk_size, cur_params.ks, cur_params.lsq, 1,
                                cur_pq_centroids, sub_residual.get(), nullptr, pq_assign.get(), pq_distance.get(), 1);

                for (uint32_t i = 0; i < real_thread_chunk_size; i++) {
                    u_int64_t true_doc_id = cur_start_doc_id + i;
                    //doc i 在第k子空间对应的聚类中心id
                    //_pq_assign[k][true_doc_id] = (unsigned char)pq_assign.get()[i];
                    auto* quantized_fea = quantized->get_quantized_feature(true_doc_id);
                    quantized_fea += quantized->get_fea_offset();
                    quantized_fea[k] = (unsigned char)pq_assign.get()[i];
                    int cur_assign = pq_assign.get()[i];
                    float* cur_doc_fea = chunk_points.get() + i * _conf.feature_dim;
                    cblas_saxpy(cur_lsq, -1.0,
                                cur_pq_centroids + (u_int64_t)cur_assign * cur_params.lsq, 1,
                                cur_doc_fea + k * cur_params.lsq, 1);
                }

            }
        }

        //计算doc的召回特征与所属于的cell的残差向量
        for (uint32_t point_id = 0; point_id < real_thread_chunk_size; ++point_id) {
            int true_doc_id = cur_start_doc_id + point_id;

            int cell_id = cell_assign[true_doc_id];

            coarse_assign = cell_id / _conf.fine_cluster_count;
            fine_assign = cell_id % _conf.fine_cluster_count;

            std::unique_ptr<float[]> cur_residual(new float[_conf.feature_dim]);
            memcpy(cur_residual.get(), _coarse_vocab + coarse_assign * _conf.feature_dim,
                   _conf.feature_dim * sizeof(float));

            cblas_saxpy(_conf.feature_dim, 1.0,
                        _fine_vocab + fine_assign * _conf.feature_dim, 1,
                        cur_residual.get(), 1);

            for (auto quantized : quantizations) {
                quantized->set_static_value_of_formula(true_doc_id, cur_residual.get());
            }
        }
    }
    tm_cost.stop();
    LOG(INFO)<<"puck assign from "<<thread_params.start_id <<" cost "<<tm_cost.m_elapsed()<<" ms.";
    return 0;
}

void PuckIndex::batch_assign(const uint32_t total_cnt, const std::string& feature_file_name,
                             uint32_t* cell_assign) {
    this->HierarchicalClusterIndex::batch_assign(total_cnt, feature_file_name, cell_assign);
    this->HierarchicalClusterIndex::save_index();
    std::vector<std::thread> threads;
    std::exception_ptr lastException = nullptr;
    std::mutex lastExceptMutex;


    _filter_quantization->init_quantized_feature_memory();


    if (_conf.whether_pq) {
        _pq_quantization->init_quantized_feature_memory();
    }

    for (uint32_t threadId = 0; threadId < _conf.threads_count; ++threadId) {
        threads.push_back(std::thread([&, threadId] {

            {
                ThreadParams thread_params;
                thread_params.points_count = std::ceil(1.0 * total_cnt / _conf.threads_count);

                thread_params.start_id = threadId* thread_params.points_count;
                thread_params.points_count = std::min(thread_params.points_count, (int)(total_cnt - thread_params.start_id));

                if (thread_params.points_count > 0) {
                    thread_params.chunks_count = std::ceil(1.0 * thread_params.points_count / FLAGS_thread_chunk_size);

                    thread_params.open_file(feature_file_name.c_str(), _conf.feature_dim);

                    try {
                        LOG(INFO) << "puck_assign, thread_params.start_id = " << thread_params.start_id << " points_count = " <<
                                    thread_params.points_count << " feature_file_name = " << feature_file_name << " threadId = " << threadId;
                        puck_assign(thread_params, cell_assign);
                    } catch (...) {
                        std::unique_lock<std::mutex> lastExcepLock(lastExceptMutex);
                        lastException = std::current_exception();
                    }
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

    LOG(INFO) << "PuckIndex batch_assign Suc.";

}

int PuckIndex::train() {
    _conf.adaptive_train_param();

    //Puck 必须有filter
    if (_conf.whether_filter == false) {
        return -1;
    }

    this->HierarchicalClusterIndex::train();
    this->HierarchicalClusterIndex::read_coodbooks();

    std::string train_pq_file_name = FLAGS_train_pq_file_name;

    if (FLAGS_train_points_count < FLAGS_pq_train_points_count) {
        google::SetCommandLineOption("pq_train_points_count", std::to_string(FLAGS_train_points_count).c_str());
    }

    u_int64_t train_vocab_len = (u_int64_t)FLAGS_pq_train_points_count * _conf.feature_dim;

    std::unique_ptr<float[]> kmeans_train_vocab(new float[train_vocab_len]);
    uint32_t pq_train_points_count = random_sampling(FLAGS_train_fea_file_name, FLAGS_train_points_count,
                                     FLAGS_pq_train_points_count, _conf.feature_dim, kmeans_train_vocab.get());
    LOG(INFO) << "true doc cnt for puck train = " << pq_train_points_count;

    //写文件，训练使用这批抽样数据
    write_fvec_format(train_pq_file_name.c_str(),
                      kmeans_train_vocab.get(), pq_train_points_count, _conf.feature_dim);


    std::unique_ptr<uint32_t[]> cell_assign(new uint32_t[pq_train_points_count]);
    this->HierarchicalClusterIndex::batch_assign(pq_train_points_count, train_pq_file_name, cell_assign.get());

    //算残差
    for (uint32_t i = 0; i < pq_train_points_count; ++i) {
        float* residual = kmeans_train_vocab.get() + i * _conf.feature_dim;
        int coarse_assign = cell_assign[i] / _conf.fine_cluster_count;
        int fine_assign = cell_assign[i] % _conf.fine_cluster_count;

        cblas_saxpy(_conf.feature_dim, -1.0,
                    _coarse_vocab + coarse_assign * _conf.feature_dim, 1,
                    residual, 1);
        cblas_saxpy(_conf.feature_dim, -1.0,
                    _fine_vocab + fine_assign * _conf.feature_dim, 1,
                    residual, 1);
    }

    std::vector<Quantization*> quantizations;


    auto& cur_params = _filter_quantization->get_quantization_params();
    LOG(INFO) << cur_params.nsq << " " << cur_params.lsq << " " << cur_params.ks << " " << cur_params.lsq;
    quantizations.push_back(_filter_quantization.get());

    if (_conf.whether_pq) {
        auto& cur_params = _pq_quantization->get_quantization_params();
        LOG(INFO) << cur_params.nsq << " " << cur_params.lsq << " " << cur_params.ks << " " << cur_params.lsq;
        quantizations.push_back(_pq_quantization.get());
    }

    std::unique_ptr<int[]> pq_assign(new int[pq_train_points_count]);
    int idx = 0;

    for (auto quantized : quantizations) {
        auto& cur_params = quantized->get_quantization_params();
        LOG(INFO) << cur_params.nsq << " " << cur_params.lsq;
        std::unique_ptr<float[]> sub_resudial(new float[pq_train_points_count * cur_params.lsq]);

        for (uint32_t k = 0; k < (uint32_t)cur_params.nsq; k++) {
            uint32_t cur_lsq = std::min(cur_params.lsq, cur_params.dim - k * cur_params.lsq);
            memset(sub_resudial.get(), 0, sizeof(float) * pq_train_points_count * cur_params.lsq);

            for (uint32_t i = 0; i < pq_train_points_count; ++i) {
                const float* cur_train_point = kmeans_train_vocab.get() + i * _conf.feature_dim;
                memcpy(sub_resudial.get() + i * cur_params.lsq, cur_train_point + k * cur_params.lsq,
                       sizeof(float) * cur_lsq);
            }

            float* cur_pq_centroids = quantized->get_sub_coodbooks(k);

            KmeansParams params(true);
            memset(pq_assign.get(), 0, sizeof(int) * pq_train_points_count);

            float err = kmeans(cur_params.lsq, pq_train_points_count, cur_params.ks, params.niter,
                               sub_resudial.get(), params.flags, params.seed, params.redo,
                               cur_pq_centroids, nullptr, pq_assign.get(), nullptr);
            LOG(INFO) << "deviation error of init sub " << k << " pq codebook clusters is " << err;

            for (uint32_t i = 0; i < pq_train_points_count; ++i) {
                float* cur_train_point = kmeans_train_vocab.get() + i * _conf.feature_dim;
                int cur_assign = pq_assign.get()[i];
                cblas_saxpy(cur_lsq, -1.0,
                            cur_pq_centroids + (u_int64_t)cur_assign * cur_params.lsq, 1,
                            cur_train_point + k * cur_params.lsq, 1);
            }
        }

        LOG(INFO) << idx << " suc.";
    }

    save_coodbooks();
    return 0;
}
int PuckIndex::init_single_build() {
    this->HierarchicalClusterIndex::init_single_build();

    if (_conf.whether_filter == false) {
        return -1;
    }

    if (_filter_quantization->init_quantized_feature_memory() != 0) {
        LOG(INFO) << "get_filter_quantization init_quantized_feature_memory error";
        return -1;
    }

    if (_conf.whether_pq) {
        if (_pq_quantization->init_quantized_feature_memory() != 0) {
            LOG(INFO) << "pq_quantization init_quantized_feature_memory error";
            return -1;
        }
    } else   {
        void* memb = nullptr;
        int32_t pagesize = getpagesize();
        size_t all_feature_length = (size_t)_conf.total_point_count * _conf.feature_dim * sizeof(float);
        size_t size = all_feature_length + (pagesize - all_feature_length % pagesize);

        int err = posix_memalign(&memb, pagesize, size);

        if (err != 0) {
            std::runtime_error("alloc_aligned_mem_failed errno=" + errno);
            return -1;
        }

        _all_feature = reinterpret_cast<float*>(memb);
    }

    return 0;
}

int PuckIndex::single_build(BuildInfo* build_info) {
    PuckBuildInfo* puck_build_info = dynamic_cast<PuckBuildInfo*>(build_info);

    if (puck_build_info == nullptr) {
        return -1;
    }

    if (this->HierarchicalClusterIndex::single_build(build_info) != 0) {
        return 1;
    }

    std::vector<Quantization*> quantizations;
    quantizations.push_back(_filter_quantization.get());

    if (_conf.whether_pq) {
        quantizations.push_back(_pq_quantization.get());
    }

    uint32_t local_idx = 0;

    if (puck_single_assign(build_info, quantizations, local_idx) != 0) {
        return 1;
    }

    puck_build_info->quantizated_feature.resize(quantizations.size());

    for (uint32_t i = 0; i < quantizations.size(); ++i) {
        auto* quantized_feature = quantizations[i]->get_quantized_feature(local_idx);
        puck_build_info->quantizated_feature[i].first = ((float*)quantized_feature)[0];

        quantized_feature +=  quantizations[i]->get_fea_offset();
        uint32_t nsq = quantizations[i]->get_quantization_params().nsq;
        puck_build_info->quantizated_feature[i].second.resize(nsq);
        memcpy(puck_build_info->quantizated_feature[i].second.data(), quantized_feature, sizeof(unsigned char) * nsq);
    }

    return 0;
}
int PuckIndex::puck_single_assign(BuildInfo* build_info, std::vector<Quantization*>& quantizations,
                                  uint32_t idx) {
    std::unique_ptr<float[]> residual(new float[_conf.feature_dim]);
    float* chunk_points = build_info->feature.data();

    memcpy(residual.get(), chunk_points, sizeof(float) * _conf.feature_dim);

    int coarse_assign = build_info->nearest_cell.cell_id / _conf.fine_cluster_count;
    int fine_assign = build_info->nearest_cell.cell_id % _conf.fine_cluster_count;

    cblas_saxpy(_conf.feature_dim, -1.0,
                _coarse_vocab + coarse_assign * _conf.feature_dim, 1,
                residual.get(), 1);
    cblas_saxpy(_conf.feature_dim, -1.0,
                _fine_vocab + fine_assign * _conf.feature_dim, 1,
                residual.get(), 1);

    float pq_distance = 0;
    int pq_assign = -1;
    int n_thread = _conf.threads_count;

    for (auto* quantization : quantizations) {
        auto& param = quantization->get_quantization_params();
        LOG(INFO) << param.nsq << " " << param.ks << " " << param.lsq;

        for (u_int32_t k = 0; k < param.nsq; ++k) {
            float* sub_residual = residual.get() + k * param.lsq;
            int distance_type = 2;
            float* cur_pq_centroids = quantization->get_sub_coodbooks(k);

            knn_full_thread(distance_type, 1, param.ks, param.lsq, 1,
                            cur_pq_centroids, sub_residual, nullptr, &pq_assign, &pq_distance, n_thread);
            auto* quantized_fea = quantization->get_quantized_feature(idx);
            quantized_fea += quantization->get_fea_offset();
            quantized_fea[k] = (unsigned char)pq_assign;

            float* codeword = cur_pq_centroids + pq_assign * param.lsq;
            cblas_saxpy(param.lsq, -1.0,
                        codeword, 1,
                        sub_residual, 1);
        }
    }

    memcpy(residual.get(), _coarse_vocab + coarse_assign * _conf.feature_dim,
           _conf.feature_dim * sizeof(float));

    cblas_saxpy(_conf.feature_dim, 1.0,
                _fine_vocab + fine_assign * _conf.feature_dim, 1,
                residual.get(), 1);

    for (auto quantized : quantizations) {
        quantized->set_static_value_of_formula(idx, residual.get());
    }

    return 0;
}
} //namesapce puck

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

