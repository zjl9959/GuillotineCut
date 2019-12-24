#pragma once
#ifndef GUILLOTINE_CUT_PICKER_H
#define GUILLOTINE_CUT_PICKER_H

#include "Solver/data/header/Batch.h"
#include "Solver/utility/Common.h"
#include "Solver/utility/Utility.h"
#include "Solver/data/header/Global.h"

namespace szx {

/* 使用不同的策略挑选物品 */
class Picker {
public:
    /* 针对某一个物品的筛选 */
    class Filter {
    public:
        Filter(TLength max_width = 32767, TLength max_height = 32767) :
            max_width_(max_width), max_height_(max_height) {};
        bool operator()(TID item);
    private:
        TLength max_width_;
        TLength max_height_;
    };
    /* 判断选择器是否满足停止条件 */
    class Terminator {
    public:
        Terminator(size_t max_item_num = 0, Area total_area_lb = 0) : max_item_num_(max_item_num),
            total_area_lb_(total_area_lb), cur_item_num_(0), cur_total_area_(0) {};
        bool operator()(TID item);
    private:
        size_t max_item_num_;          // 最大挑选物品数目
        Area total_area_lb_;        // 最大挑选物品面积和
        
        size_t cur_item_num_;          // 当前已挑选物品数目
        Area cur_total_area_;       // 当前已挑选物品面积和
    };
public:
    Picker(const Batch &source);

    /*
    * 根据source随机挑选最多max_num个物品。
    * 输入：terminator（停止条件判断器），filter（物品筛选器）。
    * 输出：target_batch（挑选出来的物品所形成的栈），true：挑选成功，false：挑选失败。
    */
    bool rand_pick(Batch &target_batch, Terminator terminator = Terminator(), Filter filter = Filter());
    bool pick_for_plate(TID pid, Batch &target_batch);
    bool pick_for_cut(TID pid, TCoord start_pos, Batch &target_batch);
private:
    const Batch &source_;
    CombinationCache cache_; // 缓存挑选过的物品，保证不重复
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
