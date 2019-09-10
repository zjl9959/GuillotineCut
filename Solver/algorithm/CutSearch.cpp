#include "CutSearch.h"

using namespace std;

namespace szx {

/* 优化1-cut
   输入：batch（物品栈），opt_tail（是否优化原料末尾）
   输出：sol（该1-cut的最优解），返回：1-cut对应利用率 */
Score CutSearch::run(Batch &batch, Solution &sol, bool opt_tail) {
    return dfs(batch, sol, opt_tail);
}

Score CutSearch::dfs(Batch &batch, Solution &sol, bool opt_tail) {
    List<TreeNode> live_nodes; live_nodes.reserve(100); // 待分支的树节点
    List<TreeNode> cur_sol; cur_sol.reserve(20);        // 当前解
    List<TreeNode> best_sol; Score best_obj = 0.0;      // 最优解及对应目标函数值
    
    Depth pre_depth = -1;    // 上一个节点深度
    Area used_item_area = 0; // 已使用物品面积
    const Area tail_area = (param_.plateWidth - start_pos_)*param_.plateHeight;

    Timer timer(static_cast<chrono::milliseconds>(timeout_ms_));
    int see_timeout = 100000;   // 检查是否超时间隔时间

    branch(TreeNode(start_pos_), batch, live_nodes, opt_tail);
    while (live_nodes.size()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop_back();
        if (node.depth - pre_depth == 1) { // 向下扩展
            cur_sol.push_back(node);
            batch.pop(node.item);
            used_item_area += item_area_[node.item];
        } else if (node.depth - pre_depth < 1) { // 向上回溯
            for (int i = static_cast<int>(cur_sol.size()) - 1; i >= node.depth; --i) {
                batch.push(cur_sol[i].item);    // 恢复栈
                used_item_area -= item_area_[cur_sol[i].item];  // 更新使用物品面积
            }
            cur_sol.erase(cur_sol.begin() + node.depth, cur_sol.end());
            cur_sol.push_back(node); // 更新当前解
            batch.pop(node.item);
            used_item_area += item_area_[node.item];
        }
        const size_t old_live_nodes_size = live_nodes.size();
        branch(node, batch, live_nodes, opt_tail);
        pre_depth = node.depth;
        if (old_live_nodes_size == live_nodes.size()) { // 找到一个完整解
            TLength cur_width = cur_sol.back().c1cpr - start_pos_;
            Score cur_obj = 0.0;
            if (opt_tail)
                cur_obj = (double)used_item_area / (double)tail_area;
            else
                cur_obj = (double)used_item_area / (double)(cur_width*param_.plateHeight);
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
        for (auto it = best_sol.begin(); it != best_sol.end(); ++it)
            sol.push_back(*it);
        return best_obj;
    }
    return 0.0;
}

Score CutSearch::pfs(Batch &batch, Solution &sol, bool opt_tail) {
    // [zjl][TODO]: add pfs implement.
    return 0.0;
}

void CutSearch::branch(const TreeNode &old, const Batch &batch, List<TreeNode> &branch_nodes, bool opt_tail) {
    const bool c2cpu_locked = old.getFlagBit(Placement::LOCKC2); // status: current c2cpu was locked.
    TID itemId = Problem::InvalidItemId;
    // case A, place item in a new L1.
    if (old.c2cpu == 0) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (TID i = 0; i < batch.stack_num(); ++i) {
                itemId = batch.get(i);
                if (itemId == Problem::InvalidItemId)continue;
                // pretreatment.
                Rect item = items_[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                if (old.c1cpr + item.w > param_.plateWidth)continue;
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // TODO: precheck if item exceed plate bound to speed up branch.
                // check if item conflict with defect and try to fix it.
                slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                {
                    // place in defect right(if no defect conflict, slip_r is 0).
                    TreeNode node_a1(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2);
                    node_a1.c3cp = node_a1.c1cpr = slip_r + item.w;
                    node_a1.c4cp = node_a1.c2cpu = item.h;
                    if (slip_r > old.c1cpr)
                        node_a1.setFlagBit(Placement::DEFECT_R);
                    if (constraintCheck(old, node_a1)) {
                        if (old.item == Problem::InvalidItemId && old.c1cpr == 0)
                            node_a1.setFlagBit(Placement::NEW_PLATE);
                        branch_nodes.push_back(node_a1);
                    }
                }
                if (slip_r > old.c1cpr) { // item conflict with defect(two choice).
                    slip_u = sliptoDefectUp(old.c1cpr, 0, item.w);
                    if (slip_u != -1) {
                        TreeNode node_a2(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2); // place in defect up.
                        node_a2.c3cp = node_a2.c1cpr = old.c1cpr + item.w;
                        node_a2.c4cp = node_a2.c2cpu = slip_u + item.h;
                        node_a2.setFlagBit(Placement::DEFECT_U);
                        if (constraintCheck(old, node_a2)) {
                            if (old.item == Problem::InvalidItemId && old.c1cpr == 0)
                                node_a2.setFlagBit(Placement::NEW_PLATE);
                            branch_nodes.push_back(node_a2);
                        }
                    }
                }
            }
        }
    }
    // case B, extend c1cpr or place item in a new L2.
    else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (TID i = 0; i < batch.stack_num(); ++i) {
                itemId = batch.get(i);
                if (itemId == Problem::InvalidItemId)continue;
                // pretreatment.
                Rect item = items_[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place in the right of old.c1cpr.
                slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                {
                    TreeNode node_b1(old, itemId, rotate);
                    node_b1.c3cp = node_b1.c1cpr = slip_r + item.w;
                    node_b1.c4cp = item.h;
                    if (slip_r > old.c1cpr)
                        node_b1.setFlagBit(Placement::DEFECT_R);
                    if (constraintCheck(old, node_b1)) {
                        branch_nodes.push_back(node_b1);
                    }
                }
                if (slip_r > old.c1cpr) {
                    slip_u = sliptoDefectUp(old.c1cpr, old.c2cpu - item.h > param_.minWasteHeight ?
                        old.c2cpu - item.h : 0, item.w);    // [zjl][TODO]:consider more.
                    if (slip_u != -1) {
                        TreeNode node_b2(old, itemId, rotate);
                        node_b2.c3cp = node_b2.c1cpr = old.c1cpr + item.w;
                        node_b2.c4cp = slip_u + item.h;
                        node_b2.setFlagBit(Placement::DEFECT_U);
                        if (constraintCheck(old, node_b2)) {
                            branch_nodes.push_back(node_b2);
                        }
                    }
                }
                // place in the upper of old.c2cpu.
                slip_r = sliptoDefectRight(old.c1cpl, old.c2cpu, item.w, item.h);
                {
                    TreeNode node_b3(old, itemId, rotate);
                    node_b3.c2cpb = old.c2cpu;
                    node_b3.c4cp = node_b3.c2cpu = old.c2cpu + item.h;
                    node_b3.c3cp = slip_r + item.w;
                    if (node_b3.c1cpr < node_b3.c3cp)
                        node_b3.c1cpr = node_b3.c3cp;
                    node_b3.setFlagBit(Placement::NEW_L2);
                    if (slip_r > old.c1cpl)
                        node_b3.setFlagBit(Placement::DEFECT_R);
                    if (constraintCheck(old, node_b3)) {
                        branch_nodes.push_back(node_b3);
                    }
                }
                if (slip_r > old.c1cpl) {
                    slip_u = sliptoDefectUp(old.c1cpl, old.c2cpu, item.w);
                    if (slip_u != -1) {
                        TreeNode node_b4(old, itemId, rotate);
                        node_b4.c2cpb = old.c2cpu;
                        node_b4.c4cp = node_b4.c2cpu = slip_u + item.h;
                        node_b4.c3cp = old.c1cpl + item.w;
                        if (node_b4.c1cpr < node_b4.c3cp)
                            node_b4.c1cpr = node_b4.c3cp;
                        node_b4.setFlagBit(Placement::NEW_L2);
                        node_b4.setFlagBit(Placement::DEFECT_U);
                        if (constraintCheck(old, node_b4)) {
                            branch_nodes.push_back(node_b4);
                        }
                    }
                }
            }
        }
    }
    // case C, place item in the old L2 or a new L2 when item extend c1cpr too much.
    else {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (TID i = 0; i < batch.stack_num(); ++i) {
                itemId = batch.get(i);
                if (itemId == Problem::InvalidItemId)continue;
                // pretreatment.
                Rect item = items_[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place item in the up of current c4cp.
                TLength last_item_w = old.getFlagBit(Placement::ROTATE) ? items_[old.item].h : items_[old.item].w;
                if (old.c2cpu > old.c4cp && last_item_w == item.w && !old.getFlagBit(Placement::DEFECT_U) &&
                    ((c2cpu_locked && old.c4cp + item.h == old.c2cpb) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpb)) &&
                    !sliptoDefectRight(old.c3cp - item.w, old.c4cp, item.w, item.h)) {
                    TreeNode node_c1(old, itemId, rotate| Placement::BIN4| Placement::LOCKC2); // place item in L4 bin.
                    node_c1.c2cpu = node_c1.c4cp = old.c4cp + item.h;
                    if (constraintCheck(old, node_c1)) {
                        branch_nodes.push_back(node_c1);
                    }
                    continue;
                }
                if (old.c1cpr > old.c3cp) { // place item in the right of current c3cp.
                    slip_r = sliptoDefectRight(old.c3cp, old.c2cpb, item.w, item.h);
                    // when c2cpu is locked, old.c2cpb + item.h <= old.c2cpu constraint must meet.
                    if (!c2cpu_locked || (c2cpu_locked && old.c2cpb + item.h <= old.c2cpu)) {
                        TreeNode node_c2(old, itemId, rotate);
                        node_c2.c3cp = slip_r + item.w;
                        if (node_c2.c1cpr < node_c2.c3cp)
                            node_c2.c1cpr = node_c2.c3cp;
                        node_c2.c4cp = old.c2cpb + item.h;
                        if (slip_r > old.c3cp)
                            node_c2.setFlagBit(Placement::DEFECT_R);
                        if (c2cpu_locked)
                            node_c2.setFlagBit(Placement::LOCKC2);
                        if (constraintCheck(old, node_c2)) {
                            branch_nodes.push_back(node_c2);
                        }
                    }
                    if (slip_r > old.c3cp) {
                        slip_u = sliptoDefectUp(old.c3cp, old.c2cpu - item.h > old.c2cpb + param_.minWasteHeight ?
                            old.c2cpu - item.h : old.c2cpb, item.w);
                        if (slip_u != -1) {
                            if (!c2cpu_locked || (c2cpu_locked && slip_u + item.h <= old.c2cpu)) {
                                TreeNode node_c3(old, itemId, rotate | Placement::DEFECT_U);
                                node_c3.c3cp = old.c3cp + item.w;
                                if (node_c3.c1cpr < node_c3.c3cp)
                                    node_c3.c1cpr = node_c3.c3cp;
                                node_c3.c4cp = item.h + slip_u;
                                if (c2cpu_locked)
                                    node_c3.setFlagBit(Placement::LOCKC2);
                                if (constraintCheck(old, node_c3)) {
                                    branch_nodes.push_back(node_c3);
                                }
                            }
                        }
                    }
                }
                if (old.c3cp + item.w > old.c1cpr) {
                    bool flag = false;
                    // creat a new L2 and place item in it.
                    slip_r = sliptoDefectRight(old.c1cpl, old.c2cpu, item.w, item.h);
                    {
                        TreeNode node_c4(old, itemId, rotate| Placement::NEW_L2);
                        node_c4.c3cp = slip_r + item.w;
                        if (node_c4.c1cpr < node_c4.c3cp)
                            node_c4.c1cpr = node_c4.c3cp;
                        node_c4.c2cpb = old.c2cpu;
                        node_c4.c4cp = node_c4.c2cpu = old.c2cpu + item.h;
                        if (slip_r > old.c1cpl)
                            node_c4.setFlagBit(Placement::DEFECT_R);
                        if (constraintCheck(old, node_c4)) {
                            branch_nodes.push_back(node_c4);
                            flag = true;
                        }
                    }
                    if (slip_r > old.c1cpl) {
                        slip_u = sliptoDefectUp(old.c1cpl, old.c2cpu, item.w);
                        if (slip_u != -1) {
                            TreeNode node_c5(old, itemId, rotate | Placement::NEW_L2 | Placement::DEFECT_U);
                            node_c5.c3cp = old.c1cpl + item.w;
                            if (node_c5.c1cpr < node_c5.c3cp)
                                node_c5.c1cpr = node_c5.c3cp;
                            node_c5.c2cpb = old.c2cpu;
                            node_c5.c4cp = node_c5.c2cpu = slip_u + item.h;
                            if (constraintCheck(old, node_c5)) {
                                branch_nodes.push_back(node_c5);
                                flag = true;
                            }
                        }
                    }
                    if (!opt_tail || flag)continue; // if item can placed in a new L2, no need to place it in a new L1.
                    // creat a new L1 and place item in it.
                    slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                    {
                        TreeNode node_c6(old, itemId, rotate| Placement::NEW_L1| Placement::NEW_L2);
                        node_c6.c1cpl = old.c1cpr;
                        node_c6.c3cp = node_c6.c1cpr = item.w + slip_r;
                        node_c6.c2cpb = 0;
                        node_c6.c4cp = node_c6.c2cpu = item.h;
                        if (slip_r > old.c1cpr)
                            node_c6.setFlagBit(Placement::DEFECT_R);
                        if (constraintCheck(old, node_c6)) {
                            branch_nodes.push_back(node_c6);
                        }
                    }
                    if (slip_r > old.c1cpr) {
                        slip_u = sliptoDefectUp(old.c1cpr, 0, item.w);
                        if (slip_u != -1) {
                            TreeNode node_c7(old, itemId, rotate | Placement::DEFECT_U | Placement::NEW_L1 | Placement::NEW_L2);
                            node_c7.c1cpl = old.c1cpr;
                            node_c7.c3cp = node_c7.c1cpr = old.c1cpr + item.w;
                            node_c7.c2cpb = 0;
                            node_c7.c4cp = node_c7.c2cpu = slip_u + item.h;
                            if (constraintCheck(old, node_c7)) {
                                branch_nodes.push_back(node_c7);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* input: old branch node, current partial solution, new branch node.
   function: check if new branch satisfy all the constraints.
   if some constraints not staisfy, try to fix is by move item. */
const bool CutSearch::constraintCheck(const TreeNode &old, TreeNode &node) {
    // if c2cpu less than c4cp, move it up.
    if (node.c2cpu < node.c4cp) {
        node.c2cpu = node.c4cp;
    }
    // check if item right side and c1cpr interval less than minWasteWidth.
    if (node.c3cp != node.c1cpr && node.c1cpr - node.c3cp < param_.minWasteWidth) {
        node.c1cpr += param_.minWasteWidth; // move c1cpr right to staisfy constraint.
    }
    // check if new c1cpr and old c1cpr interval less than minWasteWidth.
    if (node.c1cpr != old.c1cpr && node.c1cpr - old.c1cpr < param_.minWasteWidth) {
        node.c1cpr += param_.minWasteWidth;
    }
    // check if item up side and c2cpu interval less than minWasteHeight.
    if (node.c2cpu != node.c4cp && node.c2cpu - node.c4cp < param_.minWasteHeight) {
        if (node.getFlagBit(Placement::LOCKC2)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu += param_.minWasteHeight; // move c2cpu up to satisfy constraint.
            if (node.getFlagBit(Placement::DEFECT_U))
                node.c4cp = node.c2cpu;
        }
    }
    // check if new c2cpu and old c2cpu interval less than minWasteHeight
    if (node.c2cpu != old.c2cpu && node.c2cpu - old.c2cpu < param_.minWasteHeight) {
        if (node.getFlagBit(Placement::LOCKC2)) {
            return false;
        } else {
            node.c2cpu += param_.minWasteHeight;
            if (node.getFlagBit(Placement::DEFECT_U))
                node.c4cp = node.c2cpu;
        }
    }
    
    node.c1cpr = cut1ThroughDefect(node.c1cpr);
    node.c2cpu = cut2ThroughDefect(node.c1cpl, node.c1cpr, node.c2cpu);

    // check if cut1 exceed stock right side.
    if (node.c1cpr > param_.plateWidth) {
        return false;
    }
    // check if cut2 exceed stock up side.
    if (node.c2cpu > param_.plateHeight) {
        return false;
    }
    // check if maximum cut1 width not staisfy.
    if (node.c1cpr - node.c1cpl > param_.maxL1Width) {
        return false;
    }
    // check if minimum cut1 width not staisfy.
    if (node.getFlagBit(Placement::NEW_L1) && old.item != Problem::InvalidItemId &&
        old.c1cpr - old.c1cpl < param_.minL1Width) {
        return false;
    }
    // check if minimum cut2 height not staisfy.
    if (node.getFlagBit(Placement::NEW_L2) && old.item != Problem::InvalidItemId &&
        old.c2cpu - old.c2cpb < param_.minL2Height) {
        return false;
    }
    // check if cut1 and stock right side interval less than minimum waste width.
    if (param_.plateWidth != node.c1cpr &&
        param_.plateWidth - node.c1cpr < param_.minWasteWidth) {
        return false;
    }
    // check if cut2 and stock up side interval less than minimum waste height.
    if (param_.plateHeight != node.c2cpu &&
        param_.plateHeight - node.c2cpu < param_.minWasteHeight) {
        return false;
    }
    return true; // every constraints satisfied.
}

// if item or 1-cut conflict with defect, move item/1-cut to the defect right side.
const TCoord CutSearch::sliptoDefectRight(const TCoord x, const TCoord y, const TLength w, const TLength h) const {
    TCoord slip = x;
    for (auto it = defect_x_.begin(); it != defect_x_.end(); ++it) {
        if (it->x >= slip + w) {
            break; // the defects is sorted by x position, so in this case just break.
        }
        // item not in defect (right or bottom or top).
        if (!(slip - it->x >= it->w || it->y - y >= h || y - it->y >= it->h)) {
            slip = it->x + it->w;
            if (slip - x < param_.minWasteWidth)
                slip = x + param_.minWasteWidth;
        }
        // check if dir-cut cut through defect.
        if (it->x < slip + w && it->x + it->w > slip + w) {
            slip = it->x + it->w - w;
            if (slip - x < param_.minWasteWidth)
                slip = x + param_.minWasteWidth;
        }
    }
    return slip;
}

// if area(x, y, w, plateHeight-y) has just one defect, slip item to defect up, or return -1.
const TCoord CutSearch::sliptoDefectUp(const TCoord x, const TCoord y, const TLength w) const {
    TCoord res = -1;
    for (auto it = defect_x_.begin(); it != defect_x_.end(); ++it) {
        if (it->x >= x + w) {
            break;
        }
        if (it->x + it->w > x && it->x < x + w && it->y + it->h > y) {
            if (res == -1) {
                res = it->y + it->h;
            } else {
                return -1;
            }
        }
    }
    return res != -1 && res - y < param_.minWasteHeight ? y + param_.minWasteHeight : res;
}

// check if 1-cut through defect, return new 1-cut x coord. [not consider minwasteWidth constraint]
const TCoord CutSearch::cut1ThroughDefect(const TCoord x) const {
    TCoord res = x;
    for (auto it = defect_x_.begin(); it != defect_x_.end(); ++it) {
        if (it->x > res)break;
        if (it->x < res && it->x + it->w > res) {   // 1-cut cut through defect.
            res = it->x + it->w;
        }
    }
    return res;
}

// check if 2-cut through defect, return new 2-cut y coord.
const TCoord CutSearch::cut2ThroughDefect(const TCoord x1, const TCoord x2, const TCoord y) const {
    TCoord res = y;
    for (auto it = defect_y_.begin(); it != defect_y_.end(); ++it) {
        if (it->y > res)break;
        if (!(it->y + it->h <= res || it->x + it->w <= x1 || it->x >= x2)) {    // 2-cut through defect.
            res = it->y + it->h;
            if (res - y < param_.minWasteHeight)
                res = y + param_.minWasteHeight;
        }
    }
    return res;
}

}
