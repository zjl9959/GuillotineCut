#pragma once
#ifndef GUILLOTINECUT_PLATESEARCH_H
#define GUILLOTINECUT_PLATESEARCH_H

#include <mutex>

#include "Solver/data/header/Batch.h"
#include "Solver/data/header/Placement.h"

namespace szx{

class PlateSearch {
public:
    PlateSearch(TID plate) : plate_(plate), best_obj_(0) {};

    void run(const Batch &source_batch);            // ����PlateSearch�������
    void get_best_sol(Solution &sol) const
    {
        sol = best_sol_;
    }
    Area best_obj() const
    {
        return best_obj_;
    }
private:
    void skip(const Batch &source_batch);
    void beam_search(const Batch &source_batch);
    void branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, bool tail, size_t nb_branch = 1);
    Area greedy_evaluate(const Batch &source_batch, const Solution &sol);
    Area item_area(const Solution &sol);
    void update_best_sol(const Solution &sol, Area obj = 0);
private:
    TID plate_;                 // �Ż���ԭ��id
    Area best_obj_;             // �����Ʒ�����
    Solution best_sol_;         // ���Ž⡣
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
