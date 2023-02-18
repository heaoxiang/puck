/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    hierarchical_cluster.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-02 11:35
 * @brief
 ***********************************************************************/
#pragma once

#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <cmath>
#include <algorithm>
#include <gflags/gflags.h>
#include <fcntl.h>
#include <math.h>
//#include <glog/logging.h>
#include <glog/logging.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <vector.h>
#include <matrix.h>
#include <kmeans.h>
#include <mkl_cblas.h>
#include <nn.h>
#ifdef __cplusplus
}
#endif

#include "index_conf.h"
#include "puck_data_pool.h"
#include "index.h"
#include "base/time.h"
namespace puck {

//训练相关
DECLARE_int32(thread_chunk_size);
DECLARE_int32(train_points_count);
DECLARE_string(train_fea_file_name);

#define DISALLOW_COPY_AND_ASSIGN(TypeName)                      \
    TypeName(const TypeName&) = delete;            \
    void operator=(const TypeName&) = delete

#ifdef __GNUC__
# define BAIDU_CACHELINE_ALIGNMENT_8 __attribute__((aligned(8)))
#endif /* __GNUC__ */

//索引中二级聚类中心的信息
struct BAIDU_CACHELINE_ALIGNMENT_8 FineCluster {
    //uint32_t doc_cnt;
    float stationary_cell_dist;
    uint32_t memory_idx_start;
    FineCluster() {
        //doc_cnt = 0;
        stationary_cell_dist = std::sqrt(std::numeric_limits<float>::max());
        memory_idx_start = 0;
    }
    uint32_t get_point_cnt() const {
        return (this + 1)->memory_idx_start - this->memory_idx_start;
    }
};

//索引中一级聚类中心的信息
struct CoarseCluster {
    FineCluster* fine_cell_list;
    float min_dist_offset;

    CoarseCluster() {
        init();
    }
    void init() {
        fine_cell_list = nullptr;
        min_dist_offset = std::numeric_limits<float>::max();
    }
};

bool check_file_length_info(const std::string& file_name,
                            const uint64_t file_length);
void write_fvec_format(const char* file_name, const float* fea_vocab, const uint32_t fea_cnt,
                       const int dim);

int random_sampling(const std::string& init_file_name, const u_int64_t total_cnt,
                    const u_int64_t sampling_cnt, const uint32_t feature_dim, float* sampling_vocab);
class MaxHeap;
class SearchContext;
struct ThreadParams;
struct KmeansParams;
struct NearestCell;
struct BuildInfo;

class HierarchicalClusterIndex : public Index {
public:
    /*
     * @brief 默认构造函数
     **/
    HierarchicalClusterIndex();
    /*
     * @brief 默认析构函数
     **/
    virtual ~HierarchicalClusterIndex();
    /*
    * @brief 读取索引配置文件（index.dat）、初始化内存、加载索引文件，检索前需要先调用该函数
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int init();
    /*
     * @brief 检索最近的topk个样本
     * @@param [in] request : request
     * @@param [out] response : response
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int search(Request* request, Response* response);

    /*
    * @brief 初始化内存、训练码本（计算一二级聚类中心）、写码本文件
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int train();

    /*
    * @brief 读取索引配置文件（index.dat）、初始化内存、建库（计算样本最近的1个聚类中心）、写索引文件
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int build();

    /*
    * @brief 读取索引配置文件（index.dat）、初始化内存、1个样本建库（MR建库和实时入库时会调用该函数）
    * @@param [in/out] build_info : build_info包含样本建库所需的所有信息
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int single_build(BuildInfo* build_info);

    /*
     * @brief 读取索引配置文件（index.dat）、初始化内存、加载码本；第一次调研single_build时，用来初始化
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int init_single_build();

protected:
    /*
     * @brief 获取索引文件的配置信息
     * @@return （IndexConf）:当前索引的配置
     **/
    friend IndexConf load_index_conf_file();

    //////加载索引相关
    /*
     * @brief 读取索引配置文件（index.dat）
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int read_model_file();

    /*
     * @brief 初始化内存
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int init_model_memory();

    /*
     * @brief 读码本文件
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int read_coodbooks();

    /*
     * @brief 写码本文件
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int save_coodbooks() const;

    /*
     * @brief 写索引文件(建库的产出，与建库样本相关)
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int save_index();

    /*
     * @brief 读索引文件到内存，(建库的产出，与建库样本相关)
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int read_index();

    //////检索相关
    /*
     * @brief 获取cell的指针
     * @@param [in] cell_id : cell的ID
     * @@return (FineCluster*):cell_id对应的cell的指针
     **/
    FineCluster* get_fine_cluster(const uint32_t cell_id) const {
        return _coarse_clusters[cell_id / _conf.fine_cluster_count].fine_cell_list + cell_id %
               _conf.fine_cluster_count;;
    }

