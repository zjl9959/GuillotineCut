/*note: use absolute TCoordinates in the algoritm.
*/
#include "TreeSearch.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>

#include <cmath>
#include <cassert>

#include "LogSwitch.h"

using namespace std;

namespace szx {

#pragma region Interface
void TreeSearch::solve() {
    init();
    List<TreeNode> sol;
    TreeNode start_node(-1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    depthFirstSearch(start_node, aux.stacks, sol);
    //randomRestartSearch(start_node, aux.stacks, sol);
    toOutput(sol);
}

void TreeSearch::record() const {
    #if SZX_DEBUG
    int generation = 0;

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Length obj = output.totalWidth * input.param.plateHeight - total_item_area;
    Length checkerObj = -1;
    bool feasible = check(checkerObj);

    // record basic information.
    log << env.friendlyLocalTime() << ","
        << env.rid << ","
        << env.instName << ","
        << feasible << "," << (obj - checkerObj) << ","
        << output.totalWidth << ","
        << timer.elapsedSeconds() << ","
        << mu.physicalMemory << "," << mu.virtualMemory << ","
        << env.randSeed << ","
        << cfg.toBriefStr() << ","
        << generation << "," << iteration << ","
        << total_item_area / (output.totalWidth * input.param.plateHeight / 100.0) << "%," // util ratio.
        << checkerObj; // wasted area.

    // record solution vector.
    // EXTEND[szx][2]: save solution in log.
    log << endl;

    // append all text atomically.
    static mutex logFileMutex;
    lock_guard<mutex> logFileGuard(logFileMutex);

    ofstream logFile(env.logPath, ios::app);
    logFile.seekp(0, ios::end);
    if (logFile.tellp() <= 0) {
        logFile << "Time,ID,Instance,Feasible,ObjMatch,Width,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration,Util,Waste,Solution" << endl;
    }
    logFile << log.str();
    logFile.close();
    #endif // SZX_DEBUG
}
#pragma endregion Interface

#pragma region Achievement
void TreeSearch::init() {
    constexpr TID InvalidItemId = Problem::InvalidItemId;

    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());
    aux.item2stack.resize(input.batch.size());

    // assign internal id to items and stacks, then push items into stacks.
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        TID itemId = idMap.item.toConsecutiveId(i->id);
        aux.items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
        if (itemId != i->id) {
            Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
        }

        TID stackId = idMap.stack.toConsecutiveId(i->stack);
        if (aux.stacks.size() <= stackId) { aux.stacks.push_back(List<TID>()); } // create a new stack.
        List<TID> &stack(aux.stacks[stackId]);
        // OPTIMIZE[szx][6]: what if the sequence number could be negative or very large?
        if (stack.size() <= i->seq) { stack.resize(i->seq + 1, InvalidItemId); }
        stack[i->seq] = itemId;
        aux.item2stack[itemId] = stackId;
    }
    // clear invalid items in stacks.
    for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
    }

    aux.defects.reserve(input.defects.size());
    // EXTEND[szx][9]: make sure that the plate IDs are already zero-base consecutive numbers.
    aux.plates_x.resize(Problem::MaxPlateNum);
    aux.plates_y.resize(Problem::MaxPlateNum);
    for (TID p = 0; p < Problem::MaxPlateNum; ++p) { idMap.plate.toConsecutiveId(p); }

    // map defects to plates.
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        TID defectId = idMap.defect.toConsecutiveId(d->id);
        aux.defects.push_back(RectArea(d->x, d->y, d->width, d->height));

        TID plateId = idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plates_x.size() <= plateId) { aux.plates_x.resize(plateId + 1); } // create new plates.
        aux.plates_x[plateId].push_back(defectId);
        if (aux.plates_y.size() <= plateId) { aux.plates_y.resize(plateId + 1); } // create new plates.
        aux.plates_y[plateId].push_back(defectId);
    }

    aux.item_area.reserve(aux.items.size());
    //calculate item areas.
    for (int i = 0; i < aux.items.size(); ++i) {
        aux.item_area.push_back(aux.items[i].h*aux.items[i].w);
        total_item_area += aux.items[i].h*aux.items[i].w;
    }
    // reverse item sequence convicent for pop_back.
    for (int i = 0; i < aux.stacks.size(); i++)
        reverse(aux.stacks[i].begin(), aux.stacks[i].end());
    // sort defects by it's x position.
    for (int p = 0; p < aux.plates_x.size(); ++p)
        for (int d = 0; d < aux.plates_x[p].size(); ++d)
            sort(aux.plates_x[p].begin(), aux.plates_x[p].end(), [&](TID &lhs, TID &rhs) {
            return aux.defects[lhs].x < aux.defects[rhs].x; });
    // sort defects by it's y position.
    for (int p = 0; p < aux.plates_y.size(); ++p)
        for (int d = 0; d < aux.plates_y[p].size(); ++d)
            sort(aux.plates_y[p].begin(), aux.plates_y[p].end(), [&](TID &lhs, TID &rhs) {
            return aux.defects[lhs].y < aux.defects[rhs].y; });
}

/* this method will restart search after maximum steps,
   the basic search method in every turn is depth first search. */
