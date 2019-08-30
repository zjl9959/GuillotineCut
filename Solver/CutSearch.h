#pragma once

#ifndef SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
#define SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H

#include "Algorithm.h"

namespace szx {

class CutSearch {
public:
    CutSearch(const TID plate, const TCoord start_pos) :
        defect_x_(GV::plate_defect_x[plate]), defect_y_(GV::plate_defect_y[plate]), start_pos_(start_pos) {}
    Score dfs(List<List<TID>> &batch, List<SolutionNode> &sol, bool opt_tail = false, Score gap = 0.0);
    Score pfs(List<List<TID>> &batch, List<SolutionNode> &sol, bool opt_tail = false, Score gap = 0.0);
protected:
    void branch(const ItemNode &old, const List<List<TID>> &batch, List<ItemNode> &branch_nodes, bool opt_tail = false);
    const bool constraintCheck(const ItemNode &old, ItemNode &node);
    const TCoord sliptoDefectRight(const TCoord x, const TCoord y, const TLength w, const TLength h) const;
    const TCoord sliptoDefectUp(const TCoord x, const TCoord y, const TLength w) const;
    const TCoord cut1ThroughDefect(const TCoord x) const;
    const TCoord cut2ThroughDefect(const TCoord x1, const TCoord x2, const TCoord y) const;
private:
    static constexpr int timeout_ms_ = 100;
private:
    const TCoord start_pos_;
    const List<Defect> defect_x_;
    const List<Defect> defect_y_;
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