    /*
     * @brief 计算query与一级聚类中心的距离并排序
     * @@param [in\out] context : context由内存池管理
     * @@param [in] feature : query的特征向量
     * @@param [in] top_coarse_cnt : 保留top_coarse_cnt个最近的一级聚类中心
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int search_nearest_coarse_cluster(SearchContext* context, const float* feature,
                                      const uint32_t top_coarse_cnt);
    /*
     * @brief 计算query与top_coarse_cnt个一级聚类中心的下所有二级聚类中心的距离
     * @@param [in\out] context : context由内存池管理
     * @@param [in] feature : query的特征向量
     * @@return (int) : 正常返回保留的cell个数(>0)，错误返回值<0
     **/
    int search_nearest_fine_cluster(SearchContext* context, const float* feature);

    /*
     * @brief 计算query与某个cell下所有样本的距离（样本的原始特征）
     * @@param [in\out] context : context由内存池管理
     * @@param [in] cell_idx : 某个cell的id
     * @@param [in] feature : query的特征向量
     * @@param [in] result_heap : 堆结构，存储query与样本的topk
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int compute_exhaustive_distance_with_docs(SearchContext* context, const int cell_idx,
            const float* feature, MaxHeap& result_heap);
    /*
     * @brief 计算query与top-N个cell下所有样本的距离（样本的原始特征）
     * @@param [in\out] context : context由内存池管理
     * @@param [in] feature : query的特征向量
     * @@param [in] search_cell_cnt : 需要计算的cell的个数
     * @@param [in] result_heap : 堆结构，存储query与样本的topk
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int flat_topN_docs(SearchContext* context, const float* feature, const int search_cell_cnt,
                       MaxHeap& result_heap);

    /*
     * @brief 导出配置
     * @@param [out] ptr : 根据_conf的信息，按固定顺序更新ptr
     * @@return （size_t）: 配置信息的大小
     **/
    virtual size_t save_model_config(char* ptr) const;
    /*
     * @brief 根据配置信息，初始化_conf
     * @@param [int] ptr : 根据ptr，更新_conf的值
     * @@return （char*）: 下一次读取的位置
     **/
    virtual char* load_model_config(char* ptr);

    /*
     * @brief 写索引配置文件（index.dat）
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int save_model_file() const;
    /*
     * @brief 建库的计算过程
     * @@param [int] total_cnt : 建库样本总数
     * @@param [int] feature_file_name : 建库样本的文件名
     * @@param [out] cell_assign : 存储每个样本最近的cell id
     **/
    virtual void batch_assign(const uint32_t total_cnt, const std::string& feature_file_name,
                              uint32_t* cell_assign);
    /*
     * @brief 加载索引文件cell_assign.dat
     * @@param [out] cell_assign : 存储文件内存
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    int load_cell_assign(uint32_t* cell_assign);
    /*
     * @brief 检索过程中会按某种规则调整样本在内存的顺序（memory_idx），计算对应的信息
     * @@param [out] cell_start_memory_idx : 每个cell下样本中最小的memory_idx
     * @@param [out] local_to_memory_idx : 每个样本local_idx 与 memory_idx的映射关系
     * @@return (int) : 正常返回0，错误返回值<0
     **/
    virtual int convert_local_to_memory_idx(uint32_t* cell_start_memory_idx, uint32_t* local_to_memory_idx);