void TreeSearch::randomRestartSearch(const TreeNode &resume_point, List<List<TID>> &batch, List<TreeNode> &solution) {
    int left_items = 0;
    Area left_item_area = 0;
    for (auto stack : batch) {
        left_items += stack.size();
        for (auto item : stack) {
            left_item_area += aux.item_area[item];
        }
    }
    const int max_depth = left_items - 1;
    // configure varible start.
    const int max_restart_steps = 3000000; // max_restart_steps should greater than max_depth.
    const int max_restart_depth = max_depth * 0.5;
    // configure varible end.
    int best_obj = input.param.plateWidth*input.param.plateNum; // record the best objective up to now.
    List<TreeNode> live_nodes; // the tree nodes to be branched.
    List<TreeNode> cur_parsol, best_parsol; // current partial solution, best partial solution.
    size_t explored_nodes = 0;
    int cur_stamp_node = 0, last_stamp_node = 0; // stamp_node=explored_nodes when find a better solution.
    Depth pre_depth = -1; // last node depth.
    branch(resume_point, batch, cur_parsol, live_nodes);
    while (live_nodes.size() && !timer.isTimeOut()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop_back();
        explored_nodes++;
        cur_stamp_node++;
        if (node.depth < max_depth && // cut branch.
            getLowBound(node, left_item_area) > best_obj) {
            continue;
        }
        int last_livenode_end = live_nodes.size();
        if (node.depth - pre_depth == 1) { // search froward.
            cur_parsol.push_back(node);
            batch[aux.item2stack[node.item]].pop_back();
            left_items--;
            left_item_area -= aux.item_area[node.item];
            if (left_items > 0) {
                branch(node, batch, cur_parsol, live_nodes);
                random_shuffle(live_nodes.begin()+last_livenode_end, live_nodes.end());
            }
        } else if (node.depth - pre_depth < 1) { // search back.
            for (int i = cur_parsol.size() - 1; i >= node.depth; --i) { // resume stack.
                batch[aux.item2stack[cur_parsol[i].item]].push_back(
                    cur_parsol[i].item);
                left_items++;
                left_item_area += aux.item_area[cur_parsol[i].item];
            }
            cur_parsol.erase(cur_parsol.begin() + node.depth, cur_parsol.end()); // erase extend nodes.
            cur_parsol.push_back(node); // push current node into cur_parsol.
            batch[aux.item2stack[node.item]].pop_back();
            left_items--;
            left_item_area -= aux.item_area[node.item];
            if (left_items > 0) {
                branch(node, batch, cur_parsol, live_nodes);
                random_shuffle(live_nodes.begin() + last_livenode_end, live_nodes.end());
            }
        } else {
            Log(Log::Error) << "[ERROR]Depth first search: delt depth is "
                << node.depth - pre_depth << endl;
        }
        pre_depth = node.depth;
        if (left_items == 0) { // find a complate solution.
            int cur_obj = 0;
            for (TreeNode solnode : cur_parsol) {
                if (cur_obj < solnode.plate*input.param.plateWidth + solnode.c1cpr) {
                    cur_obj = solnode.plate*input.param.plateWidth + solnode.c1cpr;
                }
            }
            // TODO: evalute original objective or usage rate???
            if (best_obj > cur_obj) {
                best_obj = cur_obj;
                best_parsol = cur_parsol;
                last_stamp_node = cur_stamp_node;
                cout << "a better obj: " << cur_obj << endl;
            }
        }
        // choose a node from current bestsol and restart search.
        if (cur_stamp_node - last_stamp_node > max_restart_steps) {
            cur_stamp_node = last_stamp_node = 0;
            // random choose a resume node.
            const int restart_point = rand.pick(max_restart_depth);
            // resume batch. TODO[OPT]: copy from a total batch.
            for (int i = cur_parsol.size() - 1; i >= 0; --i) {
                batch[aux.item2stack[cur_parsol[i].item]].push_back(cur_parsol[i].item);
                left_items++;
                left_item_area += aux.item_area[cur_parsol[i].item];
            }
            // resume current solution.
            cur_parsol.clear();
            for (int i = 0; i < restart_point; ++i) {
                batch[aux.item2stack[best_parsol[i].item]].pop_back();
                left_items--;
                left_item_area -= aux.item_area[best_parsol[i].item];
                cur_parsol.push_back(best_parsol[i]);
            }
            // resume livenodes and pre_depth.
            live_nodes.clear();
            live_nodes.push_back(best_parsol[restart_point]);
            pre_depth = restart_point - 1;
        }
    }
    for (TreeNode sol : best_parsol) { // record this turn's best partial solution.
        solution.push_back(sol);
    }
    iteration = explored_nodes;
}

/* input:plate id, start 1-cut position, maximum used width, the batch to be used, solution vector.
   use depth first search to optimize partial solution. */
