#pragma once

#ifndef GUILLOTINE_CUT_PICKER_H
#define GUILLOTINE_CUT_PICKER_H

#include "../Common.h"
#include "../data/Batch.h"
#include "../data/Auxiliary.h"
#include "../utility/Utility.h"

namespace szx {

class Picker {  /* ʹ�ò�ͬ�Ĳ�����ѡ��Ʒ */
public:
    Picker(const Batch &source, Random &rand, const Auxiliary &aux) :
        source_(source), rand_(rand), aux_(aux) {};

    bool rand_pick(int max_num, Length max_width, Batch &target_batch);
private:
    const Batch &source_;
    Random &rand_;
    const Auxiliary &aux_;
    CombinationCache cache_; // ������ѡ������Ʒ����֤���ظ�
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
