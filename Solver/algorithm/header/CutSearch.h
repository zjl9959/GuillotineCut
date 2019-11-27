#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "Solver/data/header/Batch.h"

#include <mutex>

namespace szx {

class CutSearch {
public:
    struct Setting {
        bool opt_tail;  // 是否优化原料尾部。
        size_t max_iter;   // 最大扩展节点数目。
        size_t max_branch;  // 最大分支宽度。
        Setting(bool _opt_tail = false, int _max_iter = 10000, int _max_branch = 100) :
            opt_tail(_opt_tail), max_iter(_max_iter), max_branch(_max_branch) {}
    };
public:
    CutSearch(TID plate, TCoord start_pos, size_t nb_sol_cache, const Setting &set);
    
    void run(Batch &batch);
    UsageRate best_obj() const;                         // 返回最优解的目标函数值。
    void get_best_sol(Solution &sol) const;             // 获取最优解。
    void get_good_sols(List<Solution> &sols) const;     // 获取较好的解。
    void get_good_objs(List<UsageRate> &objs) const;    // 获取较好的解的目标函数值。
private:
    void beam_search(Batch &batch);
    void dfs(Batch &batch);
    void pfs(Batch &batch);
    UsageRate greedy(Area used_item_area, Batch &batch, const Solution &fix_sol);
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
    const Setting set_;                     // 设置参数。

    std::mutex sol_mutex_;                  // 更新最优解时需先获得该锁
    size_t nb_sol_cache_;                   // 缓存解的最大数量。
    std::multimap<UsageRate, Solution, UsageRateCmp> sol_cache_;   // 缓存解。
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
