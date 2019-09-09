#pragma once
#ifndef GUILLOTINECUT_TOPSEARCH_H
#define GUILLOTINECUT_TOPSEARCH_H

#include "../Common.h"
#include "PlateSearch.h"

namespace szx {

class TopSearch {
public:
    TopSearch(Random &rand, Timer &timer, Configuration &cfg, const Auxiliary &aux) :
        rand_(rand), timer_(timer), cfg_(cfg), aux_(aux) {};

    void beam_search();
protected:
    Score get_platesol(ID plate_id, const Batch &source_batch, Solution &sol);
    Length greedy_evaluate(const Batch &source_batch, Solution &sol);
private:
    Random &rand_;              // ������ӷ�����
    Timer &timer_;              // ��ʱ��
    Configuration &cfg_;  // ���ò���
    const Auxiliary &aux_;      // ��������

    std::mutex sol_mutex_;  // �������Ž�ʱ���Ȼ�ø���
    Solution best_sol_;     // ���Ž�
    Length best_obj;        // ���Ž��Ӧ��ʹ��ԭ�ϳ���
};

}

#endif
