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

    /* �����������룺��Ʒջ */
    void beam_search(const Batch &source_batch);

    /* ��ȡ���Ž⣬�����sol�����Ž⣩��*/
    void get_best_sol(Solution &sol) const;

    /* ��ȡ����ϺõĽ⣬�����sols(�����) */
    void get_good_sols(List<Solution> &sols) const;

    /* �������Ž��Ŀ�꺯��ֵ */
    Area best_obj() const { return sol_cache_.empty() ? -1 : sol_cache_.begin()->first; }

    /* ��ȡ����ϺõĽ��Ŀ�꺯��ֵ */
    void good_objs(List<Area> &objs) const;
protected:
	Area greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol);
    UsageRate get_cutsol(int repeat_num, TCoord start_pos, const Batch &source_batch, Solution &sol, CutSearch::Setting set);
private:
    Area item_area(const Solution &sol);
    void update_bestsol(const Solution &sol, Area obj = 0);
private:
    TID plate_;              // �Ż���ԭ��id
    size_t nb_sol_cache_;   // ����������
    Configuration &cfg_;
    Random &rand_;
    Timer &timer_;
    Auxiliary aux_;
    
    std::mutex sol_mutex_;  // �������Ž�ʱ���Ȼ�ø���
    std::map<Area, Solution, std::greater<Area>> sol_cache_; // ����һЩ�ϺõĽ�
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
