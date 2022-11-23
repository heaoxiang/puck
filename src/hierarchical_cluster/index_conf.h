/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file    conf.h
 * @author  yinjie06(yinjie06@baidu.com)
 * @date    2022-09-28 14:23
 * @brief
 ***********************************************************************/
#ifndef BAIDU_MMS_GRAPH_GNOIMI_INDEX_CONF_H
#define BAIDU_MMS_GRAPH_GNOIMI_INDEX_CONF_H
#include <string>

namespace puck {

struct IndexConf {

    uint32_t nsq;                                //pq量化到的维数,256->128即2个float->1个char节省了8倍空间
    uint32_t lsq;
    uint32_t feature_dim;                        //特征维度(单位float)
    uint32_t ks;                                 //pq的类聚中心点个数,一般就是256,保证一个char能放下

    uint32_t gnoimi_coarse_cells_count;          //一级聚类中心个数
    uint32_t gnoimi_fine_cells_count;            //二级聚类中心个数
    uint32_t gnoimi_search_cells;                //检索过程中取top-gnoimi_search_cells个一级聚类中心

    uint32_t total_point_count;                  //索引包含的样本数据总数

    uint32_t neighbors_count;                    //检索的doc个数
    uint32_t topk;                               //取topK个检索结果

    bool whether_pq;
    bool whether_norm;

    uint32_t threads_count;             //建库并发线程数
    float cell_keep_ratio;              //根据平均cell内包含doc个数，获得二级聚类中心检索时保留cell个数的比例
    std::string feature_file_name;          //whether_pq = 0的时候存储全部原始特征

    std::string coarse_code_book_file_name; //一级类聚中心点码本
    std::string fine_code_book_file_name;   //二级残差类聚中心点码本
    //std::string alpha_file_name;            //alpha文件

    std::string cell_assign_file_name;
    //std::string pq_assign_file_name;
    std::string pq_codebook_file_name;
    std::string key_file_name;
    //filter
    uint32_t filter_nsq;
    uint32_t filter_topk;
    bool whether_filter;
    std::string filter_data_file_name;
    std::string filter_codebook_file_name;

    ///ip2cos
    uint32_t ip2cos;
    std::string index_file_name;
    //索引版本号，根据索引文件和配置自动更新
    //1 - puck , 2022年2月正式发布，当前默认的算法
    //2 - tinker，2022年6月正式发布，适合小规模数据集（1kw以下），FLAGS_using_tinker=true时使用tinker
    uint32_t index_version;
    std::string pq_data_file_name;
    std::string index_path;
    //tinker的检索参数
    uint32_t tinker_search_range;

    IndexConf();

    /*
    * @brief 根据用户配置的数据，更新部分训练参数，保证训练效果
    **/
    void adaptive_train_param();

    /*
    * @brief 根据用户配置的数据，更新部分检索参数
    **/
    int adaptive_search_param();
    /*
    * @brief 打印配置参数
    **/
    void show();
};

} //namesapce gnoimi

#endif