void TreeSearch::depthFirstSearch(const TreeNode &resume_point, List<List<TID>> &batch, List<TreeNode> &solution) {
    int left_items = 0; // left item number in the batch.
    Area left_item_area = 0;
    for (auto stack : batch) {
        left_items += stack.size();
        for (auto item : stack) {
            left_item_area += aux.item_area[item];
        }
    }
    const int max_depth = left_items - 1;
    int best_obj = input.param.plateWidth*input.param.plateNum; // record the best objective up to now.
    List<TreeNode> live_nodes; // the tree nodes to be branched.
    List<TreeNode> cur_parsol, best_parsol; // current partial solution, best partial solution.
    size_t explored_nodes = 0;
    Depth pre_depth = -1; // last node depth.
    branch(resume_point, batch, cur_parsol, live_nodes);
    while (live_nodes.size() && !timer.isTimeOut()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop_back();
        explored_nodes++;
        if (node.depth < max_depth && // cut branch.
            getLowBound(node,left_item_area) > best_obj) {
            continue;
        }
        if (node.depth - pre_depth == 1) { // search froward.
            cur_parsol.push_back(node);
            batch[aux.item2stack[node.item]].pop_back();
            left_items--;
            left_item_area -= aux.item_area[node.item];
            if (left_items > 0) {
                branch(node, batch, cur_parsol, live_nodes);
            }
        } else if (node.depth - pre_depth < 1) { // search back.
            for (int i = cur_parsol.size() - 1; i >= node.depth; --i) { // resume stack.
                batch[aux.item2stack[cur_parsol[i].item]].push_back(
                    cur_parsol[i].item);
                left_items++;
                left_item_area += aux.item_area[cur_parsol[i].item];
            }
            cur_parsol.erase(cur_parsol.begin() + node.depth, cur_parsol.end()); // erase extend nodes.
            cur_parsol.push_back(node); // push current node into cur_parsol.
            batch[aux.item2stack[node.item]].pop_back();
            left_items--;
            left_item_area -= aux.item_area[node.item];
            if (left_items > 0) {
                branch(node, batch, cur_parsol, live_nodes);
            }
        } else {
            Log(Log::Error) << "[ERROR]Depth first search: delt depth is " 
                << node.depth - pre_depth << endl;
        }
        pre_depth = node.depth;
        if (left_items == 0) { // find a complate solution.
            int cur_obj = 0;
            for (TreeNode solnode : cur_parsol) {
                if (cur_obj < solnode.plate*input.param.plateWidth + solnode.c1cpr) {
                    cur_obj = solnode.plate*input.param.plateWidth + solnode.c1cpr;
                }
            }
            // TODO: evalute original objective or usage rate???
            if (best_obj > cur_obj) {
                best_obj = cur_obj;
                best_parsol = cur_parsol;
                cout << "a better obj: " << cur_obj << endl;
            }
        }
    }
    for (TreeNode sol : best_parsol) { // record this turn's best partial solution.
        solution.push_back(sol);
    }
    iteration = explored_nodes;
}