    /*
    * @brief 加载与样本相关的索引文件
    * @@param [in] local_to_memory_idx : 每个样本local_idx 与 memory_idx的映射关系
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    virtual int read_feature_index(uint32_t* local_to_memory_idx);

    /*
    * @brief 训练一二级聚类中心
    * @@param [in] kmenas_doc_cnt : 训练样本个数
    * @@param [in] kmeans_train_vocab : 训练样本的特征
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int train(const u_int64_t kmenas_doc_cnt, float* kmeans_train_vocab);

    /*
    * @brief 计算部分样本距离最近的cell
    * @@param [in] thread_params : 线程信息
    * @@param [out] cell_assign : 存储样本最近的cell id
    * @@param [out] error_distance : 评估cell质量的一个指标
    * @@param [out] pruning_computation : 记录剪枝比例
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int assign(const ThreadParams& thread_params, uint32_t* cell_assign, float* error_distance,
               float* pruning_computation) const;

    /*
    * @brief 计算一个样本距离最近的cell
    * @@param [in] coarse_distance/fine_distance/query_norm :<q,S>,<q,T>,<q,q>
    * @@param [out] nearest_cell : 记录最近的cell信息
    * @@return (int) : 正常返回0，错误返回值<0
    **/
    int nearest_cell_assign(const float* coarse_distance,
                            const float* fine_distance,
                            const float query_norm,
                            NearestCell& nearest_cell) const;

    /*
    * @brief 成员变量指针初始化nullptr
    **/
    void init_params_value();

    /*
    * @brief 初始化内存池
    **/
    void init_context_pool();

    /*
    * @brief 归一
    * @@param [in] context : context
    * @@param [in] feature_init : 原始特征向量
    * @@return (float*) : 检索过程中实际使用的feature
    **/
    const float* normalization(SearchContext* context, const float* feature);
    int check_feature_dim();
    DISALLOW_COPY_AND_ASSIGN(HierarchicalClusterIndex);
protected:
    IndexConf  _conf;
    DataHandlerPool<SearchContext> _context_pool;       //context pool
    CoarseCluster* _coarse_clusters;                    //新索引结构，一级聚类中心数组
    float*      _coarse_vocab;                          //存储一级聚类中心的特征，列存储
    float*      _coarse_norms;                          //存储一级聚类中心的特征的模，列存储

    float*      _fine_vocab;                            //存储二级聚类中心的特征，列存储
    float*      _fine_norms;                            //存储二级聚类中心的特征的模，列存储

    char* _model;
    uint32_t* _memory_to_local;
    float*      _all_feature;
};

//训练和建库过程中多线程中，记录线程需要处理的所有信息
struct ThreadParams {
    uint32_t start_id;       //当前线程处理的第一个doc的idx
    int points_count;         //处理的doc总数
    uint32_t chunks_count;
    FILE* learn_stream;       //特征文件句柄

    ThreadParams(): learn_stream(nullptr) {
        start_id = -1;
        points_count = -1;
    }
    //打开文件
    int open_file(const char* train_fea_file_name, uint32_t feature_dim) {
        close_file();
        learn_stream = fopen(train_fea_file_name, "r");

        if (!learn_stream) {
            return -1;
        }

        u_int64_t offset = (u_int64_t)start_id * feature_dim * sizeof(float) +
                           (u_int64_t)start_id * sizeof(int);

        //文件句柄指向要处理的doc块初始地址
        int ret = fseek(learn_stream, offset, SEEK_SET);

        if (ret != 0) {
            fpos_t ps;
            fgetpos(learn_stream, &ps);
            LOG(FATAL) << "seek file " << train_fea_file_name << " error, need offset = " << offset << " cur pos = " <<
                       ps.__pos <<
                       " when init thread params (start_id = " << start_id << ")";
            return -1;
        }

        return 0;
    }
    //关闭文件句柄
    int close_file() {
        if (learn_stream) {
            fclose(learn_stream);
            learn_stream = nullptr;
        }

        return 0;
    }
    ~ThreadParams() {
        close_file();
    }
};

struct NearestCell {
    int cell_id;
    float distance;
    float pruning_computation;
    void init() {
        cell_id = -1;
        distance = std::numeric_limits<float>::max();
        pruning_computation = 0;
    }
};

//Kmeans聚类的参数
struct KmeansParams {
    int redo;
    long int seed;
    int nt;
    int flags;
    int niter;
    KmeansParams(bool kmeans_pp = false) {
        redo    = 1;
        seed    = 0;
        nt      = std::thread::hardware_concurrency();

        if (kmeans_pp == false) {
            flags = nt | KMEANS_INIT_RANDOM | KMEANS_QUIET;
        } else {
            flags = nt | KMEANS_INIT_BERKELEY | KMEANS_QUIET;
        }

        niter   = 30;
    }

};

//建库所需信息(最近的cell id)
struct BuildInfo {
    std::string lable;
    std::vector<float> feature;
    std::string other_str_data;
    NearestCell nearest_cell;
    virtual ~BuildInfo() {}
};
IndexConf load_index_conf_file();
int getFileLineCnt(const char* fileName);
}

