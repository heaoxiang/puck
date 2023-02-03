/***********************************************************************
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved
 * @file	string_split.h
 * @author	yinjie06(yinjie06@baidu.com)
 * @date	2023-01-31 19:51
 * @brief	
 ***********************************************************************/
#pragma once
#include <string>
#include <vector>
namespace puck{

u_int32_t s_split(const std::string& input_stream, const std::string& pattern, std::vector<std::string>& ret) {
    
    //在字符串末尾也加入分隔符，方便截取最后一段
    std::string strs = input_stream + pattern;
    size_t pos = strs.find(pattern);
    ret.clear();
    while(pos != strs.npos)
    {
        std::string temp = strs.substr(0, pos);
        if(temp.length() > 0){
            ret.push_back(temp);
        }else{
            break;
        }
        //去掉已分割的字符串,在剩下的字符串中进行分割
        strs = strs.substr(pos+1, strs.size());
        pos = strs.find(pattern);
    }
    //LOG(INFO)<<"s_split = "<<ret.size();
    return ret.size();
}
}