/* input:last tree node(branch point).
   function:branch tree node by exact method.
*/// output:push branched nodes into live_nodes.
void TreeSearch::branch(const TreeNode &old, const List<List<TID>> &batch, const List<TreeNode> &cur_parsol, List<TreeNode> &live_nodes) {
    List<TreeNode> nodes; // store considered nodes, the node in it maybe not satisfy some constraints.
    List<bool> fesible; // if nodes[i] satisfy all the constraints, fesible[i] is true.
    const bool c2cpu_locked = old.getFlagBit(FlagBit::LOCKC2); // status: current c2cpu was locked.
    nodes.reserve(10);
    fesible.reserve(10);
    // case A, place item in a new L1.
    if (old.c2cpu == 0) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (auto stack : batch) {
                // pretreatment.
                if (!stack.size())continue; // skip empty stack.
                TID itemId = stack.back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                if (old.c1cpr + item.w > input.param.plateWidth)continue;
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // TODO: precheck if item exceed plate bound to speed up branch.
                // check if item conflict with defect and try to fix it.
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place in defect right(if no defect conflict, slip_r is 0).
                    auto node_a1 = nodes.rbegin();
                    node_a1->c3cp = node_a1->c1cpr = old.c1cpr + slip_r + item.w;
                    node_a1->c4cp = node_a1->c2cpu = item.h;
                    node_a1->cut2 = 0;
                    if (slip_r)node_a1->setFlagBit(FlagBit::DEFECT_R);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_a1));
                }
                if (slip_r) { // item conflict with defect(two choice).
                    slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place in defect up.
                    auto node_a2 = nodes.rbegin();
                    node_a2->c3cp = node_a2->c1cpr = old.c1cpr + item.w;
                    node_a2->c4cp = node_a2->c2cpu = slip_u + item.h;
                    node_a2->cut2 = 0;
                    node_a2->setFlagBit(FlagBit::DEFECT_U);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_a2));
                }
            }
        }
    }
    // case B, extend c1cpr or place item in a new L2.
    else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (auto stack : batch) {
                // pretreatment.
                if (!stack.size())continue; // skip empty stack.
                TID itemId = stack.back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place in the right of old.c1cpr.
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b1 = nodes.rbegin();
                    node_b1->c3cp = node_b1->c1cpr = old.c1cpr + slip_r + item.w;
                    node_b1->c4cp = item.h;
                    if (slip_r)node_b1->setFlagBit(FlagBit::DEFECT_R);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_b1));
                }
                if (slip_r) {
                    slip_u = sliptoDefectUp(RectArea(old.c1cpr, max(old.c2cpu - item.h , 0), item.w, item.h), old.plate);
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b2 = nodes.rbegin();
                    node_b2->c3cp = node_b2->c1cpr = old.c1cpr + item.w;
                    node_b2->c4cp = slip_u + old.c2cpu;
                    node_b2->setFlagBit(FlagBit::DEFECT_U);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_b2));
                }
                // place in the upper of old.c2cpu.
                slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                {
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b3 = nodes.rbegin();
                    node_b3->c2cpb = old.c2cpu;
                    node_b3->c4cp = node_b3->c2cpu = old.c2cpu + item.h;
                    node_b3->c3cp = old.c1cpl + slip_r + item.w;
                    if (node_b3->c1cpr < node_b3->c3cp)
                        node_b3->c1cpr = node_b3->c3cp;
                    ++node_b3->cut2;
                    if (slip_r)node_b3->setFlagBit(FlagBit::DEFECT_R);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_b3));
                }
                if (slip_r) {
                    slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b4 = nodes.rbegin();
                    node_b4->c2cpb = old.c2cpu;
                    node_b4->c4cp = node_b4->c2cpu = old.c2cpu + slip_u + item.h;
                    node_b4->c3cp = old.c1cpl + item.w;
                    if (node_b4->c1cpr < node_b4->c3cp)
                        node_b4->c1cpr = node_b4->c3cp;
                    ++node_b4->cut2;
                    node_b4->setFlagBit(FlagBit::DEFECT_U);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_b4));
                }
            }
        }
    }
    // case C, place item in the old L2 or a new L2 when item extend c1cpr too much.
    else {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (auto stack : batch) {
                // pretreatment.
                if (!stack.size())continue; // skip empty stack.
                TID itemId = stack.back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place item in the up of current c4cp.
                Length last_item_w = old.getFlagBit(FlagBit::ROTATE) ? aux.items[old.item].h : aux.items[old.item].w;
                if (old.c2cpu > old.c4cp && last_item_w == item.w && !old.getFlagBit(FlagBit::DEFECT_U) &&
                    ((c2cpu_locked && old.c4cp + item.h == old.c2cpb) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpb)) &&
                    !sliptoDefectRight(RectArea(old.c3cp - item.w, old.c4cp, item.w, item.h), old.plate)) {
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place item in L4 bin.
                    auto node_c1 = nodes.rbegin();
                    node_c1->c2cpu = node_c1->c4cp = old.c4cp + item.h;
                    node_c1->setFlagBit(FlagBit::BIN4);
                    node_c1->setFlagBit(FlagBit::LOCKC2);
                    fesible.push_back(constraintCheck(old, cur_parsol, *node_c1));
                    continue;
                }
                if (old.c1cpr > old.c3cp) { // place item in the right of current c3cp.
                    slip_r = sliptoDefectRight(RectArea(old.c3cp, old.c2cpb, item.w, item.h), old.plate);
                    // when c2cpu is locked, old.c2cpb + item.h <= old.c2cpu constraint must meet.
                    if (!c2cpu_locked || (c2cpu_locked && old.c2cpb + item.h <= old.c2cpu)) {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c2 = nodes.rbegin();
                        node_c2->c3cp = old.c3cp + slip_r + item.w;
                        if (node_c2->c1cpr < node_c2->c3cp)
                            node_c2->c1cpr = node_c2->c3cp;
                        node_c2->c4cp = old.c2cpb + item.h;
                        if (slip_r)
                            node_c2->setFlagBit(FlagBit::DEFECT_R);
                        if (c2cpu_locked)
                            node_c2->setFlagBit(FlagBit::LOCKC2);
                        fesible.push_back(constraintCheck(old, cur_parsol, *node_c2));
                    }
                    if(slip_r)
                        slip_u = sliptoDefectUp(RectArea(old.c3cp, max(old.c2cpu - item.h, 0), item.w, item.h), old.plate);
                    if (slip_u && (!c2cpu_locked || (c2cpu_locked && old.c2cpb + slip_u + item.h <= old.c2cpu))) {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c3 = nodes.rbegin();
                        node_c3->c3cp = old.c3cp + item.w;
                        if (node_c3->c1cpr < node_c3->c3cp)
                            node_c3->c1cpr = node_c3->c3cp;
                        node_c3->c4cp = old.c2cpu + slip_u;
                        node_c3->setFlagBit(FlagBit::DEFECT_U);
                        if (c2cpu_locked)
                            node_c3->setFlagBit(FlagBit::LOCKC2);
                        fesible.push_back(constraintCheck(old, cur_parsol, *node_c3));
                    }
                }
                if (old.c3cp + item.w > old.c1cpr) {
                    bool flag = false;
                    // creat a new L2 and place item in it.
                    slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c4 = nodes.rbegin();
                        node_c4->c3cp = old.c1cpl + slip_r + item.w;
                        if (node_c4->c1cpr < node_c4->c3cp)
                            node_c4->c1cpr = node_c4->c3cp;
                        node_c4->c2cpb = old.c2cpu;
                        node_c4->c4cp = node_c4->c2cpu = old.c2cpu + item.h;
                        ++node_c4->cut2; // update new L2 id.
                        if (slip_r)node_c4->setFlagBit(FlagBit::DEFECT_R);
                        fesible.push_back(constraintCheck(old, cur_parsol, *node_c4));
                        flag |= fesible.back();
                    }
                    if (slip_r) {
                        slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c5 = nodes.rbegin();
                        node_c5->c3cp = old.c1cpl + item.w;
                        if (node_c5->c1cpr < node_c5->c3cp)
                            node_c5->c1cpr = node_c5->c3cp;
                        node_c5->c2cpb = old.c2cpu;
                        node_c5->c4cp = node_c5->c2cpu = old.c2cpu + slip_u + item.h;
                        ++node_c5->cut2;
                        node_c5->setFlagBit(FlagBit::DEFECT_U);
                        fesible.push_back(constraintCheck(old, cur_parsol, *node_c5));
                        flag |= fesible.back();
                    }
                    if (flag)continue; // if item can placed in a new L2, no need to place it in a new L1.
                    // creat a new L1 and place item in it.
                    slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                    {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c6 = nodes.rbegin();
                        node_c6->c1cpl = old.c1cpr;
                        node_c6->c3cp = node_c6->c1cpr = old.c1cpr + item.w + slip_r;
                        node_c6->c2cpb = 0;
                        node_c6->c4cp = node_c6->c2cpu = item.h;
                        if (slip_r)node_c6->setFlagBit(FlagBit::DEFECT_R);
                        ++node_c6->cut1;
                        node_c6->cut2 = 0;
                        fesible.push_back(constraintCheck(old, cur_parsol, *node_c6));
                    }
                    if (slip_r) {
                        slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c7 = nodes.rbegin();
                        node_c7->c1cpl = old.c1cpr;
                        node_c7->c3cp = node_c7->c1cpr = old.c1cpr + item.w;
                        node_c7->c2cpb = 0;
                        node_c7->c4cp = node_c7->c2cpu = slip_u + item.h;
                        node_c7->setFlagBit(FlagBit::DEFECT_U);
                        ++node_c7->cut1;
                        node_c7->cut2 = 0;
                        fesible.push_back(constraintCheck(old, cur_parsol, *node_c7));
                    }
                }
            }
        }
    }
    // choose the fesible nodes and evaluate it.
    size_t size_record = live_nodes.size();
    for (int i = 0; i < nodes.size(); ++i) {
        if (fesible[i]) { // collect the fesible nodes and push.
            live_nodes.push_back(nodes[i]);
        }
    }
    if (size_record == live_nodes.size()) {  // no place to put item, creat new.
        TreeNode mask_node(old.depth, old.plate + 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        branch(mask_node, batch, cur_parsol, live_nodes);
    }
    // the mark score method may speed up search but will miss optimal solution.
    List<NodeInfo> node_pool;
    node_pool.reserve(10);
    for (int i = 0; i < nodes.size(); ++i) {
        if (fesible[i]) { // a fesible node
            node_pool.push_back(NodeInfo(
                i, getBranchScore(old, nodes[i]), aux.items[nodes[i].item].w));
        }
    }
    /*
    if (node_pool.size()) {
        // sort the node by it's score, if score is the same, bigger item first
        sort(node_pool.begin(), node_pool.end(), [](const NodeInfo &lhs, const NodeInfo &rhs) {
            return lhs.item_w > rhs.item_w; }); // place bigger item first
        stable_sort(node_pool.begin(), node_pool.end(), [](const NodeInfo &lhs, const NodeInfo &rhs) {
            return lhs.score < rhs.score; }); // sort by score
        // random choose some nodes to branch
        int left_nodes = node_pool.size()*cfg.cut_rate;
        if (left_nodes < 1) // make sure at least choose one branch node
            left_nodes = 1;
        if (left_nodes < cfg.sampling_num) { // no need to sampling, just copy nodes to live_nodes
            for (int i = left_nodes - 1; i >=0; --i) {
                live_nodes.push_back(nodes[node_pool[i].id]);
            }
        } else { // sampling cfg.sampling_num branch nodes and push into live_nodes
            for (int i = left_nodes - 1; i > left_nodes - cfg.sampling_num - 1; --i) {
                const int rd_num = rand.pick(i + 1); // random number range from 0 to i
                swap(node_pool.begin() + rd_num, node_pool.begin() + i);
                live_nodes.push_back(nodes[node_pool[i].id]);
            }
        }
    } else { // no place to put item, creat new.
        TreeNode mask_node(old.depth, old.plate + 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        branch(mask_node, batch, cur_parsol, live_nodes);
    }
    */
}

