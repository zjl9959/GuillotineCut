#pragma once

#ifndef GUILLOTINECUT_MYCUTSEARCH_H
#define GUILLOTINECUT_MYCUTSEARCH_H

#include "../data/Auxiliary.h"
#include "../data/Batch.h"
#include "../data/Placement.h"

namespace szx {

class MyCutSearch {
public:
	struct TreeNode : public Placement {
		Depth depth;
		TreeNode() {}
		TreeNode(TCoord C1cpl) : Placement(C1cpl), depth(-1) {}
		TreeNode(const TreeNode &node, const TID item_id, const Status _flag) :
			Placement(node, item_id, _flag), depth(node.depth + 1) {}
	};

	struct OneCut {
		TID plate_id;
		TID cut_id;
		Batch &candidte;
		List<Placement> &sol;
		bool is_tail;
	};


public:
	MyCutSearch(TID plate, TCoord start_pos, const Auxiliary &aux) :
		plate_id_(plate), start_pos_(start_pos), aux_(aux) {}
	UsageRate run(Batch &batch, Solution &sol, bool opt_tail = false);
protected:
	UsageRate dfs(Batch &batch, Solution &sol, bool opt_tail = false);
	void branch(const TreeNode &old, const Batch &batch, const List<TreeNode> &cur_sol, List<TreeNode> &branch_nodes, bool opt_tail = false);
	const bool constraintCheck(const TreeNode &old, const List<TreeNode> &cur_sol, TreeNode &node);
	const TCoord sliptoDefectRight(const TCoord x, const TCoord y, const TLength w, const TLength h) const;
	const TCoord sliptoDefectUp(const TCoord x, const TCoord y, const TLength w, const TLength h) const;
	const bool defectConflictArea(const TCoord x, const TCoord y, const TLength w, const TLength h) const;

private:
	const TID plate_id_;
	const TCoord start_pos_;
	const Auxiliary& aux_;
	static constexpr int timeout_ms_ = 100;
};

}

#endif // LY_GUILLOTINECUT_MYCUTSEARCH_H