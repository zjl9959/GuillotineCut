#include "MyCutSearch.h"

using namespace std;

namespace szx {

/* 
  优化1-cut
  输入：batch（物品栈），opt_tail（是否优化原料末尾）
  输出：sol（该1-cut的最优解），返回：1-cut对应利用率 
*/
Score szx::MyCutSearch::run(Batch & batch, Solution & sol, bool opt_tail) {
	return dfs(batch, sol, opt_tail);
}

Score szx::MyCutSearch::dfs(Batch & batch, Solution & sol, bool opt_tail) {
	List<TreeNode> live_nodes; live_nodes.reserve(100); // 待分支的树节点
	List<TreeNode> cur_sol; cur_sol.reserve(20);        // 当前解 
	List<TreeNode> best_sol; Score best_obj = 0.0;      // 最优解及对应目标函数值

	Depth pre_depth = -1;    // 上一个节点深度
	Area used_item_area = 0; // 已使用物品面积
	
	Timer timer(static_cast<Timer::Millisecond>(timeout_ms_));
	int see_timeout = 100000; // 检查是否超时间隔时间

	branch(TreeNode(start_pos_), batch, cur_sol, live_nodes, opt_tail);
	while (live_nodes.size()) {
		TreeNode node = live_nodes.back();
		live_nodes.pop_back();
		if (node.depth - pre_depth == 1) { // 向下扩展
			cur_sol.push_back(node);
			batch.pop(node.item);
			used_item_area += aux_.item_area[node.item];
		}
		else if (node.depth - pre_depth < 1) { // 向上回溯
			for (int i = cur_sol.size() - 1; i >= node.depth; --i) { 
				batch.push(cur_sol[i].item);   // 恢复栈
				used_item_area -= aux_.item_area[cur_sol[i].item]; 
			}
			cur_sol.erase(cur_sol.begin() + node.depth, cur_sol.end()); // 删除兄弟节点及兄弟的子节点，cur_sol[i] 在树的第 i_th 层
			cur_sol.push_back(node);
			batch.pop(node.item);
			used_item_area += aux_.item_area[node.item];
		}
		const size_t old_live_nodes_size = live_nodes.size();
		branch(node, batch, cur_sol, live_nodes, opt_tail);
		pre_depth = node.depth;
		if (old_live_nodes_size == live_nodes.size()) { // 不能继续分支，找到一个完整解
			Score cur_obj;
			if (opt_tail) {
				cur_obj = (double)used_item_area / (double)((aux_.param.plateWidth - start_pos_) * aux_.param.plateHeight);
			}
			else {
				cur_obj = (double)used_item_area / (double)((cur_sol.back().c1cpr - start_pos_) * aux_.param.plateHeight);
			}
			if (best_obj < cur_obj) {
				best_obj = cur_obj;
				best_sol = cur_sol;
			}
		}
		if (see_timeout == 0) { // 检查是否超时
			if (timer.isTimeOut())
				break;
			else
				see_timeout = 100000;
		}
		--see_timeout;
	}

	if (!best_sol.empty()) {
		sol.clear();
		sol.reserve(best_sol.size());
		for (int i = 0; i < best_sol.size(); ++i) { sol.push_back(best_sol[i]); }
		return best_obj;
	}
	return -2.0;
}

/*
  分支函数：
  输入：old（分支起点），batch（物品栈），opt_tail（优化尾部）
  输出：branch_nodes（用栈保存的搜索树）
*/
void MyCutSearch::branch(const TreeNode & old, const Batch & batch, const List<TreeNode> &cur_sol, List<TreeNode>& branch_nodes, bool opt_tail) {
	TID itemId = Problem::InvalidItemId;
	/*
	* case A, 开一个新的L1桶.
	* c2cpu = 0 & c1cpl = c1cpr, 当前没有2-cut，即新的1-cut没有放置物品
	*/
#pragma region CASE_A
	if (old.c2cpu == 0) {
		for (int rotate = 0; rotate <= 1; ++rotate) {
			for (int s = 0; s < batch.stack_num(); ++s) {
				itemId = batch.get(s);
				if (itemId == Problem::InvalidItemId) continue; // skip empty stack.
				// pretreatment.
				Rect item = aux_.items[itemId];
				if (item.w == item.h && rotate) continue; // square item no need to rotate.
				if (rotate) { // rotate item.
					item.w = aux_.items[itemId].h;
					item.h = aux_.items[itemId].w;
				}
				if (old.c1cpr + item.w > aux_.param.plateWidth) continue;

				TLength slip_r, slip_u;
				// TODO: precheck if item exceed plate bound to speed up branch.
				// check if item conflict with defect and try to fix it.
				slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
				if (slip_r == old.c1cpr) { // no conflict
					TreeNode node_a0(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2);
					node_a0.c3cp = node_a0.c1cpr = old.c1cpr + item.w;
					node_a0.c4cp = node_a0.c2cpu = item.h;
					if (old.item == Problem::InvalidItemId && old.c1cpr == 0) {
						node_a0.setFlagBit(Placement::NEW_PLATE);
					}
					if (constraintCheck(old, cur_sol, node_a0)) {
						branch_nodes.push_back(node_a0);
					}
				}
				else if(slip_r > old.c1cpr) { // item conflict with defect (two branches).
					TreeNode node_a1(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2 | Placement::DEFECT_R);
					node_a1.c3cp = node_a1.c1cpr = slip_r + item.w;
					node_a1.c4cp = node_a1.c2cpu = item.h;
					if (old.item == Problem::InvalidItemId && old.c1cpr == 0) {
						node_a1.setFlagBit(Placement::NEW_PLATE);
					}
					if (constraintCheck(old, cur_sol, node_a1)) {	
						branch_nodes.push_back(node_a1);
					}

					slip_u = sliptoDefectUp(old.c1cpr, 0, item.w, item.h);
					TreeNode node_a2(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2 | Placement::DEFECT_U);
					node_a2.c3cp = node_a2.c1cpr = old.c1cpr + item.w;
					node_a2.c4cp = node_a2.c2cpu = slip_u + item.h;
					if (old.item == Problem::InvalidItemId && old.c1cpr == 0) {
						node_a2.setFlagBit(Placement::NEW_PLATE);
					}
					if (constraintCheck(old, cur_sol, node_a2)) {
						branch_nodes.push_back(node_a2);
					}
				}
				else {
					assert(!(slip_r < old.c1cpr));
				}
			}
		}
	}
#pragma endregion CASE_A

	/*
	* case B, 向右放置则拓宽c1cpr，向上放置则开一个新的L2桶.  
	* c2cpu != 0 & c2cpb = 0 & c1cpr = c3cp, 有2-cut说明必有1-cut(c1cpl != c1cpr)，拓宽1-cut或者开一个新的2-cut
	*/
#pragma region CASE_B
	else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) {
		for (int rotate = 0; rotate <= 1; ++rotate) {
			for (int s = 0; s < batch.stack_num(); ++s) {
				itemId = batch.get(s);
				if (itemId == Problem::InvalidItemId) continue; // skip empty stack.
				Rect item = aux_.items[itemId];
				if (item.w == item.h && rotate) continue; // square item no need to rotate.
				if (rotate) { // rotate item.
					item.w = aux_.items[itemId].h;
					item.h = aux_.items[itemId].w;
				}

				TLength slip_r, slip_u;

				/* place in the right of old.c1cpr. */
#pragma region C1cprRight
				slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
				if (slip_r == old.c1cpr) { // no conflict
					TreeNode node_b0(old, itemId, rotate);
					node_b0.c3cp = node_b0.c1cpr = old.c1cpr + item.w;
					node_b0.c4cp = item.h;
					if (constraintCheck(old, cur_sol, node_b0)) {
						branch_nodes.push_back(node_b0);
					}
				}
				else if (slip_r > old.c1cpr) {
					TreeNode node_b1(old, itemId, rotate | Placement::DEFECT_R);
					node_b1.c3cp = node_b1.c1cpr = slip_r + item.w;
					node_b1.c4cp = item.h;
					if (constraintCheck(old, cur_sol, node_b1)) {
						branch_nodes.push_back(node_b1);
					}

					// [TODO] TCoord old_item_y = old.c2cpu - item.h > aux_.param.minWasteHeight ? old.c2cpu - item.h : 0;
					TCoord old_item_y = 0;
					slip_u = sliptoDefectUp(old.c1cpr, old_item_y, item.w, item.h);
					TreeNode node_b2(old, itemId, rotate | Placement::DEFECT_U);
					node_b2.c3cp = node_b2.c1cpr = old.c1cpr + item.w;
					node_b2.c4cp = slip_u + item.h;
					if (constraintCheck(old, cur_sol, node_b2)) {
						branch_nodes.push_back(node_b2);
					}
				}
				else {
					assert(!(slip_r < old.c1cpr));
				}
#pragma endregion C1cprRight
				
				/* place in the upper of old.c2cpu. */
#pragma region C2cpuUpper
				slip_r = sliptoDefectRight(old.c1cpl, old.c2cpu, item.w, item.h);
				if (slip_r == old.c1cpl) { // no conflict
					TreeNode node_b0(old, itemId, rotate | Placement::NEW_L2);
					node_b0.c2cpb = old.c2cpu;
					node_b0.c4cp = node_b0.c2cpu = old.c2cpu + item.h;
					node_b0.c3cp = old.c1cpl + item.w;
					node_b0.c1cpr = max(node_b0.c1cpr, node_b0.c3cp);
					if (constraintCheck(old, cur_sol, node_b0)) {
						branch_nodes.push_back(node_b0);
					}
				}
				else if (slip_r > old.c1cpl) {
					TreeNode node_b3(old, itemId, rotate | Placement::NEW_L2 | Placement::DEFECT_R);
					node_b3.c2cpb = old.c2cpu;
					node_b3.c4cp = node_b3.c2cpu = old.c2cpu + item.h;
					node_b3.c3cp = slip_r + item.w;
					node_b3.c1cpr = max(node_b3.c1cpr, node_b3.c3cp);
					if (constraintCheck(old, cur_sol, node_b3)) {
						branch_nodes.push_back(node_b3);
					}

					slip_u = sliptoDefectUp(old.c1cpl, old.c2cpu, item.w, item.h);
					TreeNode node_b4(old, itemId, rotate | Placement::NEW_L2 | Placement::DEFECT_U);
					node_b4.c2cpb = old.c2cpu;
					node_b4.c4cp = node_b4.c2cpu = slip_u + item.h;
					node_b4.c3cp = old.c1cpl + item.w;
					node_b4.c1cpr = max(node_b4.c1cpr, node_b4.c3cp);
					if (constraintCheck(old, cur_sol, node_b4)) {
						branch_nodes.push_back(node_b4);
					}
				}
				else {
					assert(!(slip_r < old.c1cpl));
				}
#pragma endregion C2cpuUpper
			}
		}
	}
#pragma endregion CASE_B

