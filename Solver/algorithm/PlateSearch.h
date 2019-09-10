#pragma once
#ifndef GUILLOTINECUT_PLATESEARCH_H
#define GUILLOTINECUT_PLATESEARCH_H

#include <mutex>

#include "CutSearch.h"
#include "../data/Configuration.h"
#include "../data/Auxiliary.h"

namespace szx{

class PlateSearch {
public:
    PlateSearch(TID plate, Configuration &cfg, Random &rand, Timer &timer, const Auxiliary &aux) :
		cfg_(cfg), rand_(rand), timer_(timer), plate_(plate), aux_(aux), bestobj_(0) {};

    /* �����������룺��Ʒջ */
    void beam_search(const Batch &source_batch);

    /* ��ȡ���Ž⣬��������Ž⣻���أ����Ž��Ŀ�꺯��ֵ */
    Area get_bestsol(Solution &sol);

protected:
	Area greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol);
    Score get_cutsol(int repeat_num, TCoord start_pos, const Batch &source_batch, Solution &sol, bool tail = false);
private:
    Area item_area(const Solution &sol);
    void update_bestsol(const Solution &sol, Area obj = 0);
private:
	static constexpr ScorePair InvalidPair = std::make_pair(-1, 0.0);
    TID plate_;              // �Ż���ԭ��id
    Configuration &cfg_;
    Random &rand_;
    Timer &timer_;
    Auxiliary aux_;
    
    std::mutex sol_mutex_;  // �������Ž�ʱ���Ȼ�ø���
    Solution bestsol_;      // ���Ž�
    Area bestobj_;          // ����Ŀ�꺯��ֵ
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
