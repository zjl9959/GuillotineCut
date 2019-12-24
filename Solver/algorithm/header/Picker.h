#pragma once
#ifndef GUILLOTINE_CUT_PICKER_H
#define GUILLOTINE_CUT_PICKER_H

#include "Solver/data/header/Batch.h"
#include "Solver/utility/Common.h"
#include "Solver/utility/Utility.h"
#include "Solver/data/header/Global.h"

namespace szx {

/* ʹ�ò�ͬ�Ĳ�����ѡ��Ʒ */
class Picker {
public:
    /* ���ĳһ����Ʒ��ɸѡ */
    class Filter {
    public:
        Filter(TLength max_width = 32767, TLength max_height = 32767) :
            max_width_(max_width), max_height_(max_height) {};
        bool operator()(TID item);
    private:
        TLength max_width_;
        TLength max_height_;
    };
    /* �ж�ѡ�����Ƿ�����ֹͣ���� */
    class Terminator {
    public:
        Terminator(size_t max_item_num = 0, Area total_area_lb = 0) : max_item_num_(max_item_num),
            total_area_lb_(total_area_lb), cur_item_num_(0), cur_total_area_(0) {};
        bool operator()(TID item);
    private:
        size_t max_item_num_;          // �����ѡ��Ʒ��Ŀ
        Area total_area_lb_;        // �����ѡ��Ʒ�����
        
        size_t cur_item_num_;          // ��ǰ����ѡ��Ʒ��Ŀ
        Area cur_total_area_;       // ��ǰ����ѡ��Ʒ�����
    };
public:
    Picker(const Batch &source);

    /*
    * ����source�����ѡ���max_num����Ʒ��
    * ���룺terminator��ֹͣ�����ж�������filter����Ʒɸѡ������
    * �����target_batch����ѡ��������Ʒ���γɵ�ջ����true����ѡ�ɹ���false����ѡʧ�ܡ�
    */
    bool rand_pick(Batch &target_batch, Terminator terminator = Terminator(), Filter filter = Filter());
    bool pick_for_plate(TID pid, Batch &target_batch);
    bool pick_for_cut(TID pid, TCoord start_pos, Batch &target_batch);
private:
    const Batch &source_;
    CombinationCache cache_; // ������ѡ������Ʒ����֤���ظ�
};

}

#endif // !GUILLOTINE_CUT_PICKER_H