	/*
	* case C，将物品放在L2桶中，或者新开一个L2桶（L1桶撑大太多）.
	* [opt] 使用利用率评估是否新开L2桶
	* c2cpu != 0 & (c2cpb != 0 || c1cpr != c3cp)
	*/
#pragma region CASE_C
	else {
		assert(old.c2cpb != 0); // c1cpr != c3cp，说明必有 c2cpb != 0，按照 c1cpr ?= c3cp 分类讨论
		const bool c2cpu_locked = old.getFlagBit(Placement::LOCKC2); // status: 当前L2上界不能移动
		for (int rotate = 0; rotate <= 1; ++rotate) {
			for (int s = 0; s < batch.stack_num(); ++s) {
				itemId = batch.get(s);
				if (itemId == Problem::InvalidItemId) continue; // skip empty stack.
				Rect item = aux_.items[itemId];
				if (item.w == item.h && rotate) continue; // square item no need to rotate.
				if (rotate) { // rotate item.
					item.w = aux_.items[itemId].h;
					item.h = aux_.items[itemId].w;
				}
				// if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
				TLength slip_r, slip_u;

				/* place item in L4 bin. */
#pragma region C4cpUpper
				TLength old_item_w = old.getFlagBit(Placement::ROTATE) ? aux_.items[old.item].h : aux_.items[old.item].w;
				TCoord  old_item_x = old.c3cp - old_item_w;
				if (old.c2cpu > old.c4cp && old_item_w == item.w && !old.getFlagBit(Placement::DEFECT_U) 
					&& ((c2cpu_locked && old.c4cp + item.h == old.c2cpu) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpu))
					&& (sliptoDefectRight(old_item_x, old.c4cp, item.w, item.h) == old_item_x)
					) {
					TreeNode node_c1(old, itemId, rotate | Placement::BIN4 | Placement::LOCKC2); 
					node_c1.c2cpu = node_c1.c4cp = old.c4cp + item.h;
					if (constraintCheck(old, cur_sol, node_c1)) {
						branch_nodes.push_back(node_c1);
						continue; // best placement
					}
				}
#pragma endregion C4cpUpper

				/* c1cpr != c3cp, place item in the right of current c3cp. */
#pragma region C3cpRight
				if (old.c1cpr != old.c3cp) {
					slip_r = sliptoDefectRight(old.c3cp, old.c2cpb, item.w, item.h);
					if (slip_r == old.c3cp) {
						// when c2cpu is locked, old.c2cpb + item.h <= old.c2cpu constraint must meet.
						if (!c2cpu_locked || old.c2cpb + item.h <= old.c2cpu) {
							TreeNode node_c0(old, itemId, rotate);
							node_c0.c3cp = old.c3cp + item.w;
							node_c0.c1cpr = max(node_c0.c1cpr, node_c0.c3cp);
							node_c0.c4cp = old.c2cpb + item.h;
							if (c2cpu_locked) { node_c0.setFlagBit(Placement::LOCKC2); }
							if (constraintCheck(old, cur_sol, node_c0)) {
								branch_nodes.push_back(node_c0);
							}
						}
					}
					else if (slip_r > old.c3cp) {
						if (!c2cpu_locked || old.c2cpb + item.h <= old.c2cpu) {
							TreeNode node_c2(old, itemId, rotate | Placement::DEFECT_R);
							node_c2.c3cp = slip_r + item.w;
							node_c2.c1cpr = max(node_c2.c1cpr, node_c2.c3cp);
							node_c2.c4cp = old.c2cpb + item.h;
							if (c2cpu_locked) { node_c2.setFlagBit(Placement::LOCKC2); }
							if (constraintCheck(old, cur_sol, node_c2)) {
								branch_nodes.push_back(node_c2);
							}
						}

						// [TODO] TCoord old_item_y = old.c2cpu - item.h > old.c2cpb + aux_.param.minWasteHeight ? old.c2cpu - item.h : old.c2cpb;
						TCoord old_item_y = old.c2cpb;
						slip_u = sliptoDefectUp(old.c3cp, old_item_y, item.w, item.h);
						if (!c2cpu_locked || slip_u + item.h <= old.c2cpu) {
							TreeNode node_c3(old, itemId, rotate | Placement::DEFECT_U);
							node_c3.c3cp = old.c3cp + item.w;
							node_c3.c1cpr = max(node_c3.c1cpr, node_c3.c3cp);
							node_c3.c4cp = slip_u + item.h;
							if (c2cpu_locked) { node_c3.setFlagBit(Placement::LOCKC2); }
							if (constraintCheck(old, cur_sol, node_c3)) {
								branch_nodes.push_back(node_c3);
							}
						}
					}
					else {
						assert(!(slip_r < old.c3cp));
					}
				}
#pragma endregion C3cpRight

				bool is_new_L2 = false; // successfully creat a new L2 ?
				/* c3cp + item.w > c1cpr, creat a new L2 and place item in it. */
#pragma region C2cpuUpper
				// [TODO] else
				if (old.c3cp + item.w > old.c1cpr) {
					slip_r = sliptoDefectRight(old.c1cpl, old.c2cpu, item.w, item.h);
					if (slip_r == old.c1cpl) {
						TreeNode node_c0(old, itemId, rotate | Placement::NEW_L2);
						node_c0.c3cp = old.c1cpl + item.w;
						node_c0.c1cpr = max(node_c0.c1cpr, node_c0.c3cp);
						node_c0.c2cpb = old.c2cpu;
						node_c0.c4cp = node_c0.c2cpu = old.c2cpu + item.h;
						if (constraintCheck(old, cur_sol, node_c0)) {
							branch_nodes.push_back(node_c0);
							is_new_L2 = true;
						}
					}
					else if (slip_r > old.c1cpl) {
						TreeNode node_c4(old, itemId, rotate | Placement::NEW_L2 | Placement::DEFECT_R);
						node_c4.c3cp = slip_r + item.w;
						node_c4.c1cpr = max(node_c4.c1cpr, node_c4.c3cp);
						node_c4.c2cpb = old.c2cpu;
						node_c4.c4cp = node_c4.c2cpu = old.c2cpu + item.h;
						if (constraintCheck(old, cur_sol, node_c4)) {
							branch_nodes.push_back(node_c4);
							is_new_L2 = true;
						}

						slip_u = sliptoDefectUp(old.c1cpl, old.c2cpu, item.w, item.h);
						TreeNode node_c5(old, itemId, rotate | Placement::NEW_L2 | Placement::DEFECT_U);
						node_c5.c3cp = old.c1cpl + item.w;
						node_c5.c1cpr = max(node_c5.c1cpr, node_c5.c3cp);
						node_c5.c2cpb = old.c2cpu;
						node_c5.c4cp = node_c5.c2cpu = slip_u + item.h;
						if (constraintCheck(old, cur_sol, node_c5)) {
							branch_nodes.push_back(node_c5);
							is_new_L2 = true;
						}
					}
					else {
						assert(!(slip_r < old.c1cpl));
					}
				}
#pragma endregion C2cpuUpper

				if (!opt_tail || is_new_L2) continue; // if item can placed in a new L2, no need to place it in a new L1.

				/* creat a new L1 and place item in it. */
#pragma region C1cprRight
				slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
				if (slip_r == old.c1cpr) {
					TreeNode node_c0(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2);
					node_c0.c1cpl = old.c1cpr;
					node_c0.c3cp = node_c0.c1cpr = old.c1cpr + item.w;
					node_c0.c2cpb = 0;
					node_c0.c4cp = node_c0.c2cpu = item.h;
					if (constraintCheck(old, cur_sol, node_c0)) {
						branch_nodes.push_back(node_c0);
					}
				}
				else if (slip_r > old.c1cpr) {
					TreeNode node_c6(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2 | Placement::DEFECT_R);
					node_c6.c1cpl = old.c1cpr;
					node_c6.c3cp = node_c6.c1cpr = slip_r + item.w;
					node_c6.c2cpb = 0;
					node_c6.c4cp = node_c6.c2cpu = item.h;
					if (constraintCheck(old, cur_sol, node_c6)) {
						branch_nodes.push_back(node_c6);
					}

					slip_u = sliptoDefectUp(old.c1cpr, 0, item.w, item.h);
					TreeNode node_c7(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2 | Placement::DEFECT_U);
					node_c7.c1cpl = old.c1cpr;
					node_c7.c3cp = node_c7.c1cpr = old.c1cpr + item.w;
					node_c7.c2cpb = 0;
					node_c7.c4cp = node_c7.c2cpu = slip_u + item.h;
					if (constraintCheck(old, cur_sol, node_c7)) {
						branch_nodes.push_back(node_c7);
					}
				}
				else {
					assert(!(slip_r < old.c1cpr));
				}
#pragma endregion C1cprRight
			}
		}
	}
