#pragma once
#ifndef GUILLOTINECUT_TOPSEARCH_H
#define GUILLOTINECUT_TOPSEARCH_H

#include <mutex>

#include "Solver/utility/Common.h"
#include "Solver/data/header/Batch.h"

namespace szx {

class TopSearch {
public:
    TopSearch() : best_obj_(Problem::Output::MaxWidth) {};

    /* �����������㷨�����Ż� */
    void beam_search();
    /* ���þֲ������㷨�����Ż� */
    void local_search();
    /* ��ȡ��õ����Ž� */
    void get_best_sol(Solution &sol) { sol = best_sol_; }
    /* ��ȡ���Ž�ʹ��ԭ�ϵĳ��� */
    Length best_obj() { return best_obj_; }
private:
    void branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch = 1);
    Length greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol);
    Length get_obj(const Solution &sol);
    void update_best_sol(const Solution &sol, Length obj = -1);
    
    size_t find_first_improvement(const Solution &cur_sol, Solution &improve);
    void get_plates_usage_rate(const Solution &sol, List<std::pair<size_t, UsageRate>> &usage_rates);
    size_t prev_plate_index(const Solution &sol, size_t cur_index);
private:
    std::mutex sol_mutex_;      // �������Ž�ʱ���Ȼ�ø���
    Solution best_sol_;         // ���Ž�
    Length best_obj_;           // ���Ž��Ӧ��ʹ��ԭ�ϳ���
};

}

#endif