/* input: old branch node, new branch node.
   function: check if new branch satisfy all the constraints.
   if some constraints not staisfy, try to fix is by move item.
*/
const bool TreeSearch::constraintCheck(const TreeNode &old, const List<TreeNode> &cur_parsol, TreeNode &node) {
    bool moved_cut1 = false, moved_cut2 = false;
    // if c2cpu less than c4cp, move it up.
    if (node.c2cpu < node.c4cp) {
        node.c2cpu = node.c4cp;
        moved_cut2 = true;
    }
    // check if item right side and c1cpr interval less than minWasteWidth.
    if (node.c3cp != node.c1cpr && node.c1cpr - node.c3cp < input.param.minWasteWidth) {
        node.c1cpr += input.param.minWasteWidth; // move c1cpr right to staisfy constraint.
        moved_cut1 = true;
    }
    // check if new c1cpr and old c1cpr interval less than minWasteWidth.
    if (node.c1cpr != old.c1cpr && node.c1cpr - old.c1cpr < input.param.minWasteWidth) {
        node.c1cpr += input.param.minWasteWidth;
        moved_cut1 = true;
    }
    // check if item up side and c2cpu interval less than minWasteHeight.
    if (node.c4cp != node.c2cpu && node.c2cpu - node.c4cp < input.param.minWasteHeight) {
        if (node.getFlagBit(FlagBit::LOCKC2)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu += input.param.minWasteHeight; // move c2cpu up to satisfy constraint.
            moved_cut2 = true;
        }
    }
    // check if new c2cpu and old c2cpu interval less than minWasteHeight
    if (node.cut1 == old.cut1 && node.cut2 == old.cut2 && node.c2cpu != old.c2cpu 
        && node.c2cpu - old.c2cpu < input.param.minWasteHeight) {
        if (node.getFlagBit(FlagBit::LOCKC2)) {
            return false;
        } else {
            node.c2cpu += input.param.minWasteHeight;
            moved_cut2 = true;
        }
    }
    // check c3cp not cut through defect if item placed in defect up.
    if (node.getFlagBit(FlagBit::DEFECT_U)) {
        for (int i = 0; i < aux.plates_x[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_x[node.plate][i]];
            if (defect.x > node.c3cp) {
                break;
            }
            if (node.c3cp < defect.x + defect.w
                && node.c2cpb < defect.y + defect.h && node.c2cpu > defect.y) {
                return false;
            }
        }
    }
    // check not cut through defect constraint and try to fix it.
    if (moved_cut1 || old.c1cpr != node.c1cpr) {
        for (int i = 0; i < aux.plates_x[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_x[node.plate][i]];
            if (defect.x <= node.c1cpr) {
                if (defect.x + defect.w > node.c1cpr) { // c1cpr cut through defect.
                    if (defect.x + defect.w - node.c1cpr < input.param.minWasteWidth) {
                        if (defect.x + defect.w - node.c1cpr < input.param.minWasteWidth && !moved_cut1)
                            node.c1cpr += input.param.minWasteWidth;
                        else
                            node.c1cpr = defect.x + defect.w; // move c1cpr -> defect right side to satisfy the constraint.
                    } else {
                        return false;
                    }
                }
            } else { // the leftover defects is in the right of c1cpr.
                break;
            }
        }
    }
    if (moved_cut2 || old.c2cpu != node.c2cpu) {
        for (int i = 0; i < aux.plates_y[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_y[node.plate][i]];
            if (defect.y < node.c2cpu) {
                if (defect.y + defect.h > node.c2cpu
                    && node.c1cpl < defect.x + defect.w && node.c1cpr > defect.x) { // c2cpu cut through defect.
                    if (node.getFlagBit(FlagBit::LOCKC2) || // can't move c2cpu to satisfy the constraint.
                        defect.y + defect.h - node.c2cpu < input.param.minWasteHeight) {
                        return false;
                    } else {
                        if (defect.y + defect.h - node.c2cpu < input.param.minWasteHeight && !moved_cut2)
                            node.c2cpu += input.param.minWasteHeight;
                        else
                            node.c2cpu = defect.y + defect.h; // move c2cpu -> defect up side to satisfy the constraint.
                        moved_cut2 = true;
                    }
                }
            } else { // the leftover defects is in the upper of c2cpu.
                break;
            }
        }
    }
    // search back the current solution to check if item(placed in defect upper) conflict with defect after slip up.
    if (moved_cut2) {
        // check if current consider node conflict with defect after move.
        if (node.getFlagBit(FlagBit::DEFECT_U)) {
            const TLength item_w = node.getFlagBit(FlagBit::ROTATE) ? 
                aux.items[node.item].h : aux.items[node.item].w;
            const TLength item_h = node.getFlagBit(FlagBit::ROTATE) ? 
                aux.items[node.item].w : aux.items[node.item].h;
            if (defectConflictArea(RectArea(
                node.c3cp - item_w, node.c2cpu - item_h, item_w, item_h), node.plate))
                return false;
        }
        // check current partial solution
        for (int i = cur_parsol.size() - 1; i >= 0; --i) {
            if (cur_parsol[i].cut2 == node.cut2 && // the checked item should be in the same cut2.
                cur_parsol[i].cut1 == node.cut1 &&
                cur_parsol[i].plate == node.plate) {
                if (cur_parsol[i].getFlagBit(FlagBit::DEFECT_U)) {
                    const TLength item_w = cur_parsol[i].getFlagBit(FlagBit::ROTATE) ?
                        aux.items[cur_parsol[i].item].h : aux.items[cur_parsol[i].item].w;
                    const TLength item_h = cur_parsol[i].getFlagBit(FlagBit::ROTATE) ?
                        aux.items[cur_parsol[i].item].w : aux.items[cur_parsol[i].item].h;
                    if (defectConflictArea(RectArea(
                        cur_parsol[i].c3cp - item_w, cur_parsol[i].c2cpu - item_h, item_w, item_h), cur_parsol[i].plate))
                        return false;
                }
            } else {
                break;
            }
        }
    }
    // check if cut1 exceed stock right side.
    if (node.c1cpr > input.param.plateWidth) {
        return false;
    }
    // check if cut2 exceed stock up side.
    if (node.c2cpu > input.param.plateHeight) {
        return false;
    }
    // check if maximum cut1 width not staisfy.
    if (node.c1cpr - node.c1cpl > input.param.maxL1Width) {
        return false;
    }
    // check if minimum cut1 width not staisfy.
    if (node.cut1 > old.cut1 && old.cut1 > -1 && old.c1cpr - old.c1cpl < input.param.minL1Width) {
        return false;
    }
    // check if minimum cut2 height not staisfy.
    if (node.cut2 > old.cut2 && old.c2cpu - old.c2cpb < input.param.minL2Height) {
        return false;
    }
    // check if cut1 and stock right side interval less than minimum waste width.
    if (input.param.plateWidth - node.c1cpr != 0 &&
        input.param.plateWidth - node.c1cpr < input.param.minWasteWidth) {
        return false;
    }
    // check if cut2 and stock up side interval less than minimum waste height.
    if (input.param.plateHeight - node.c2cpu != 0 &&
        input.param.plateHeight - node.c2cpu < input.param.minWasteHeight) {
        return false;
    }
    return true; // every constraints satisfied.
}

