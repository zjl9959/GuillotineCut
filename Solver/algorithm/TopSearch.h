#pragma once
#ifndef GUILLOTINECUT_TOPSEARCH_H
#define GUILLOTINECUT_TOPSEARCH_H

#include "../Common.h"
#include "PlateSearch.h"

namespace szx {

class TopSearch {
public:
    TopSearch(Random &rand, Timer &timer, Configuration &cfg, const Auxiliary &aux) :
        rand_(rand), timer_(timer), cfg_(cfg), aux_(aux), best_obj_(Problem::Output::MaxWidth) {};

    /* �����������㷨�����Ż� */
    void beam_search();
    /* ��ȡ��õ����Ž� */
    Length get_bestsol(Solution &sol);
protected:
    Area get_platesol(ID plate_id, const Batch &source_batch, Solution &sol);
    Length greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &sol);
private:
    Length get_obj(const Solution &sol);
    void update_bestsol(const Solution &sol, Length obj = -1);
private:
    Random &rand_;              // ������ӷ�����
    Timer &timer_;              // ��ʱ��
    Configuration &cfg_;        // ���ò���
    const Auxiliary &aux_;      // ��������

    std::mutex sol_mutex_;      // �������Ž�ʱ���Ȼ�ø���
    Solution best_sol_;         // ���Ž�
    Length best_obj_;           // ���Ž��Ӧ��ʹ��ԭ�ϳ���
};

}

#endif
