#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "../data/Auxiliary.h"
#include "../data/Batch.h"
#include "../data/Placement.h"
#include "../data/Problem.h"

namespace szx {

class CutSearch {
public:
    struct Setting {
        bool opt_tail;  // 是否优化原料尾部。
        int max_iter;   // 最大扩展节点数目。
        int max_branch;  // 最大分支宽度。
        Setting(bool _opt_tail = false, int _max_iter = 10000, int _max_branch = 10) :
            opt_tail(_opt_tail), max_iter(_max_iter), max_branch(_max_branch) {}
    };
public:
    CutSearch(TID plate, TCoord start_pos, const Auxiliary &aux, const Setting &set) :
        defect_x_(aux.plate_defect_x[plate]), defect_y_(aux.plate_defect_y[plate]),
        start_pos_(start_pos), items_(aux.items), item_area_(aux.item_area), param_(aux.param), set_(set) {}
    void run(Batch &batch);
    UsageRate best_obj() const { return best_obj_; }
    void get_best_sol(Solution &sol) const { sol = best_sol_; }
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
private:
    UsageRate best_obj_;                    // 最优解对应利用率。
    Solution best_sol_;                     // 最优解的值。
    const TCoord start_pos_;                // 1-cut开始位置。
    const Setting set_;                     // 设置参数。
    const List<Defect> &defect_x_;          // 瑕疵按照x坐标从小到大排列。
    const List<Defect> &defect_y_;          // 瑕疵按照y坐标从小到大排列。
    const List<Rect> &items_;               // 每个物品的长宽信息。
    const List<Area> &item_area_;           // 每个物品的面积信息。
    const Problem::Input::Param &param_;    // 全局参数。
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
