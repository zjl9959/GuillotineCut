#pragma once
#ifndef GUILLOTINECUT_TOPSEARCH_H
#define GUILLOTINECUT_TOPSEARCH_H

#include <mutex>

#include "Solver/utility/Common.h"
#include "Solver/data/header/Batch.h"

namespace szx {

class TopSearch {
public:
    TopSearch() : best_obj_(Problem::Output::MaxWidth), total_item_area_(0) {
        for (auto area : gv::item_area)total_item_area_ += area;
    };
    void run(); // ����TopSearch�������
    /* ��ȡ��õ����Ž� */
    void get_best_sol(Solution &sol) { sol = best_sol_; }
    /* ��ȡ���Ž�ʹ��ԭ�ϵĳ��� */
    Length best_obj() { return best_obj_; }
private:
    void beam_search();
    void branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch = 1);
    Length greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol);
    Length get_obj(const Solution &sol);
    void update_best_sol(const Solution &sol, Length obj = -1);
    List<Area> get_plates_waste(const Solution &sol);
private:
    Area total_item_area_;      // ��Ʒ�����
    std::mutex sol_mutex_;      // �������Ž�ʱ���Ȼ�ø���
    Solution best_sol_;         // ���Ž�
    Length best_obj_;           // ���Ž��Ӧ��ʹ��ԭ�ϳ���
};

}

#endif
