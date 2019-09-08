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
    Picker(const Batch &source, Random &rand, const Auxiliary &aux) :
        source_(source), rand_(rand), aux_(aux) {};

    /* 
    根据source随机挑选最多max_num个物品。
    输入：max_num（最多挑多少个物品），max_width（物品最大宽度限制）
    输出：target_batch（挑选出来的物品所形成的栈），true：挑选成功，false：挑选失败
    */
    bool rand_pick(int max_num, Length max_width, Batch &target_batch);
private:
    const Batch &source_;
    Random &rand_;
    const Auxiliary &aux_;
    CombinationCache cache_; // 缓存挑选过的物品，保证不重复
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
