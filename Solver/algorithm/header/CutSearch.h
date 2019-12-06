#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "Solver/data/header/Batch.h"

#include <mutex>

namespace szx {

class CutSearch {
public:
    enum BRANCH_MODE {
        CUT,    // 只在该1-cut内分支。
        PLATE,  // 分支时可以开新的1-cut，但不能开新的plate。
    };
public:
    CutSearch(TID plate, TCoord start_pos, size_t nb_sol_cache, const BRANCH_MODE mode);
    
    void run(Batch &batch);
    UsageRate best_obj() const;                         // 返回最优解的目标函数值。
    void get_best_sol(Solution &sol) const;             // 获取最优解。
    void get_good_sols(List<Solution> &sols) const;     // 获取较好的解。
    void get_good_objs(List<UsageRate> &objs) const;    // 获取较好的解的目标函数值。
private:
    void beam_search(Batch &batch);
    void dfs(Batch &batch);
    void pfs(Batch &batch);
    UsageRate greedy(Batch &batch, const Solution &fix_sol);
    void branch(const Placement &old, const Batch &batch, List<Placement> &branch_nodes);
    bool constraintCheck(const Placement &old, Placement &node);
    TCoord sliptoDefectRight(TCoord x, TCoord y, TLength w, TLength h) const;
    TCoord sliptoDefectUp(TCoord x, TCoord y, TLength w) const;
    TCoord cut1ThroughDefect(TCoord x) const;
    TCoord cut2ThroughDefect(TCoord y) const;
    Area envelope_area(const Placement &place) const;
    void update_sol_cache(UsageRate obj, const Solution &sol);
private:
    struct UsageRateCmp {
        bool operator()(const UsageRate &lhs, const UsageRate &rhs) {
            return rhs < lhs;
        }
    };
private:
    const TID plate_;                       // 1-cut位于原料的id。
    const Area tail_area_;                  // 剩余面积
    const TCoord start_pos_;                // 1-cut开始位置。
    const BRANCH_MODE mode_;                // 分支模式的设置。

    std::mutex sol_mutex_;                  // 更新最优解时需先获得该锁
    size_t nb_sol_cache_;                   // 缓存解的最大数量。
    std::map<UsageRate, Solution, UsageRateCmp> sol_cache_;   // 缓存解。
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