/* input:rectangle area to place item, plate id.
   function: if rectangle area conflict with one or more defects,
   slip the area right to the rightmost defects right side.
*/// output: slip TLength to avoid conflict with defect.
const TLength TreeSearch::sliptoDefectRight(const RectArea &area, const TID plate) const {
    TLength slip = area.x;
    for (auto d : aux.plates_x[plate]) {
        if (aux.defects[d].x > slip + area.w) {
            break; // the defects is sorted by x position, so in this case just break.
        }
        if (!(slip - aux.defects[d].x > aux.defects[d].w // defect in item left.
            || aux.defects[d].y - area.y > area.h // defect in item up.
            || area.y - aux.defects[d].y > aux.defects[d].h)) { // defect in item bottom.
            slip = aux.defects[d].x + aux.defects[d].w; // defect[d] conflict with area, slip to defect right side.
        }
    }
    if (slip - area.x != 0 && slip - area.x < input.param.minWasteWidth)
        return input.param.minWasteWidth;
    return slip - area.x;
}

/* input:rectangle area to place item, plate id.
   function: if rectangle area conflict with one or more defects.
   slip the area up to the upmost defects up side.
*/// output: slip TLength to avoid conflict with defect.
const TLength TreeSearch::sliptoDefectUp(const RectArea &area, const TID plate) const {
    TLength slip = area.y;
    for (auto d : aux.plates_y[plate]) {
        if (aux.defects[d].y > slip + area.h) {
            break; // the defects is sorted by y position, so in this case just break.
        }
        if (!(aux.defects[d].x - area.x > area.w // defect in item right.
            || area.x - aux.defects[d].x > aux.defects[d].w // defect in item left.
            || slip - aux.defects[d].y > aux.defects[d].h)) { // defect in item bottom.
            slip = aux.defects[d].y + aux.defects[d].h; // defect[d] conflict with area, slip to defect up side.
        }
    }
    if (slip - area.y != 0 && slip - area.y < input.param.minWasteHeight)
        return input.param.minWasteHeight;
    return slip - area.y;
}