#pragma endregion CASE_C
}

const bool MyCutSearch::constraintCheck(const TreeNode & old, const List<TreeNode> &cur_sol, TreeNode & node) {
	bool moved_cut1 = false, moved_cut2 = false;
	// [?] 必要性？ if c2cpu less than c4cp, move it up.
	if (node.c2cpu < node.c4cp) {
		node.c2cpu = node.c4cp;
		moved_cut2 = true;
	}
#pragma region minWasteChecker
	// check if item right side and c1cpr interval less than minWasteWidth.
	if (node.c3cp != node.c1cpr && node.c1cpr - node.c3cp < aux_.param.minWasteWidth) {
		node.c1cpr += aux_.param.minWasteWidth; // move c1cpr right to staisfy constraint.
		moved_cut1 = true;
	}
	// check if new c1cpr and old c1cpr interval less than minWasteWidth.
	if (node.c1cpr != old.c1cpr && node.c1cpr - old.c1cpr < aux_.param.minWasteWidth) {
		node.c1cpr += aux_.param.minWasteWidth;
		moved_cut1 = true;
	}
	// check if item up side and c2cpu interval less than minWasteHeight.
	if (node.c4cp != node.c2cpu && node.c2cpu - node.c4cp < aux_.param.minWasteHeight) {
		if (node.getFlagBit(Placement::LOCKC2)) { // c2cpu cant's move in this case.
			return false;
		}
		else {
			node.c2cpu += aux_.param.minWasteHeight; // move c2cpu up to satisfy constraint.
			if (node.getFlagBit(Placement::DEFECT_U))
				node.c4cp = node.c2cpu;
			moved_cut2 = true;
		}
	}
	// check if new c2cpu and old c2cpu interval less than minWasteHeight
	if (node.c2cpu != old.c2cpu && node.c2cpu - old.c2cpu < aux_.param.minWasteHeight) {
		if (node.getFlagBit(Placement::LOCKC2)) {
			return false;
		}
		else {
			node.c2cpu += aux_.param.minWasteHeight;
			if (node.getFlagBit(Placement::DEFECT_U))
				node.c4cp = node.c2cpu;
			moved_cut2 = true;
		}
	}
#pragma endregion minWasteChecker

#pragma region defectChecker
	auto & defect_x = aux_.plate_defect_x[plate_id_];
	auto & defect_y = aux_.plate_defect_y[plate_id_];
#pragma region c3cpChecker
	for (auto it = defect_x.begin(); it != defect_x.end(); ++it) {
		if (node.c3cp <= it->x) {           // defect in c3cp right.
			break;
		}
		if (!(node.c3cp >= it->x + it->w  // defect in c3cp left.
			|| node.c2cpb >= it->y + it->h  // defect in item bottom.
			|| node.c2cpu <= it->y)         // defect in item up.
			) {
			return false;
		}
	}
#pragma endregion c3cpChecker

	// [TODO] [opt]
#pragma region cut1ThroughDefectChecker
	if (moved_cut1 || old.c1cpr != node.c1cpr) {
		for (auto it = defect_x.begin(); it != defect_x.end(); ++it) {
			if (it->x >= node.c1cpr) break;
			if (it->x + it->w > node.c1cpr) { // c1cpr cut through defect.
				if (it->x + it->w - node.c1cpr < aux_.param.minWasteWidth) {
					if (!moved_cut1)
						node.c1cpr += aux_.param.minWasteWidth;
					else
						node.c1cpr = it->x + it->w; // move c1cpr -> defect right side to satisfy the constraint.
					moved_cut1 = true;
				}
				else {
					return false;
				}
			}
		}
		if (moved_cut1) {
			if (defectConflictArea(node.c1cpl, node.c2cpb, node.c1cpr - node.c1cpl, 0)) {
				return false;
			}
			for (auto r_iter = cur_sol.rbegin(); r_iter != cur_sol.rend(); ++r_iter) {
				if (r_iter->getFlagBit(Placement::NEW_L1) || r_iter->getFlagBit(Placement::NEW_PLATE)) {
					break;
				}
				if (r_iter->getFlagBit(Placement::NEW_L2)) {
					if (defectConflictArea(node.c1cpl, r_iter->c2cpb, node.c1cpr - node.c1cpl, 0)) {
						return false;
					}
				}
			}
		}
	}
#pragma endregion cut1ThroughDefectChecker

	// [TODO] [opt]
#pragma region cut2ThroughDefectChecker
	if (moved_cut2 || old.c2cpu != node.c2cpu || old.c1cpr != node.c1cpr) {
		for (auto it = defect_y.begin(); it != defect_y.end(); ++it) {
			if (it->y < node.c2cpu) {
				if (it->y + it->h > node.c2cpu && node.c1cpl < it->x + it->w && node.c1cpr > it->x) { // c2cpu cut through defect.
					if (node.getFlagBit(Placement::LOCKC2) || // can't move c2cpu to satisfy the constraint.
						it->y + it->h - node.c2cpu < aux_.param.minWasteHeight) {
						return false;
					}
					else {
						if (it->y + it->h - node.c2cpu < aux_.param.minWasteHeight && !moved_cut2)
							node.c2cpu += aux_.param.minWasteHeight;
						else
							node.c2cpu = it->y + it->h; // move c2cpu -> defect up side to satisfy the constraint.
						if (node.getFlagBit(Placement::DEFECT_U))
							node.c4cp = node.c2cpu;
						moved_cut2 = true;
					}
				}
			}
			else { // the leftover defects is in the upper of c2cpu.
				break;
			}
		}
	}
	// search back the current solution to check if item(placed in defect upper) conflict with defect after slip up.
	if (moved_cut2) {
		// check if current consider node conflict with defect after move.
		if (node.getFlagBit(Placement::DEFECT_U)) {
			const TLength item_w = node.getFlagBit(Placement::ROTATE) ? aux_.items[node.item].h : aux_.items[node.item].w;
			const TLength item_h = node.getFlagBit(Placement::ROTATE) ? aux_.items[node.item].w : aux_.items[node.item].h;
			if (defectConflictArea(node.c3cp - item_w, node.c2cpu - item_h, item_w, item_h)) {
				return false;
			}
		}
		// check current partial solution
		for (auto r_iter = cur_sol.rbegin(); r_iter != cur_sol.rend(); ++r_iter) {
			if (r_iter->getFlagBit(Placement::NEW_L2) || r_iter->getFlagBit(Placement::NEW_L1) || r_iter->getFlagBit(Placement::NEW_PLATE)) {
				break; // the checked item should be in the same cut2.
			}
			if (r_iter->getFlagBit(Placement::DEFECT_U)) {
				const TLength item_w = r_iter->getFlagBit(Placement::ROTATE) ? aux_.items[r_iter->item].h : aux_.items[r_iter->item].w;
				const TLength item_h = r_iter->getFlagBit(Placement::ROTATE) ? aux_.items[r_iter->item].w : aux_.items[r_iter->item].h;
				if (defectConflictArea(r_iter->c3cp - item_w, node.c2cpu - item_h, item_w, item_h)) {
					return false;
				}
			}
			// check if (old.c3cp,old.c2cpb,0,node.c2cpu-old.c2cpb) conflict with defect.
			if (defectConflictArea(r_iter->c3cp, r_iter->c2cpb, 0, node.c2cpu - r_iter->c2cpb)) {
				return false;
			}
		}
	}
#pragma endregion cut2ThroughDefectChecker
#pragma endregion defectChecker

#pragma region sideChecker
	// check if cut1 exceed stock right side.
	if (node.c1cpr > aux_.param.plateWidth) {
		return false;
	}
	// check if cut2 exceed stock up side.
	if (node.c2cpu > aux_.param.plateHeight) {
		return false;
	}
	// check if cut1 and stock right side interval less than minimum waste width.
	if (aux_.param.plateWidth != node.c1cpr && aux_.param.plateWidth - node.c1cpr < aux_.param.minWasteWidth) {
		return false;
	}
	// check if cut2 and stock up side interval less than minimum waste height.
	if (aux_.param.plateHeight != node.c2cpu && aux_.param.plateHeight - node.c2cpu < aux_.param.minWasteHeight) {
		return false;
	}
#pragma endregion sideChecker

#pragma region cutWidthAndHeightChecker
	// check if maximum cut1 width not staisfy.
	if (node.c1cpr - node.c1cpl > aux_.param.maxL1Width) {
		return false;
	}
	// check if minimum cut1 width not staisfy.
	if (node.getFlagBit(Placement::NEW_L1) && old.item != Problem::InvalidItemId && old.c1cpr - old.c1cpl < aux_.param.minL1Width) {
		return false;
	}
	// check if minimum cut2 height not staisfy.
	if (node.getFlagBit(Placement::NEW_L2) && old.item != Problem::InvalidItemId && old.c2cpu - old.c2cpb < aux_.param.minL2Height) {
		return false;
	}
#pragma endregion cutWidthAndHeightChecker

	return true; // every constraints satisfied.
}

