#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "Solver/data/header/Batch.h"

#include <mutex>

namespace szx {

// 0 : CutSearch中不断更新中间解文件
// 1 ：PlateSearch中不断更新中间解文件
#define UPDATE_MIDDLE_SOL 2

class CutSearch {
public:
    CutSearch(TID plate, TCoord start_pos, bool opt_tail);
    
    void run(Batch &batch);
    UsageRate best_obj() const
    {
        return best_obj_;
    }
    void get_best_sol(Solution &sol) const
    {
        sol = best_sol_;
    }
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
    void update_best_sol(UsageRate obj, const Solution &sol);
private:
    const TID plate_;                       // 1-cut位于原料的id。
    const Area tail_area_;                  // 剩余面积
    const TCoord start_pos_;                // 1-cut开始位置。
    const bool opt_tail_;                   // 是否优化最后一个1-cut。

    UsageRate best_obj_;                    // 最优目标函数值。
    Solution best_sol_;                     // 最优解。
};

void update_middle_solution(TID pid, const Solution &sol);

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
