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

    void run(const Batch &source_batch);            // 运行PlateSearch求解器。
    void get_best_sol(Solution &sol) const;         // 获取最优解。
    void get_good_sols(List<Solution> &sols) const; // 获取多个较好的解，输出：sols(多个解)。
    Area best_obj() const;                          // 返回最优解的目标函数值。
    void get_good_objs(List<Area> &objs) const;     // 获取多个较好的解的目标函数值。
private:
    void skip(const Batch &source_batch);
    void beam_search(const Batch &source_batch);
    void branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, CutSearch::BRANCH_MODE mode, size_t nb_branch = 1);
    Area greedy_evaluate(const Batch &source_batch, const Solution &sol);
    Area item_area(const Solution &sol);
    void update_sol_cache(const Solution &sol, Area obj = 0);
private:
    TID plate_;                 // 优化的原料id
    size_t nb_sol_cache_;       // 缓存解的数量
    std::mutex sol_mutex_;      // 更新最优解时需先获得该锁
    std::map<Area, Solution, std::greater<Area>> sol_cache_; // 缓存一些较好的解 // [zjl][TODO]:对于目标函数值相同的解该如何记录与过滤
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
