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

    /* 束搜索，输入：物品栈 */
    void beam_search(const Batch &source_batch);

    /* 获取最优解，输出：sol（最优解）；*/
    void get_best_sol(Solution &sol) const;

    /* 获取多个较好的解，输出：sols(多个解) */
    void get_good_sols(List<Solution> &sols) const;

    /* 返回最优解的目标函数值 */
    Area best_obj() const { return sol_cache_.empty() ? -1 : sol_cache_.begin()->first; }

    /* 获取多个较好的解的目标函数值 */
    void good_objs(List<Area> &objs) const;
protected:
	Area greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol);
    UsageRate get_cutsol(int repeat_num, TCoord start_pos, const Batch &source_batch, Solution &sol, CutSearch::Setting set);
private:
    Area item_area(const Solution &sol);
    void update_bestsol(const Solution &sol, Area obj = 0);
private:
    TID plate_;              // 优化的原料id
    size_t nb_sol_cache_;   // 缓存解的数量
    Configuration &cfg_;
    Random &rand_;
    Timer &timer_;
    Auxiliary aux_;
    
    std::mutex sol_mutex_;  // 更新最优解时需先获得该锁
    std::map<Area, Solution, std::greater<Area>> sol_cache_; // 缓存一些较好的解
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