// if one or more defct conflict with area, return true.
const bool TreeSearch::defectConflictArea(const RectArea & area, const TID plate) const {
    for (auto d : aux.plates_y[plate]) {
        if (aux.defects[d].y > area.y + area.h) {
            break;
        }
        if (!(aux.defects[d].x - area.x > area.w // defect in item right.
            || area.x - aux.defects[d].x > aux.defects[d].w  // defect in item left.
            || area.y - aux.defects[d].y > aux.defects[d].h)) {  // defect in item bottom.
            return true;
        }
    }
    return false;
}

/* input: branch node to evaluate.
   function: calculate the local waste area caused by the branch node.
   the evaluate is not so precise, just as reference.
*/// output: waste area.
const double TreeSearch::getBranchScore(const TreeNode &old, const TreeNode & node) const {
    Area waste = 0;
    const TCoord item_left = node.c3cp - aux.items[node.item].w;
    const TCoord item_bottom = node.c4cp - aux.items[node.item].h;
    if (item_left > old.c3cp) { // place item in defect right caused waste.
        waste += (item_left - old.c3cp)*(node.c2cpu - node.c2cpb);
    }
    if (item_bottom > node.c2cpb && node.getFlagBit(FlagBit::DEFECT_U)) { // place item in defect upper caused waste.
        waste += (node.c3cp - old.c3cp)*(item_bottom - node.c2cpb);
    }
    if (node.c2cpu > node.c4cp) { // c2cpu and c4cp interval caused waste.
        waste += (node.c2cpu - node.c4cp)*aux.items[node.item].w;
    }
    if (node.c2cpu > old.c2cpu) { // move c2cpu caused waste.
        waste += (node.c2cpu - old.c2cpu)*(old.c3cp - old.c1cpl);
    }
    if (node.c1cpr > old.c1cpr) { // move c1cpr caused waste.
        waste += (node.c1cpr - old.c1cpr)*node.c2cpb;
    }
    if (node.cut2 > old.cut2) { // last L2 and last item waste when creat new L2.
        waste += (old.c1cpr - old.c3cp)*(old.c2cpu - old.c2cpb);
    }
    return waste/aux.item_area[node.item];
}

/* input: current branch node, the left item area to calculate low bound.
   calculate a little relaxed low bound */
const Length TreeSearch::getLowBound(const TreeNode & cur_node, Area left_item_area) const {
    left_item_area = left_item_area - (cur_node.c2cpu - cur_node.c2cpb)*(cur_node.c1cpr - cur_node.c3cp) -
        (input.param.plateHeight - cur_node.c2cpu)*(cur_node.c1cpr - cur_node.c1cpl) - aux.item_area[cur_node.item];
    return input.param.plateWidth*cur_node.plate + cur_node.c1cpr + max(
        0, static_cast<Length>(left_item_area / input.param.plateHeight));
}

