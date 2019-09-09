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
        Filter(Length max_width = 0, Length max_height = 0, Length min_width = 0, Length min_height = 0) :
            max_width_(max_width), max_height_(max_height), min_width_(min_width), min_height_(min_height) {};
        bool operator()(ID item, const Auxiliary &aux);
    private:
        Length max_width_;
        Length max_height_;
        Length min_width_;
        Length min_height_;
    };
    class Terminator { /* �ж�ѡ�����Ƿ�����ֹͣ���� */
    public:
        Terminator(ID max_item_num = 0, Area total_area_lb = 0) : max_item_num_(max_item_num),
            total_area_lb_(total_area_lb), cur_item_num_(0), cur_total_area_(0) {};
        bool operator()(ID item, const Auxiliary &aux);
    private:
        ID max_item_num_;       // �����ѡ��Ʒ��Ŀ
        Area total_area_lb_;   // �����ѡ��Ʒ�����
        
        ID cur_item_num_;       // ��ǰ����ѡ��Ʒ��Ŀ
        Area cur_total_area_;   // ��ǰ����ѡ��Ʒ�����
    };
public:
    Picker(const Batch &source, Random &rand, const Auxiliary &aux) :
        source_(source), rand_(rand), aux_(aux) {};

    /* ����source�����ѡ���max_num����Ʒ��
    ���룺terminator��ֹͣ�����ж�������filter����Ʒɸѡ����
    �����target_batch����ѡ��������Ʒ���γɵ�ջ����true����ѡ�ɹ���false����ѡʧ�� */
    bool rand_pick(Batch &target_batch, Terminator &terminator = Terminator(), Filter &filter = Filter());
private:
    const Batch &source_;
    Random &rand_;
    const Auxiliary &aux_;
    CombinationCache cache_; // ������ѡ������Ʒ����֤���ظ�
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
