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
    CutSearch(TID plate, TCoord start_pos, const Auxiliary &aux) :
        defect_x_(aux.plate_defect_x[plate]), defect_y_(aux.plate_defect_y[plate]),
        start_pos_(start_pos), items_(aux.items), item_area_(aux.item_area), param_(aux.param) {}
    UsageRate run(Batch &batch, Solution &sol, Setting set);
protected:
    UsageRate beam_search(Batch &batch, Solution &sol, Setting set);
    UsageRate dfs(Batch &batch, Solution &sol, Setting set);
    UsageRate pfs(Batch &batch, Solution &sol, Setting set);
    void branch(const Placement &old, const Batch &batch, List<Placement> &branch_nodes, bool opt_tail = false);
    const bool constraintCheck(const Placement &old, Placement &node);
    const TCoord sliptoDefectRight(const TCoord x, const TCoord y, const TLength w, const TLength h) const;
    const TCoord sliptoDefectUp(const TCoord x, const TCoord y, const TLength w) const;
    const TCoord cut1ThroughDefect(const TCoord x) const;
    const TCoord cut2ThroughDefect(const TCoord y) const;
private:
    const TCoord start_pos_;
    const List<Defect> &defect_x_;
    const List<Defect> &defect_y_;
    const List<Rect> &items_;
    const List<Area> &item_area_;
    const Problem::Input::Param &param_;
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