// this function convert TreeNode type solution into output type .
void TreeSearch::toOutput(List<TreeNode> &sol) {
    using SpecialType = Problem::Output::Node::SpecialType;
    const TLength PW = input.param.plateWidth, PH = input.param.plateHeight;
    // define current plate/cut1/cut2 identity in solution node.
    TID cur_plate = -1, cur_cut1 = 0, cur_cut2 = 0;
    // define current node/parentL0.. identity in output solution tree node.
    TID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    // define current c1cpl/c1cpr... when constructing solution tree.
    TCoord c1cpl = 0, c1cpr = 0, c2cpb=0, c2cpu = 0, c3cp = 0;
    // construct solution tree by visit each sol TreeNode.
    for (int n = 0; n < sol.size(); ++n) {
        if (cur_plate < sol[n].plate) { // a new plate.
            // add waste for last plate
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (cur_plate != -1 && c2cpu < input.param.plateHeight) { // add waste between c2cpu and plate up bound.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            if (cur_plate != -1 && c1cpr < input.param.plateWidth) { // add waste between c1cpr and plate right bound.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Waste, 1, parent_L0));
            }
            // update cut position and cut identity information.
            cur_plate = sol[n].plate;
            cur_cut1 = 0;
            cur_cut2 = 0;
            c1cpl = 0;
            c1cpr = getC1cpr(sol, n, cur_plate, cur_cut1);
            c2cpb = 0;
            c2cpu = getC2cpu(sol, n, cur_cut1, cur_cut2);
            c3cp = 0;
            // creat new nodes.
            parent_L0 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new plate.
                cur_plate, nodeId++, 0, 0, PW, PH, SpecialType::Branch, 0, -1));
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1.
                cur_plate, nodeId++, 0, 0, c1cpr, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, 0, 0, c1cpr, c2cpu, SpecialType::Branch, 2, parent_L1));
        }
        else if (cur_cut1 < sol[n].cut1) { // a new cut1.
            // add waste for last L1.
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (c2cpu < input.param.plateHeight) { // add waste between c2cpu and plate up bound.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            // update cut position and cut identity information.
            cur_cut1 = sol[n].cut1;
            cur_cut2 = 0;
            c1cpl = c1cpr;
            c1cpr = getC1cpr(sol, n, cur_plate, cur_cut1);
            c2cpb = 0;
            c2cpu = getC2cpu(sol, n, cur_cut1, cur_cut2);
            c3cp = c1cpl;
            // creat new nodes.
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1.
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        }
        else if (cur_cut2 < sol[n].cut2) { // a new cut2.
            // add waste for last L2.
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            // update cut position and cut identity information.
            cur_cut2 = sol[n].cut2;
            c2cpb = c2cpu;
            c2cpu = getC2cpu(sol, n, cur_cut1, cur_cut2);
            c3cp = c1cpl;
            // creat new nodes.
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        const TLength item_w = sol[n].getFlagBit(FlagBit::ROTATE) ? aux.items[sol[n].item].h : aux.items[sol[n].item].w;
        const TLength item_h = sol[n].getFlagBit(FlagBit::ROTATE) ? aux.items[sol[n].item].w : aux.items[sol[n].item].h;
        // if item placed in defect right bound, creat waste between c3cp and item left bound.
        if (sol[n].getFlagBit(FlagBit::DEFECT_R)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // if has bin4, place this node into last L3, else creat a new L3.
        if (sol[n].getFlagBit(FlagBit::BIN4)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
            continue;
        } else {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        // if item placed in defect up bound, creat waste between item bottom and c2cpb.
        if (sol[n].getFlagBit(FlagBit::DEFECT_U)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
        } else {
            // creat a item in L3.
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, item_h, sol[n].item, 4, parent_L3));
            // add waste between c4cp and c2cpu.
            if (c2cpu > sol[n].c4cp && n + 1 < sol.size() && !sol[n + 1].getFlagBit(FlagBit::BIN4)) {
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, sol[n].c3cp - item_w, sol[n].c4cp, item_w, c2cpu - sol[n].c4cp, SpecialType::Waste, 4, parent_L3));
            }
        }
        c3cp = sol[n].c3cp;
    }
    const TreeNode last_item = sol.back();
    const TLength last_item_w = last_item.getFlagBit(FlagBit::ROTATE) ? 
        aux.items[last_item.item].h : aux.items[last_item.item].w;
    if (last_item.c4cp < c2cpu && !last_item.getFlagBit(FlagBit::DEFECT_U)) { // add last waste between c4cp and c2cpu.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, last_item.c3cp - last_item_w, 
            last_item.c4cp, last_item_w, c2cpu - last_item.c4cp, SpecialType::Waste, 4, parent_L3));
    }
    if (c3cp < c1cpr) { // add last waste between c3cp and c1cpr.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
    }
    if (c2cpu < input.param.plateHeight) { // add last waste between c2cpu and plate up bound.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
    }
    if (c1cpr < input.param.plateWidth) { // add last residual between c1cpr and plate right bound.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Residual, 1, parent_L0));
    }
    // calculate total width of output.
    output.totalWidth = 0;
    for (int i = 0; i < sol.size(); ++i) {
        if (output.totalWidth < sol[i].plate*input.param.plateWidth + sol[i].c1cpr) {
            output.totalWidth = sol[i].plate*input.param.plateWidth + sol[i].c1cpr;
        }
    }
}

/* input: solution TreeNode type, solution start index, current cut1 id.
*/// ouput: current cut1's c1cpr.
const TCoord TreeSearch::getC1cpr(const List<TreeNode> &sol, const int index, const TID cur_plate, const TID cur_cut1) const {
    TCoord res = 0;
    for (int i = index; i < sol.size(); ++i) {
        if (cur_plate == sol[i].plate && cur_cut1 == sol[i].cut1) {
            if (res < sol[i].c1cpr)
                res = sol[i].c1cpr;
        } else {
            break;
        }
    }
    return res;
}

/* input: solution TreeNode type, solution start index, current cut2 id.
*/// ouput: current cut2's c2cpu.
const TCoord TreeSearch::getC2cpu(const List<TreeNode> &sol, const int index, const TID cur_cut1, const TID cur_cut2) const {
    TCoord res = 0;
    for (int i = index; i < sol.size(); ++i) {
        if (cur_cut1 == sol[i].cut1 && cur_cut2 == sol[i].cut2) {
            if (res < sol[i].c2cpu)
                res = sol[i].c2cpu;
        } else {
            break;
        }
    }
    return res;
}

bool TreeSearch::check(Length & checkerObj) const {
    #if SZX_DEBUG
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        ItemProductionError = 0x2,
        DefectSuperposingError = 0x4,
        ItemDimensionError = 0x8,
        IdentityError = 0x10,
        SequenceError = 0x20
    };

    checkerObj = System::exec("Checker.exe " + env.instName + " " + env.solutionPath());
    if (checkerObj > 0) { return true; }
    checkerObj = ~checkerObj;
    if (checkerObj == CheckerFlag::IoError) { Log(LogSwitch::Checker) << "IoError." << endl; }
    if (checkerObj & CheckerFlag::FormatError) { Log(LogSwitch::Checker) << "FormatError." << endl; }
    if (checkerObj & CheckerFlag::ItemProductionError) { Log(LogSwitch::Checker) << "ItemProductionError." << endl; }
    if (checkerObj & CheckerFlag::DefectSuperposingError) { Log(LogSwitch::Checker) << "DefectSuperposingError." << endl; }
    if (checkerObj & CheckerFlag::ItemDimensionError) { Log(LogSwitch::Checker) << "ItemDimensionError." << endl; }
    if (checkerObj & CheckerFlag::IdentityError) { Log(LogSwitch::Checker) << "IdentityError." << endl; }
    if (checkerObj & CheckerFlag::SequenceError) { Log(LogSwitch::Checker) << "SequenceError." << endl; }
    return false;
    #else
    checkerObj = 0;
    return true;
    #endif // SZX_DEBUG
}

#pragma endregion Achievement

}