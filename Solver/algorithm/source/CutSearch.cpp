#include "Solver/algorithm/header/CutSearch.h"

#include "Solver/data/header/PFSTree.h"
#include "Solver/data/header/Global.h"

using namespace std;

namespace szx {

CutSearch::CutSearch(TID plate, TCoord start_pos, size_t nb_sol_cache, const BRANCH_MODE mode) :
    plate_(plate), tail_area_((gv::param.plateWidth - start_pos)*gv::param.plateHeight),
    start_pos_(start_pos), mode_(mode), nb_sol_cache_(nb_sol_cache) {}

/*
* 优化1-cut，根据全局配置参数，自动选择优化策略。
* 输入：batch（物品栈），opt_tail（是否优化原料末尾）
* 输出：sol（该1-cut的最优解），返回：1-cut对应利用率
*/
void CutSearch::run(Batch &batch) {
    if(gv::cfg.cut_mode == Configuration::CBEAM)
        beam_search(batch);
    else if(gv::cfg.cut_mode == Configuration::CPFS)
        pfs(batch);
    else if(gv::cfg.cut_mode == Configuration::CDFS)
        dfs(batch);
}

UsageRate CutSearch::best_obj() const {
    return sol_cache_.empty() ? UsageRate() : sol_cache_.begin()->first;
}

void CutSearch::get_best_sol(Solution &sol) const {
    if (!sol_cache_.empty())
        sol = sol_cache_.begin()->second;
}

void CutSearch::get_good_sols(List<Solution> &sols) const {
    for (auto it = sol_cache_.begin(); it != sol_cache_.end(); ++it) {
        sols.push_back(it->second);
    }
}

void CutSearch::get_good_objs(List<UsageRate> &objs) const {
    for (auto it = sol_cache_.begin(); it != sol_cache_.end(); ++it) {
        objs.push_back(it->first);
    }
}

#pragma region BeamSearch
/* 束搜索 */
void CutSearch::beam_search(Batch &batch) {
    Area used_item_area = 0;    // 固定物品的面积和。
    Solution fix_sol;   // 已经被固定下来的解。
    Placement resume_point(start_pos_); // 每次分支的恢复点。
    List<Placement> branch_nodes;   // 缓存分支节点。
    // 初始分支
    branch(resume_point, batch, branch_nodes);
    using ScorePair = pair<double, size_t>;
    while (!branch_nodes.empty()) {
        // 限制beam_search宽度不超过set_.max_branch。
        List<ScorePair> score_pair;
        for (size_t i = 0; i < branch_nodes.size(); ++i) {
            double score = static_cast<double>(used_item_area + gv::item_area[branch_nodes[i].item]) / envelope_area(branch_nodes[i]);
            score_pair.push_back(make_pair(score, i));
        }
        sort(score_pair.begin(), score_pair.end(), [](const ScorePair &lhs, const ScorePair &rhs) {return lhs.first > rhs.first; });
        // 贪心的估计并更新
        UsageRate best_score;
        Placement best_place;
        for (size_t i = 0; i < min(gv::cfg.mcbn, branch_nodes.size()); ++i) {
            Placement &place = branch_nodes[score_pair[i].second];
            fix_sol.push_back(place);
            used_item_area += gv::item_area[place.item];
            batch.remove(place.item);
            UsageRate score = greedy(used_item_area, batch, fix_sol);
            if (score.valid() && best_score < score) {
                best_score = score;
                best_place = place;
            }
            fix_sol.pop_back();
            used_item_area -= gv::item_area[place.item];
            batch.add(place.item);
        }
        if (best_score.valid()) {
            fix_sol.push_back(best_place);
            used_item_area += gv::item_area[best_place.item];
            batch.remove(best_place.item);
            branch_nodes.clear();
            branch(best_place, batch, branch_nodes);
        } else {
            break;
        }
    }
}
#pragma endregion

#pragma region DFSAlgorithm
struct DFSTreeNode {
    Depth depth;
    Placement place;
    DFSTreeNode() : depth(-1) {}
    DFSTreeNode(Depth _depth, Placement &_place) : depth(_depth), place(_place) {}
};

/* 深度优先搜索 */
void CutSearch::dfs(Batch &batch) {
    Depth pre_depth = -1;    // 上一个节点深度
    UsageRate cur_obj;  // 当前解利用率
    Solution cur_sol;   // 当前解
    List<DFSTreeNode> live_nodes; // 待分支的树节点
    List<Placement> branch_nodes; // 储存指向分支出来的新Placement的指针
    // 第一次分支作特殊处理
    branch(Placement(start_pos_), batch, branch_nodes);
    for (auto it = branch_nodes.begin(); it != branch_nodes.end(); ++it) {
        live_nodes.push_back(DFSTreeNode(0, *it));
    }
    while (live_nodes.size()) { // 对搜索空间进行完整的搜索
        DFSTreeNode node = live_nodes.back();
        live_nodes.pop_back();
        if (node.depth - pre_depth == 1) { // 向下扩展
            cur_sol.push_back(node.place);
            batch.remove(node.place.item);
        } else if (node.depth - pre_depth < 1) { // 向上回溯
            for (int i = static_cast<int>(cur_sol.size()) - 1; i >= node.depth; --i) {
                batch.add(cur_sol[i].item);    // 恢复栈
            }
            cur_sol.erase(cur_sol.begin() + node.depth, cur_sol.end());
            cur_sol.push_back(node.place); // 更新当前解
            batch.remove(node.place.item);
        }
        UsageRate upper_bound(static_cast<double>(batch.total_item_area()) /
            (envelope_area(node.place) + batch.left_item_area()));
        if (upper_bound < best_obj()) {
            pre_depth = node.depth;
            continue;      // 剪枝。
        }
        // 分支并更新live_nodes。
        branch_nodes.clear();
        branch(node.place, batch, branch_nodes);
        for (auto it = branch_nodes.begin(); it != branch_nodes.end(); ++it) {
            live_nodes.push_back(DFSTreeNode(node.depth + 1, *it));
        }
        // 计算当前解的目标函数值。
        if (mode_ == PLATE) {
            cur_obj = UsageRate((double)batch.used_item_area() / (double)tail_area_);
        } else {
            cur_obj = UsageRate((double)batch.used_item_area() / (double)(
                (cur_sol.back().c1cpr - start_pos_)*gv::param.plateHeight));
        }
        update_sol_cache(cur_obj, cur_sol);     // 检查更新最优解。
        // 更新辅助变量。
        pre_depth = node.depth;
    }
}
#pragma endregion

#pragma region PFSAlgorithm
/* 优度优先搜索 */
void CutSearch::pfs(Batch &batch) {
    size_t cur_iter = 0;   // 当前扩展节点次数。
    PfsTree::Node *last_node = nullptr;
    UsageRate cur_obj;     // 当前解利用率。
    Solution cur_sol;      // 缓存当前解
    List<Placement> branch_nodes;   // 缓存每次分支扩展出来的节点。
    branch_nodes.reserve(100);
    PfsTree tree;
    branch(Placement(start_pos_), batch, branch_nodes);
    for (auto it = branch_nodes.begin(); it != branch_nodes.end(); ++it) {
        tree.add(*it, static_cast<double>(gv::item_area[it->item]) /
            static_cast<double>(envelope_area(*it)));
    }
    while (!tree.empty() && cur_iter < gv::cfg.mcit) {
        ++cur_iter;
        PfsTree::Node *node = tree.get();
        tree.update_batch(last_node, node, batch);
        UsageRate upper_bound(static_cast<double>(batch.total_item_area()) /
            (envelope_area(node->place) + batch.left_item_area()));
        if (upper_bound < best_obj()) {
            tree.add_leaf_node(node);
            last_node = node;
            continue;           // 剪枝。
        }
        // 分支并更新tree。
        branch_nodes.clear();
        branch(node->place, batch, branch_nodes);
        for (auto it = branch_nodes.begin(); it != branch_nodes.end(); ++it) {
            tree.add(node, *it, static_cast<double>(batch.used_item_area() + gv::item_area[it->item]) /
                static_cast<double>(envelope_area(*it)));
        }
        // 计算目标函数值。
        if (mode_ == PLATE) {
            cur_obj = UsageRate((double)batch.used_item_area() / (double)tail_area_);
        } else {
            cur_obj = UsageRate((double)batch.used_item_area() / (double)(
                (node->place.c1cpr - start_pos_)*gv::param.plateHeight));
        }
        // 更新目标函数值。
        tree.get_tree_path(node, cur_sol);
        update_sol_cache(cur_obj, cur_sol);
        // 更新辅助信息。
        if(branch_nodes.empty())    // 树种需要记录叶子节点，以便删除树。
            tree.add_leaf_node(node);
        last_node = node;
    }
}
#pragma endregion

/* 
* 贪心构造解
* 输入：used_item_area(固定部分物品占用面积), batch(可以物品栈), fix_sol(固定部分解)
* 输出：贪心走到底（到达1-cut尾部或原料尾部）时所得解的利用率。
* 注意：由于函数在运行时会改变引用的batch和fix_sol，该函数不便使用多线程。
*/
UsageRate CutSearch::greedy(Area used_item_area, Batch &batch, const Solution &fix_sol) {
    assert(!fix_sol.empty());
    UsageRate cur_obj;  // fix_sol + greedy_sol的目标函数值。
    Placement cur_node = fix_sol.back();    // 下一次分支的起点。
    // 检查当前解是否需要更新。
    if (mode_ == PLATE) {
        cur_obj = UsageRate((double)used_item_area / (double)tail_area_);
    } else {
        cur_obj = UsageRate((double)used_item_area / (double)(
            (cur_node.c1cpr - start_pos_)*gv::param.plateHeight));
    }
    update_sol_cache(cur_obj, fix_sol);
    Solution greedy_sol(fix_sol);    // 储存贪心构造的解。
    List<Placement> branch_nodes;
    branch(cur_node, batch, branch_nodes);
    while (!branch_nodes.empty()) {
        // 挑选并记录最好的解。
        UsageRate best_node_score;
        for (auto it = branch_nodes.begin(); it != branch_nodes.end(); ++it) {
            UsageRate this_node_score(static_cast<double>(used_item_area + gv::item_area[it->item]) /
                static_cast<double>(envelope_area(*it)));
            if (best_node_score < this_node_score) {
                best_node_score = this_node_score;
                cur_node = *it;
            }
        }
        greedy_sol.push_back(cur_node);
        used_item_area += gv::item_area[cur_node.item];
        // 计算目标函数值并检查更新最优解。
        if (mode_ == PLATE) {
            cur_obj = UsageRate((double)used_item_area / (double)tail_area_);
        } else {
            cur_obj = UsageRate((double)used_item_area / (double)(
                (cur_node.c1cpr - start_pos_)*gv::param.plateHeight));
        }
        update_sol_cache(cur_obj, greedy_sol);
        // 更新batch并再次分支。
        batch.remove(cur_node.item);
        branch_nodes.clear();
        branch(cur_node, batch, branch_nodes);
    }
    // 恢复batch中的物品。
    size_t count = 0, max_count = greedy_sol.size() - fix_sol.size();
    for (auto it = greedy_sol.rbegin(); it != greedy_sol.rend() && count < max_count; ++it, ++count)
        batch.add(it->item);
    return cur_obj;
}

/*
* 分支函数：
* 输入：old（分支起点），batch（物品栈），opt_tail（优化尾部）
* 输出：branch_nodes（用栈保存的搜索树）
*/
void CutSearch::branch(const Placement &old, const Batch &batch, List<Placement> &branch_nodes) {
    const bool c2cpu_locked = old.getFlagBit(Placement::LOCKC2); // status: 当前L2上界不能移动
    TID itemId = Problem::InvalidItemId;
    // case A, 开一个新的L1桶.
    if (old.c2cpu == 0) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (TID i = 0; i < batch.stack_num(); ++i) {
                itemId = batch.get(i);
                if (itemId == Problem::InvalidItemId)continue;
                // pretreatment.
                Rect item = gv::items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                if (old.c1cpr + item.w > gv::param.plateWidth)continue;
                // TODO: precheck if item exceed plate bound to speed up branch.

				// 瑕疵冲突, slip_r：瑕疵右边界到左侧cut的距离, slip_u：瑕疵上边界到下侧cut的距离
				TLength slip_r = 0, slip_u = 0;
                slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
				/* 不冲突直接放置，或者放在瑕疵右侧 */
                {
                    Placement node_a1(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2);
                    node_a1.c3cp = node_a1.c1cpr = slip_r + item.w; 
                    node_a1.c4cp = node_a1.c2cpu = item.h;
                    if (slip_r > old.c1cpr)
                        node_a1.setFlagBit(Placement::DEFECT_R); // 放在瑕疵右侧
                    if (constraintCheck(old, node_a1)) {
                        if (old.item == Problem::InvalidItemId && old.c1cpr == 0)
                            node_a1.setFlagBit(Placement::NEW_PLATE);
                        branch_nodes.push_back(node_a1);
                    }
                }
				/* 放在瑕疵上侧 */
                if (slip_r > old.c1cpr) { // 已知和瑕疵冲突
                    slip_u = sliptoDefectUp(old.c1cpr, 0, item.w);
                    if (slip_u != -1) {
                        Placement node_a2(old, itemId, rotate | Placement::NEW_L1 | Placement::NEW_L2); // place in defect up.
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
    // case B, 向右放置则拓宽c1cpr，向上放置则开一个新的L2桶.
    else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (TID i = 0; i < batch.stack_num(); ++i) {
                itemId = batch.get(i);
                if (itemId == Problem::InvalidItemId)continue;
                // pretreatment.
                Rect item = gv::items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place in the right of old.c1cpr.
                slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                {
                    Placement node_b1(old, itemId, rotate);
                    node_b1.c3cp = node_b1.c1cpr = slip_r + item.w;
                    node_b1.c4cp = item.h;
                    if (slip_r > old.c1cpr)
                        node_b1.setFlagBit(Placement::DEFECT_R);
                    if (constraintCheck(old, node_b1)) {
                        branch_nodes.push_back(node_b1);
                    }
                }
                if (slip_r > old.c1cpr) {
                    slip_u = sliptoDefectUp(old.c1cpr, old.c2cpu - item.h > gv::param.minWasteHeight ?
                        old.c2cpu - item.h : 0, item.w);    // [zjl][TODO]:consider more.
                    if (slip_u != -1) {
                        Placement node_b2(old, itemId, rotate);
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
                    Placement node_b3(old, itemId, rotate);
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
                        Placement node_b4(old, itemId, rotate);
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
                Rect item = gv::items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) swap(item.w, item.h);
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place item in the up of current c4cp.
                TLength last_item_w = old.getFlagBit(Placement::ROTATE) ? gv::items[old.item].h : gv::items[old.item].w;
                if (old.c2cpu > old.c4cp &&
                    last_item_w == item.w &&
                    !old.getFlagBit(Placement::DEFECT_U) &&
                    ((c2cpu_locked && old.c4cp + item.h == old.c2cpu) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpu)) &&
                    sliptoDefectRight(old.c3cp - item.w, old.c4cp, item.w, item.h) == old.c3cp - item.w) {
                    Placement node_c1(old, itemId, rotate| Placement::BIN4| Placement::LOCKC2); // place item in L4 bin.
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
                        Placement node_c2(old, itemId, rotate);
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
                        slip_u = sliptoDefectUp(old.c3cp, old.c2cpu - item.h > old.c2cpb + gv::param.minWasteHeight ?
                            old.c2cpu - item.h : old.c2cpb, item.w);
                        if (slip_u != -1) {
                            if (!c2cpu_locked || (c2cpu_locked && slip_u + item.h <= old.c2cpu)) {
                                Placement node_c3(old, itemId, rotate | Placement::DEFECT_U);
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
                        Placement node_c4(old, itemId, rotate| Placement::NEW_L2);
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
                            Placement node_c5(old, itemId, rotate | Placement::NEW_L2 | Placement::DEFECT_U);
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
                    if (mode_ == CUT || flag)continue; // if item can placed in a new L2, no need to place it in a new L1.
                    // creat a new L1 and place item in it.
                    slip_r = sliptoDefectRight(old.c1cpr, 0, item.w, item.h);
                    {
                        Placement node_c6(old, itemId, rotate| Placement::NEW_L1| Placement::NEW_L2);
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
                            Placement node_c7(old, itemId, rotate | Placement::DEFECT_U | Placement::NEW_L1 | Placement::NEW_L2);
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

/* 
* 检查新的分支节点是否满足约束。如果不满足，则修复。
* input: old branch node, current partial solution, new branch node.
*/
bool CutSearch::constraintCheck(const Placement &old, Placement &node) {
    // if c2cpu less than c4cp, move it up.
    if (node.c2cpu < node.c4cp) { node.c2cpu = node.c4cp; }

#pragma region minWasteChecker
    // check if item right side and c1cpr interval less than minWasteWidth.
    if (node.c3cp != node.c1cpr && node.c1cpr - node.c3cp < gv::param.minWasteWidth) {
        node.c1cpr += gv::param.minWasteWidth; // move c1cpr right to staisfy constraint.
    }
    // check if new c1cpr and old c1cpr interval less than minWasteWidth.
    if (node.c1cpr != old.c1cpr && node.c1cpr - old.c1cpr < gv::param.minWasteWidth) {
        node.c1cpr += gv::param.minWasteWidth;
    }
    // check if item up side and c2cpu interval less than minWasteHeight.
    if (node.c4cp != node.c2cpu && node.c2cpu - node.c4cp < gv::param.minWasteHeight) {
        if (node.getFlagBit(Placement::LOCKC2)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu += gv::param.minWasteHeight; // move c2cpu up to satisfy constraint.
            if (node.getFlagBit(Placement::DEFECT_U))
                node.c4cp = node.c2cpu;  // 物品在上，废料在下，防止在物品上方产生废料
        }
    }
    // check if new c2cpu and old c2cpu interval less than minWasteHeight
    if (node.c2cpu != old.c2cpu && node.c2cpu - old.c2cpu < gv::param.minWasteHeight) {
        if (node.getFlagBit(Placement::LOCKC2)) {
            return false;
        } else {
            node.c2cpu += gv::param.minWasteHeight;
            if (node.getFlagBit(Placement::DEFECT_U))
                node.c4cp = node.c2cpu;
        }
    }
#pragma endregion minWasteChecker
    
#pragma region defectChecker
    node.c1cpr = cut1ThroughDefect(node.c1cpr);
    node.c2cpu = cut2ThroughDefect(node.c2cpu);
#pragma endregion defectChecker

#pragma region sideChecker
    // check if cut1 exceed stock right side.
    if (node.c1cpr > gv::param.plateWidth) {
        return false;
    }
    // check if cut2 exceed stock up side.
    if (node.c2cpu > gv::param.plateHeight) {
        return false;
    }
	// check if cut1 and stock right side interval less than minimum waste width.
	if (gv::param.plateWidth != node.c1cpr &&
        gv::param.plateWidth - node.c1cpr < gv::param.minWasteWidth) {
		return false;
	}
	// check if cut2 and stock up side interval less than minimum waste height.
	if (gv::param.plateHeight != node.c2cpu &&
        gv::param.plateHeight - node.c2cpu < gv::param.minWasteHeight) {
		return false;
	}
#pragma endregion sideChecker

#pragma region cutWidthAndHeightChecker
    // check if maximum cut1 width not staisfy.
    if (node.c1cpr - node.c1cpl > gv::param.maxL1Width) {
        return false;
    }
    // check if minimum cut1 width not staisfy.
    if (node.getFlagBit(Placement::NEW_L1) && old.item != Problem::InvalidItemId &&
        old.c1cpr - old.c1cpl < gv::param.minL1Width) {
        return false;
    }
    // check if minimum cut2 height not staisfy.
    if (node.getFlagBit(Placement::NEW_L2) && old.item != Problem::InvalidItemId &&
        old.c2cpu - old.c2cpb < gv::param.minL2Height) {
        return false;
    }
#pragma endregion cutWidthAndHeightChecker

    return true; // every constraints satisfied.
}

// if item or 1-cut conflict with defect, move item/1-cut to the defect right side.
TCoord CutSearch::sliptoDefectRight(TCoord x, TCoord y, TLength w, TLength h) const {
    TCoord slip = x;
    for (auto it = gv::defect_x[plate_].begin(); it != gv::defect_x[plate_].end(); ++it) {
        if (it->x >= slip + w) {
            break; // defect in item right.
        }
        // 物品不在瑕疵的右、上、下侧，冲突
        if (!(slip - it->x >= it->w || y - it->y >= it->h || it->y - y >= h)) {
            slip = it->x + it->w;
            if (slip - x < gv::param.minWasteWidth)
                slip = x + gv::param.minWasteWidth;
        }
        // 检查c1cpr & c3cp(slip+w)是否冲突
        if (it->x < slip + w && it->x + it->w > slip + w) {
            slip = it->x + it->w - w;
            if (slip - x < gv::param.minWasteWidth)
                slip = x + gv::param.minWasteWidth;
        }
    }
    return slip;
}

// if area(x, y, w, plateHeight-y) has just one defect, slip item to defect up, or return -1.
TCoord CutSearch::sliptoDefectUp(TCoord x, TCoord y, TLength w) const {
    TCoord res = -1;
    for (auto it = gv::defect_x[plate_].begin(); it != gv::defect_x[plate_].end(); ++it) {
        if (it->x >= x + w) { // 瑕疵在右侧
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
    return res != -1 && res - y < gv::param.minWasteHeight ? y + gv::param.minWasteHeight : res;
}

// check if 1-cut through defect, return new 1-cut x coord. [not consider minwasteWidth constraint]
TCoord CutSearch::cut1ThroughDefect(TCoord x) const {
    TCoord res = x;
    for (auto it = gv::defect_x[plate_].begin(); it != gv::defect_x[plate_].end(); ++it) {
        if (it->x >= res) break;
        if (it->x + it->w > res) {   // 1-cut cut through defect.
            res = it->x + it->w;
        }
    }
    return res;
}

// check if 2-cut through defect, return new 2-cut y coord.
TCoord CutSearch::cut2ThroughDefect(TCoord y) const {
    TCoord res = y;
    for (auto it = gv::defect_y[plate_].begin(); it != gv::defect_y[plate_].end(); ++it) {
        if (it->y >= res)break;
        if (it->y + it->h > res) {    // 2-cut through defect.
            res = it->y + it->h;
            if (res - y < gv::param.minWasteHeight)
                res = y + gv::param.minWasteHeight;
        }
    }
    return res;
}

/* 获取当前放置物品和该1-cut形成的包络面积。 */
inline Area CutSearch::envelope_area(const Placement &place) const {
    return (place.c1cpl - start_pos_)*gv::param.plateHeight +
        place.c2cpb*(place.c1cpr - place.c1cpl) +
        (place.c3cp - place.c1cpl)*(place.c2cpu - place.c2cpb);
}

/*
* 更新sol_cache，自动删除多余的较差的解。
* 输入：obj(当前解的利用率)，sol（当前解的值）。
*/
void CutSearch::update_sol_cache(UsageRate obj, const Solution &sol) {
    lock_guard<mutex> guard(sol_mutex_);
    if (sol_cache_.size() < nb_sol_cache_) {
        sol_cache_.insert(make_pair(obj, sol));
    } else if (sol_cache_.rbegin()->first < obj) {
        sol_cache_.insert(make_pair(obj, sol));
        if (sol_cache_.size() > nb_sol_cache_)
            sol_cache_.erase(prev(sol_cache_.end()));
    }
}

}
