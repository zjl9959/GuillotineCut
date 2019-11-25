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
    PlateSearch(TID plate, size_t nb_sol_cache, Configuration &cfg, Random &rand, Timer &timer, const Auxiliary &aux) :
		cfg_(cfg), rand_(rand), timer_(timer), plate_(plate), aux_(aux), nb_sol_cache_(nb_sol_cache) {};

    void beam_search(const Batch &source_batch);    // �����������룺��Ʒջ��
    void get_best_sol(Solution &sol) const;         // ��ȡ���Ž⡣
    void get_good_sols(List<Solution> &sols) const; // ��ȡ����ϺõĽ⣬�����sols(�����)��
    Area best_obj() const;                          // �������Ž��Ŀ�꺯��ֵ��
    void get_good_objs(List<Area> &objs) const;     // ��ȡ����ϺõĽ��Ŀ�꺯��ֵ��
protected:
    void branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, bool opt_tail = false, size_t nb_branch = 1);
    Area greedy_evaluate(const Batch &source_batch, const Solution &sol);
private:
    Area item_area(const Solution &sol);
    void update_sol_cache(const Solution &sol, Area obj = 0);
private:
    TID plate_;              // �Ż���ԭ��id
    size_t nb_sol_cache_;   // ����������
    Configuration &cfg_;
    Random &rand_;
    Timer &timer_;
    Auxiliary aux_;
    
    std::mutex sol_mutex_;  // �������Ž�ʱ���Ȼ�ø���
    std::map<Area, Solution, std::greater<Area>> sol_cache_; // ����һЩ�ϺõĽ� // [zjl][TODO]:����Ŀ�꺯��ֵ��ͬ�Ľ����μ�¼�����
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
