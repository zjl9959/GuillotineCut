#pragma once
#ifndef GUILLOTINE_CUT_PICKER_H
#define GUILLOTINE_CUT_PICKER_H

#include "../Common.h"
#include "../data/Batch.h"
#include "../data/Auxiliary.h"
#include "../utility/Utility.h"

namespace szx {

class Picker {  /* 使用不同的策略挑选物品 */
public:
    class Filter { /* 针对某一个物品的筛选 */
    public:
        Filter(Length max_width = 0, Length max_height = 0, Length min_width = 0, Length min_height = 0) :
            max_width_(max_width), max_height_(max_height), min_width_(min_width), min_height_(min_height) {};
        bool operator()(ID item, const Auxiliary &aux);
    private:
        Length max_width_;
        Length max_height_;
        Length min_width_;
        Length min_height_;
    };
    class Terminator { /* 判断选择器是否满足停止条件 */
    public:
        Terminator(ID max_item_num = 0, Area total_area_lb = 0) : max_item_num_(max_item_num),
            total_area_lb_(total_area_lb), cur_item_num_(0), cur_total_area_(0) {};
        bool operator()(ID item, const Auxiliary &aux);
    private:
        ID max_item_num_;       // 最大挑选物品数目
        Area total_area_lb_;   // 最大挑选物品面积和
        
        ID cur_item_num_;       // 当前已挑选物品数目
        Area cur_total_area_;   // 当前已挑选物品面积和
    };
public:
    Picker(const Batch &source, Random &rand, const Auxiliary &aux) :
        source_(source), rand_(rand), aux_(aux) {};

    /* 根据source随机挑选最多max_num个物品。
    输入：terminator（停止条件判断器），filter（物品筛选器）
    输出：target_batch（挑选出来的物品所形成的栈），true：挑选成功，false：挑选失败 */
    bool rand_pick(Batch &target_batch, Terminator &terminator = Terminator(), Filter &filter = Filter());
private:
    const Batch &source_;
    Random &rand_;
    const Auxiliary &aux_;
    CombinationCache cache_; // 缓存挑选过的物品，保证不重复
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
