#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "../data/Auxiliary.h"
#include "../data/Batch.h"
#include "../data/Placement.h"
#include "../data/Auxiliary.h"
#include "../data/Problem.h"

namespace szx {

class CutSearch {
public:
    struct TreeNode : public Placement { /* 定义树搜索中的树节点 */
        Depth depth;    // 节点深度
        Score score;    // 节点优度
        
        TreeNode() {}
        TreeNode(Coord C1cpl) : Placement(C1cpl), depth(-1), score(0.0) {}
        TreeNode(const TreeNode &node, const ID item_id, const Status _flag, const Score _score = 0.0) :
            Placement(node, item_id, _flag), depth(node.depth + 1), score(_score) {}
    };
public:
    CutSearch(ID plate, Coord start_pos, const Auxiliary &aux) :
        defect_x_(aux.plate_defect_x[plate]), defect_y_(aux.plate_defect_y[plate]),
        start_pos_(start_pos), items_(aux.items), item_area_(aux.item_area), param_(aux.param) {}
    Score run(Batch &batch, Solution &sol, bool opt_tail = false);
protected:
    Score dfs(Batch &batch, Solution &sol, bool opt_tail = false);
    Score pfs(Batch &batch, Solution &sol, bool opt_tail = false);
    void branch(const TreeNode &old, Batch &batch, List<TreeNode> &branch_nodes, bool opt_tail = false);
    const bool constraintCheck(const TreeNode &old, TreeNode &node);
    const Coord sliptoDefectRight(const Coord x, const Coord y, const Length w, const Length h) const;
    const Coord sliptoDefectUp(const Coord x, const Coord y, const Length w) const;
    const Coord cut1ThroughDefect(const Coord x) const;
    const Coord cut2ThroughDefect(const Coord x1, const Coord x2, const Coord y) const;
private:
    static constexpr int timeout_ms_ = 100;
private:
    const Coord start_pos_;
    const List<Defect> defect_x_;
    const List<Defect> defect_y_;
    const List<Rect> items_;
    const List<Area> item_area_;
    const Problem::Input::Param param_;
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
