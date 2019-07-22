#include "CutSearch.h"

using namespace std;

namespace szx {

bool CutSearch::dfs(List<List<TID>>& batch, CutNode& sol, bool opt_tail, Score gap) {
    Area batch_item_area = 0;
    Area used_item_area = 0; // current used items area.
    const Area tail_area = (GV::param.plateWidth - start_pos_)*GV::param.plateHeight;
    Timer timer(static_cast<chrono::milliseconds>(timeout_ms_));
    int see_timeout = 100000;
    List<TID> item2batch(GV::items.size());
    for (int i = 0; i < batch.size(); ++i) {
        for (auto item : batch[i]) {
            batch_item_area += GV::item_area[item];
            item2batch[item] = i;
        }
    }

    double best_obj = 0.0;
    double best_ub = 0.0;
    List<ItemNode> live_nodes; // the tree nodes to be branched.
    List<ItemNode> cur_parsol; // current partial solution, best partial solution.
    List<ItemNode> best_parsol;
    live_nodes.reserve(100);
    cur_parsol.reserve(20);
    Depth pre_depth = -1; // last node depth.
    ItemNode resume_point(start_pos_);
    
    branch(resume_point, batch, live_nodes, opt_tail);
    while (live_nodes.size()) {
        ItemNode node = live_nodes.back();
        live_nodes.pop_back();
        double ub = (double)batch_item_area / (double)((node.c1cpr - resume_point.c1cpl)*GV::param.plateHeight);
        if (best_ub < ub) best_ub = ub;
        if (ub < best_obj) continue;
        
        if (node.depth - pre_depth == 1) { // search forward.
            cur_parsol.push_back(node);
            batch[item2batch[node.item]].pop_back();
            used_item_area += GV::item_area[node.item];
        } else if (node.depth - pre_depth < 1) { // search back.
            for (int i = cur_parsol.size() - 1; i >= node.depth; --i) { // resume stack.
                batch[item2batch[cur_parsol[i].item]].push_back(
                    cur_parsol[i].item);
                used_item_area -= GV::item_area[cur_parsol[i].item];
            }
            cur_parsol.erase(cur_parsol.begin() + node.depth, cur_parsol.end()); // erase extend nodes.
            cur_parsol.push_back(node); // push current node into cur_parsol.
            batch[item2batch[node.item]].pop_back();
            used_item_area += GV::item_area[node.item];
        }
        const size_t old_live_nodes_size = live_nodes.size();
        branch(node, batch, live_nodes, opt_tail);
        pre_depth = node.depth;
        if (old_live_nodes_size == live_nodes.size()) { // fill a complate 1-cut.
            TLength cur_width = cur_parsol.back().c1cpr - start_pos_;
            double cur_obj = 0.0;
            if (opt_tail)
                cur_obj = (double)used_item_area / (double)tail_area;
            else
                cur_obj = (double)used_item_area / (double)(cur_width*GV::param.plateHeight);
            if (best_obj < cur_obj) {
                best_obj = cur_obj;
                best_parsol = cur_parsol;
            }
            if (cur_obj > best_ub - gap)
                break;
        }
        if (see_timeout == 0) {
            if (timer.isTimeOut())
                break;
            else
                see_timeout = 100000;
        }
        --see_timeout;
    }
    if (!best_parsol.empty()) {
        sol.width = best_parsol.back().c1cpr - start_pos_;
        sol.usage_rate = best_obj;
        sol.add_solution_nodes(best_parsol);
        return true;
    }
    return false;
}

bool CutSearch::pfs(List<List<TID>>& batch, CutNode & sol, bool opt_tail, Score gap) {
    // [zjl][TODO]: add pfs implement.
    return false;
}

void CutSearch::branch(const ItemNode &old, const List<List<TID>> &batch, List<ItemNode> &branch_nodes, bool opt_tail) {
    const bool c2cpu_locked = old.getFlagBit(LOCKC2); // status: current c2cpu was locked.
    // case A, place item in a new L1.
    if (old.c2cpu == 0) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = GV::items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                if (old.c1cpr + item.w > GV::param.plateWidth)continue;
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // TODO: precheck if item exceed plate bound to speed up branch.
                // check if item conflict with defect and try to fix it.
                slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                {
                    ItemNode node_a1(old, itemId, rotate|NEW_L1); // place in defect right(if no defect conflict, slip_r is 0).
                    node_a1.c3cp = node_a1.c1cpr = slip_r + item.w;
                    node_a1.c4cp = node_a1.c2cpu = item.h;
                    if (slip_r > old.c1cpr)
                        node_a1.setFlagBit(DEFECT_R);
                    if (constraintCheck(old, node_a1)) {
                        branch_nodes.push_back(node_a1);
                    }
                }
                if (slip_r > old.c1cpr) { // item conflict with defect(two choice).
                    slip_u = sliptoDefectUp(old.c1cpr, 0, item.w);
                    if (slip_u != -1) {
                        ItemNode node_a2(old, itemId, rotate | NEW_L1); // place in defect up.
                        node_a2.c3cp = node_a2.c1cpr = old.c1cpr + item.w;
                        node_a2.c4cp = node_a2.c2cpu = slip_u + item.h;
                        node_a2.setFlagBit(DEFECT_U);
                        if (constraintCheck(old, node_a2)) {
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
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = GV::items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place in the right of old.c1cpr.
                slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                {
                    ItemNode node_b1(old, itemId, rotate);
                    node_b1.c3cp = node_b1.c1cpr = slip_r + item.w;
                    node_b1.c4cp = item.h;
                    if (slip_r > old.c1cpr)
                        node_b1.setFlagBit(DEFECT_R);
                    if (constraintCheck(old, node_b1)) {
                        branch_nodes.push_back(node_b1);
                    }
                }
                if (slip_r > old.c1cpr) {
                    slip_u = sliptoDefectUp(old.c1cpr, old.c2cpu - item.h > GV::param.minWasteHeight ?
                        old.c2cpu - item.h : 0, item.w);    // [zjl][TODO]:consider more.
                    if (slip_u != -1) {
                        ItemNode node_b2(old, itemId, rotate);
                        node_b2.c3cp = node_b2.c1cpr = old.c1cpr + item.w;
                        node_b2.c4cp = slip_u + item.h;
                        node_b2.setFlagBit(DEFECT_U);
                        if (constraintCheck(old, node_b2)) {
                            branch_nodes.push_back(node_b2);
                        }
                    }
                }
                // place in the upper of old.c2cpu.
                slip_r = sliptoDefectRight(old.c1cpl, old.c2cpu, item.w, item.h);
                {
                    ItemNode node_b3(old, itemId, rotate);
                    node_b3.c2cpb = old.c2cpu;
                    node_b3.c4cp = node_b3.c2cpu = old.c2cpu + item.h;
                    node_b3.c3cp = slip_r + item.w;
                    if (node_b3.c1cpr < node_b3.c3cp)
                        node_b3.c1cpr = node_b3.c3cp;
                    node_b3.setFlagBit(NEW_L2);
                    if (slip_r > old.c1cpl)
                        node_b3.setFlagBit(DEFECT_R);
                    if (constraintCheck(old, node_b3)) {
                        branch_nodes.push_back(node_b3);
                    }
                }
                if (slip_r > old.c1cpl) {
                    slip_u = sliptoDefectUp(old.c1cpl, old.c2cpu, item.w);
                    if (slip_u != -1) {
                        ItemNode node_b4(old, itemId, rotate);
                        node_b4.c2cpb = old.c2cpu;
                        node_b4.c4cp = node_b4.c2cpu = slip_u + item.h;
                        node_b4.c3cp = old.c1cpl + item.w;
                        if (node_b4.c1cpr < node_b4.c3cp)
                            node_b4.c1cpr = node_b4.c3cp;
                        node_b4.setFlagBit(NEW_L2);
                        node_b4.setFlagBit(DEFECT_U);
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
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = GV::items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place item in the up of current c4cp.
                Length last_item_w = old.getFlagBit(ROTATE) ? GV::items[old.item].h : GV::items[old.item].w;
                if (old.c2cpu > old.c4cp && last_item_w == item.w && !old.getFlagBit(DEFECT_U) &&
                    ((c2cpu_locked && old.c4cp + item.h == old.c2cpb) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpb)) &&
                    !sliptoDefectRight(old.c3cp - item.w, old.c4cp, item.w, item.h)) {
                    ItemNode node_c1(old, itemId, rotate|BIN4|LOCKC2); // place item in L4 bin.
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
                        ItemNode node_c2(old, itemId, rotate);
                        node_c2.c3cp = slip_r + item.w;
                        if (node_c2.c1cpr < node_c2.c3cp)
                            node_c2.c1cpr = node_c2.c3cp;
                        node_c2.c4cp = old.c2cpb + item.h;
                        if (slip_r > old.c3cp)
                            node_c2.setFlagBit(DEFECT_R);
                        if (c2cpu_locked)
                            node_c2.setFlagBit(LOCKC2);
                        if (constraintCheck(old, node_c2)) {
                            branch_nodes.push_back(node_c2);
                        }
                    }
                    if (slip_r > old.c3cp) {
                        slip_u = sliptoDefectUp(old.c3cp, old.c2cpu - item.h > old.c2cpb + GV::param.minWasteHeight ?
                            old.c2cpu - item.h : old.c2cpb, item.w);
                        if (slip_u != -1) {
                            if (!c2cpu_locked || (c2cpu_locked && slip_u + item.h <= old.c2cpu)) {
                                ItemNode node_c3(old, itemId, rotate | DEFECT_U);
                                node_c3.c3cp = old.c3cp + item.w;
                                if (node_c3.c1cpr < node_c3.c3cp)
                                    node_c3.c1cpr = node_c3.c3cp;
                                node_c3.c4cp = item.h + slip_u;
                                if (c2cpu_locked)
                                    node_c3.setFlagBit(LOCKC2);
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
                        ItemNode node_c4(old, itemId, rotate|NEW_L2);
                        node_c4.c3cp = slip_r + item.w;
                        if (node_c4.c1cpr < node_c4.c3cp)
                            node_c4.c1cpr = node_c4.c3cp;
                        node_c4.c2cpb = old.c2cpu;
                        node_c4.c4cp = node_c4.c2cpu = old.c2cpu + item.h;
                        if (slip_r > old.c1cpl)
                            node_c4.setFlagBit(DEFECT_R);
                        if (constraintCheck(old, node_c4)) {
                            branch_nodes.push_back(node_c4);
                            flag = true;
                        }
                    }
                    if (slip_r > old.c1cpl) {
                        slip_u = sliptoDefectUp(old.c1cpl, old.c2cpu, item.w);
                        if (slip_u != -1) {
                            ItemNode node_c5(old, itemId, rotate | NEW_L2 | DEFECT_U);
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
                        ItemNode node_c6(old, itemId, rotate|NEW_L1|NEW_L2);
                        node_c6.c1cpl = old.c1cpr;
                        node_c6.c3cp = node_c6.c1cpr = item.w + slip_r;
                        node_c6.c2cpb = 0;
                        node_c6.c4cp = node_c6.c2cpu = item.h;
                        if (slip_r > old.c1cpr)
                            node_c6.setFlagBit(DEFECT_R);
                        if (constraintCheck(old, node_c6)) {
                            branch_nodes.push_back(node_c6);
                        }
                    }
                    if (slip_r > old.c1cpr) {
                        slip_u = sliptoDefectUp(old.c1cpr, 0, item.w);
                        if (slip_u != -1) {
                            ItemNode node_c7(old, itemId, rotate | DEFECT_U | NEW_L1 | NEW_L2);
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
const bool CutSearch::constraintCheck(const ItemNode &old, ItemNode &node) {
    bool moved_cut1 = false, moved_cut2 = false;
    // if c2cpu less than c4cp, move it up.
    if (node.c2cpu < node.c4cp) {
        node.c2cpu = node.c4cp;
        moved_cut2 = true;
    }
    // check if item right side and c1cpr interval less than minWasteWidth.
    if (node.c3cp != node.c1cpr && node.c1cpr - node.c3cp < GV::param.minWasteWidth) {
        node.c1cpr += GV::param.minWasteWidth; // move c1cpr right to staisfy constraint.
        moved_cut1 = true;
    }
    // check if new c1cpr and old c1cpr interval less than minWasteWidth.
    if (node.c1cpr != old.c1cpr && node.c1cpr - old.c1cpr < GV::param.minWasteWidth) {
        node.c1cpr += GV::param.minWasteWidth;
        moved_cut1 = true;
    }
    // check if item up side and c2cpu interval less than minWasteHeight.
    if (node.c4cp != node.c2cpu && node.c2cpu - node.c4cp < GV::param.minWasteHeight) {
        if (node.getFlagBit(LOCKC2)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu += GV::param.minWasteHeight; // move c2cpu up to satisfy constraint.
            if (node.getFlagBit(DEFECT_U))
                node.c4cp = node.c2cpu;
            moved_cut2 = true;
        }
    }
    // check if new c2cpu and old c2cpu interval less than minWasteHeight
    if (node.getFlagBit(NEW_L2) && node.c2cpu != old.c2cpu
        && node.c2cpu - old.c2cpu < GV::param.minWasteHeight) {
        if (node.getFlagBit(LOCKC2)) {
            return false;
        } else {
            node.c2cpu += GV::param.minWasteHeight;
            if (node.getFlagBit(DEFECT_U))
                node.c4cp = node.c2cpu;
            moved_cut2 = true;
        }
    }
    
    node.c1cpr = cut1ThroughDefect(node.c1cpr);
    node.c2cpu = cut2ThroughDefect(node.c1cpl, node.c1cpr, node.c2cpu);

    // check if cut1 exceed stock right side.
    if (node.c1cpr > GV::param.plateWidth) {
        return false;
    }
    // check if cut2 exceed stock up side.
    if (node.c2cpu > GV::param.plateHeight) {
        return false;
    }
    // check if maximum cut1 width not staisfy.
    if (node.c1cpr - node.c1cpl > GV::param.maxL1Width) {
        return false;
    }
    // check if minimum cut1 width not staisfy.
    if (node.getFlagBit(NEW_L1) && old.c1cpr - old.c1cpl < GV::param.minL1Width) {
        return false;
    }
    // check if minimum cut2 height not staisfy.
    if (node.getFlagBit(NEW_L2) && old.c2cpu - old.c2cpb < GV::param.minL2Height) {
        return false;
    }
    // check if cut1 and stock right side interval less than minimum waste width.
    if (GV::param.plateWidth - node.c1cpr != 0 &&
        GV::param.plateWidth - node.c1cpr < GV::param.minWasteWidth) {
        return false;
    }
    // check if cut2 and stock up side interval less than minimum waste height.
    if (GV::param.plateHeight - node.c2cpu != 0 &&
        GV::param.plateHeight - node.c2cpu < GV::param.minWasteHeight) {
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
            if (slip - x < GV::param.minWasteWidth)
                slip = x + GV::param.minWasteWidth;
        }
        // check if dir-cut cut through defect.
        if (it->x < slip + w && it->x + it->w > slip + w) {
            slip = it->x + it->w - w;
            if (slip - x < GV::param.minWasteWidth)
                slip = x + GV::param.minWasteWidth;
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
    return res - x < GV::param.minWasteHeight ? x + GV::param.minWasteHeight : res;
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
            if (res - y < GV::param.minWasteHeight)
                res = y + GV::param.minWasteHeight;
        }
    }
    return res;
}

}
