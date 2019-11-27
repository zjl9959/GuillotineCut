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
    /* ��ȡ��õ����Ž� */
    void get_best_sol(Solution &sol) { sol = best_sol_; }
    /* ��ȡ���Ž�ʹ��ԭ�ϵĳ��� */
    Length best_obj() { return best_obj_; }
protected:
    void branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch = 1);
    Length greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol);
private:
    Length get_obj(const Solution &sol);
    void update_best_sol(const Solution &sol, Length obj = -1);
private:
    std::mutex sol_mutex_;      // �������Ž�ʱ���Ȼ�ø���
    Solution best_sol_;         // ���Ž�
    Length best_obj_;           // ���Ž��Ӧ��ʹ��ԭ�ϳ���
};

}

#endif
