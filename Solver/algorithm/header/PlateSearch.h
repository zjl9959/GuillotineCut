#pragma once
#ifndef GUILLOTINECUT_PLATESEARCH_H
#define GUILLOTINECUT_PLATESEARCH_H

#include <mutex>

#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/data/header/Batch.h"
#include "Solver/data/header/Placement.h"

namespace szx{

class PlateSearch {
public:
    PlateSearch(TID plate, size_t nb_sol_cache) : plate_(plate), nb_sol_cache_(nb_sol_cache) {};

    void run(const Batch &source_batch);            // ����PlateSearch�������
    void get_best_sol(Solution &sol) const;         // ��ȡ���Ž⡣
    void get_good_sols(List<Solution> &sols) const; // ��ȡ����ϺõĽ⣬�����sols(�����)��
    Area best_obj() const;                          // �������Ž��Ŀ�꺯��ֵ��
    void get_good_objs(List<Area> &objs) const;     // ��ȡ����ϺõĽ��Ŀ�꺯��ֵ��
private:
    void skip(const Batch &source_batch);
    void beam_search(const Batch &source_batch);
    void branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, CutSearch::BRANCH_MODE mode, size_t nb_branch = 1);
    Area greedy_evaluate(const Batch &source_batch, const Solution &sol);
    Area item_area(const Solution &sol);
    void update_sol_cache(const Solution &sol, Area obj = 0);
private:
    TID plate_;                 // �Ż���ԭ��id
    size_t nb_sol_cache_;       // ����������
    std::mutex sol_mutex_;      // �������Ž�ʱ���Ȼ�ø���
    std::map<Area, Solution, std::greater<Area>> sol_cache_; // ����һЩ�ϺõĽ� // [zjl][TODO]:����Ŀ�꺯��ֵ��ͬ�Ľ����μ�¼�����
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
