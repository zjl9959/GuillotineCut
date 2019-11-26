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
    class Filter { /* ���ĳһ����Ʒ��ɸѡ */
    public:
        Filter(TLength max_width = 32767, TLength max_height = 32767) :
            max_width_(max_width), max_height_(max_height) {};
        bool operator()(TID item, const Auxiliary &aux);
    private:
        TLength max_width_;
        TLength max_height_;
    };
    class Terminator { /* �ж�ѡ�����Ƿ�����ֹͣ���� */
    public:
        Terminator(TID max_item_num = 0, Area total_area_lb = 0) : max_item_num_(max_item_num),
            total_area_lb_(total_area_lb), cur_item_num_(0), cur_total_area_(0) {};
        bool operator()(TID item, const Auxiliary &aux);
    private:
        TID max_item_num_;       // �����ѡ��Ʒ��Ŀ
        Area total_area_lb_;   // �����ѡ��Ʒ�����
        
        TID cur_item_num_;       // ��ǰ����ѡ��Ʒ��Ŀ
        Area cur_total_area_;   // ��ǰ����ѡ��Ʒ�����
    };
public:
    Picker(const Batch &source, Random &rand, const Auxiliary &aux) :
        source_(source), rand_(rand), aux_(aux), cache_(static_cast<ID>(aux.items.size())) {};

    /* ����source�����ѡ���max_num����Ʒ��
    ���룺terminator��ֹͣ�����ж�������filter����Ʒɸѡ����
    �����target_batch����ѡ��������Ʒ���γɵ�ջ����true����ѡ�ɹ���false����ѡʧ�� */
    bool rand_pick(Batch &target_batch, Terminator terminator = Terminator(), Filter filter = Filter());
private:
    const Batch &source_;
    Random &rand_;
    const Auxiliary &aux_;
    CombinationCache cache_; // ������ѡ������Ʒ����֤���ظ�
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