const TCoord MyCutSearch::sliptoDefectRight(const TCoord x, const TCoord y, const TLength w, const TLength h) const {
	TCoord slip = x;
	auto & defect_x = aux_.plate_defect_x[plate_id_];
	for (auto it = defect_x.begin(); it != defect_x.end(); ++it) {
		if (it->x >= slip + w) {
			break; // the defects is sorted by x position, so in this case just break.
		}
		if (!( slip - it->x >= it->w  // defect in item left.
			|| it->y - y >= h         // defect in item up.
			|| y - it->y >= it->h)    // defect in item bottom.
			) { 
			slip = it->x + it->w; 
			if (slip != x && slip - x < aux_.param.minWasteWidth) {
				slip = x + aux_.param.minWasteWidth;
			}
		}
	}
	return slip;
}

const TCoord MyCutSearch::sliptoDefectUp(const TCoord x, const TCoord y, const TLength w, const TLength h) const {
	TCoord slip = y;
	auto & defect_y = aux_.plate_defect_y[plate_id_];
	for (auto it = defect_y.begin(); it != defect_y.end(); ++it) {
		if (it->y >= slip + h) {
			break; // the defects is sorted by y position, so in this case just break.
		}
		if (!( it->x - x >= w         // defect in item right.
			|| x - it->x >= it->w     // defect in item left.
			|| slip - it->y >= it->h) // defect in item bottom.
			) { 
			slip = it->y + it->h;
			if (slip != y && slip - y < aux_.param.minWasteHeight) {
				slip = y + aux_.param.minWasteHeight;
			}
		}
	}
	return slip;
}

const bool MyCutSearch::defectConflictArea(const TCoord x, const TCoord y, const TLength w, const TLength h) const {
	auto & defect_y = aux_.plate_defect_y[plate_id_];
	for (auto it = defect_y.begin(); it != defect_y.end(); ++it) {
		if (it->y >= y + h) {
			break;
		}
		if (!( it->x - x >= w          // defect in item right.
			|| x - it->x >= it->w      // defect in item left.
			|| y - it->y >= it->h)) {  // defect in item bottom.
			return true;
		}
	}
	return false;
}

}
