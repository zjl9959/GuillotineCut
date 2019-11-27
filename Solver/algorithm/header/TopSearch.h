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

    /* 调用束搜索算法进行优化 */
    void beam_search();
    /* 获取算得的最优解 */
    void get_best_sol(Solution &sol) { sol = best_sol_; }
    /* 获取最优解使用原料的长度 */
    Length best_obj() { return best_obj_; }
protected:
    void branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch = 1);
    Length greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol);
private:
    Length get_obj(const Solution &sol);
    void update_best_sol(const Solution &sol, Length obj = -1);
private:
    std::mutex sol_mutex_;      // 更新最优解时需先获得该锁
    Solution best_sol_;         // 最优解
    Length best_obj_;           // 最优解对应的使用原料长度
};

}

#endif
