/*note: use absolute coordinates in the algoritm.
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
    // TODO: add code...
}
#pragma endregion Interface

#pragma region Achievement
void TreeSearch::init() {
    constexpr ID InvalidItemId = Problem::InvalidItemId;

    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());
    aux.defect2stack.resize(input.batch.size());

    // assign internal id to items and stacks, then push items into stacks.
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        ID itemId = idMap.item.toConsecutiveId(i->id);
        aux.items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
        if (itemId != i->id) {
            Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
        }

        ID stackId = idMap.stack.toConsecutiveId(i->stack);
        if (aux.stacks.size() <= stackId) { aux.stacks.push_back(List<ID>()); } // create a new stack.
        List<ID> &stack(aux.stacks[stackId]);
        // OPTIMIZE[szx][6]: what if the sequence number could be negative or very large?
        if (stack.size() <= i->seq) { stack.resize(i->seq + 1, InvalidItemId); }
        stack[i->seq] = itemId;
        aux.defect2stack[itemId] = stackId;
    }
    // clear invalid items in stacks.
    for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
    }

    aux.defects.reserve(input.defects.size());
    // EXTEND[szx][9]: make sure that the plate IDs are already zero-base consecutive numbers.
    aux.plates_x.resize(Problem::MaxPlateNum);
    aux.plates_y.resize(Problem::MaxPlateNum);
    for (ID p = 0; p < Problem::MaxPlateNum; ++p) { idMap.plate.toConsecutiveId(p); }

    // map defects to plates.
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        ID defectId = idMap.defect.toConsecutiveId(d->id);
        aux.defects.push_back(RectArea(d->x, d->y, d->width, d->height));

        ID plateId = idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plates_x.size() <= plateId) { aux.plates_x.resize(plateId + 1); } // create new plates.
        aux.plates_x[plateId].push_back(defectId);
        if (aux.plates_y.size() <= plateId) { aux.plates_y.resize(plateId + 1); } // create new plates.
        aux.plates_y[plateId].push_back(defectId);
    }

    aux.item_area.reserve(aux.items.size());
    //calculate item areas.
    for (int i = 0; i < aux.items.size(); ++i)
        aux.item_area.push_back(aux.items[i].h*aux.items[i].w);
    // reverse item sequence convicent for pop_back.
    for (int i = 0; i < aux.stacks.size(); i++)
        reverse(aux.stacks[i].begin(), aux.stacks[i].end());
    // sort defects by it's x position.
    for (int p = 0; p < aux.plates_x.size(); ++p)
        for (int d = 0; d < aux.plates_x[p].size(); ++d)
            sort(aux.plates_x[p].begin(), aux.plates_x[p].end(), [&](ID &lhs, ID &rhs) {
            return aux.defects[lhs].x < aux.defects[rhs].x; });
    // sort defects by it's y position.
    for (int p = 0; p < aux.plates_y.size(); ++p)
        for (int d = 0; d < aux.plates_y[p].size(); ++d)
            sort(aux.plates_y[p].begin(), aux.plates_y[p].end(), [&](ID &lhs, ID &rhs) {
            return aux.defects[lhs].y < aux.defects[rhs].y; });
    // TODO[DEBUG]: verify if defects.x smallest is in the first??? 
}

/* input:plate id, start 1-cut position, maximum used width, the batch to be used, solution vector.
   use depth first search to optimize partial solution.
*/
void TreeSearch::depthFirstSearch(const int ub, List<List<ID>> &batch, List<TreeNode> &solution) {
    int left_items = 0; // left item number in the batch
    for (auto stack : batch)
        left_items += stack.size();
    int best_obj = input.param.plateWidth*input.param.plateNum; // record the best objective up to now
    List<TreeNode> live_nodes; // the tree nodes to be branched
    TreeNode temp = solution.back();
    temp.depth = -1; // start search from depth 0
    branch(temp, batch, live_nodes);
    List<TreeNode> cur_parsol, best_parsol; // current partial solution, best partial solution
    int explored_nodes = 0, cutted_nodes = 0;
    Depth pre_depth = -1; // last node depth
    while (live_nodes.size() && !timer.isTimeOut()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop_back();
        explored_nodes++;
        //TODO: add get low bound...
        /*if () {
            cutted_nodes++;
            continue;
        }*/
        if (node.depth - pre_depth == 1) { // search froward
            cur_parsol.push_back(node);
            batch[aux.defect2stack[node.item]].pop_back();
            left_items--;
            if (left_items > 0) {
                branch(node, batch, live_nodes);
            }
        } else if (node.depth - pre_depth < 1) { // search back
            for (int i = cur_parsol.size() - 1; i >= node.depth; --i) { // resume stack
                batch[aux.defect2stack[cur_parsol[i].item]].push_back(
                    cur_parsol[i].item);
                left_items++;
            }
            cur_parsol.erase(cur_parsol.begin() + node.depth, cur_parsol.end()); // erase extend nodes
            cur_parsol.push_back(node); // push current node into cur_parsol
            batch[aux.defect2stack[node.item]].pop_back();
            left_items--;
            if (left_items > 0) {
                branch(node, batch, live_nodes);
            }
        } else {
            Log(Log::Error) << "[ERROR]Depth first search: delt depth is " 
                << node.depth - pre_depth << endl;
        }
        pre_depth = node.depth;
        if (left_items == 0) { // find a complate solution
            int cur_obj = 0;
            for (TreeNode solnode : cur_parsol) {
                if (cur_obj < solnode.c1cpr)
                    cur_obj = solnode.c1cpr;
            }
            // TODO: evalute original objective or usage rate???
            if (best_obj > cur_obj) {
                best_obj = cur_obj;
                best_parsol = cur_parsol;
            }
        }
    }
    for (TreeNode sol : best_parsol) { // record this turn's best partial solution
        solution.push_back(sol);
    }
}

/* input:last tree node(branch point)
   function:branch tree node by exceed method
*/// output:push branched nodes into live_nodes
void TreeSearch::branch(const TreeNode &old, const List<List<ID>> &batch, List<TreeNode> &live_nodes) {
    List<TreeNode> nodes;
    List<bool> fesible;
    nodes.reserve(10);
    fesible.reserve(10);
    for (int rotate = 0; rotate <= 1; ++rotate) {
        for (auto stack : batch) {
            // pretreatment.
            if (!stack.size())continue; // skip empty stack.
            ID itemId = stack.back();
            Rect item = aux.items[itemId];
            if (item.w == item.h && rotate)continue; // skip rectangle item.
            if (rotate) { // rotate item.
                item.w = aux.items[itemId].h;
                item.h = aux.items[itemId].w;
            }
            Length slip_r = 0, slip_u = 0;
            // TODO: what if item is too larger?
            if (old.c2cpu == 0) { // case A
                // check if item conflict with defect and try to fix it
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place in defect right(if no defect conflict, slip_r is 0)
                    auto node_a1 = nodes.rbegin();
                    node_a1->c3cp = node_a1->c1cpr = old.c1cpr + slip_r + item.w;
                    node_a1->c4cp = node_a1->c2cpu = item.h;
                    node_a1->cut1 = old.cut1 + 1;
                    node_a1->cut2 = 0;
                    if (slip_r)node_a1->setFlagBit(FlagBit::DEFECT);
                    fesible.push_back(constraintCheck(old, *node_a1));
                }
                if (slip_u) { // item conflict with defect(two choice)
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place in defect up
                    auto node_a2 = nodes.rbegin();
                    node_a2->c3cp = node_a2->c1cpr = old.c1cpr + item.w;
                    node_a2->c4cp = node_a2->c2cpu = slip_u + item.h;
                    node_a2->cut1 = old.cut1 + 1;
                    node_a2->cut2 = 0;
                    node_a2->setFlagBit(FlagBit::DEFECT);
                    fesible.push_back(constraintCheck(old, *node_a2));
                }
            } 
            else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) { // case B
                // place in the right of old.c1cpr
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b1 = nodes.rbegin();
                    node_b1->c3cp = node_b1->c1cpr = old.c1cpr + slip_r + item.w;
                    node_b1->c4cp = item.h;
                    if (node_b1->c2cpu < node_b1->c4cp)
                        node_b1->c2cpu = item.h;
                    if (slip_r)node_b1->setFlagBit(FlagBit::DEFECT);
                    fesible.push_back(constraintCheck(old, *node_b1));
                }
                if (slip_u) {
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b2 = nodes.rbegin();
                    node_b2->c3cp = node_b2->c1cpr = old.c1cpr + item.w;
                    node_b2->c4cp = slip_u + item.h;
                    if (node_b2->c2cpu < node_b2->c4cp)
                        node_b2->c2cpu = node_b2->c4cp;
                    node_b2->setFlagBit(FlagBit::DEFECT);
                    fesible.push_back(constraintCheck(old, *node_b2));
                }
                // place in the upper of old.c2cpu
                slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                {
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b3 = nodes.rbegin();
                    node_b3->c2cpb = old.c2cpu;
                    node_b3->c4cp = node_b3->c2cpu = old.c2cpu + item.h;
                    node_b3->c3cp = old.c1cpl + slip_r + item.w;
                    if (node_b3->c1cpr < node_b3->c3cp)
                        node_b3->c1cpr = node_b3->c3cp;
                    node_b3->cut2 = old.cut2 + 1;
                    if (slip_r)node_b3->setFlagBit(FlagBit::DEFECT);
                    fesible.push_back(constraintCheck(old, *node_b3));
                }
                if (slip_u) {
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_b4 = nodes.rbegin();
                    node_b4->c2cpb = old.c2cpu;
                    node_b4->c4cp = node_b4->c2cpu = old.c2cpu + slip_r + item.h;
                    node_b4->c3cp = old.c1cpl + item.w;
                    if (node_b4->c1cpr < node_b4->c3cp)
                        node_b4->c1cpr = node_b4->c3cp;
                    node_b4->cut2 = old.cut2 + 1;
                    node_b4->setFlagBit(FlagBit::DEFECT);
                    fesible.push_back(constraintCheck(old, *node_b4));
                }
            } 
            else if(old.c3cp < old.c1cpr){ // case C
                if (old.c4cp > old.c2cpb && !old.getFlagBit(FlagBit::DEFECT) && // last item in L3 and no defects in this L3
                    old.c3cp - aux.items[old.item].w == item.w && old.c4cp+item.h >= old.c2cpu) { // the item just fit the L4
                    if (sliptoDefectRight(RectArea(old.c3cp - aux.items[old.item].w, old.c4cp, item.w, item.h), old.plate) == 0) {
                        nodes.push_back(TreeNode(old, itemId, rotate)); // place item in L4 bin
                        auto node_c1 = nodes.rbegin();
                        node_c1->c4cp = old.c4cp + item.h;
                        if (old.getFlagBit(FlagBit::BIN4)) { // already have one bin4
                            if (node_c1->c2cpu == node_c1->c4cp) { // c2cpu should equal to c4cp in this case 
                                node_c1->setFlagBit(FlagBit::BIN4);
                                fesible.push_back(constraintCheck(old, *node_c1));
                            }
                        } else { // this is the first bin4 in current cut2
                            if (node_c1->c2cpu < node_c1->c4cp)
                                node_c1->c2cpu = node_c1->c4cp;
                            node_c1->setFlagBit(FlagBit::BIN4);
                            fesible.push_back(constraintCheck(old, *node_c1));
                        }
                    }
                }
                { // place item in the right of current c3cp
                    slip_r = sliptoDefectRight(RectArea(old.c3cp, old.c2cpb, item.w, item.h), old.plate);
                    slip_u = sliptoDefectUp(RectArea(old.c3cp, old.c2cpb, item.w, item.h), old.plate);
                    {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c2 = nodes.rbegin();
                        node_c2->c3cp = old.c3cp + slip_r + item.w;
                        if (node_c2->c1cpr < node_c2->c3cp)
                            node_c2->c1cpr = node_c2->c3cp;
                        node_c2->c4cp = old.c2cpb + item.h;
                        if (slip_r)node_c2->setFlagBit(FlagBit::DEFECT);
                        if (old.getFlagBit(FlagBit::BIN4)) { // c2cpu can not move
                            if (node_c2->c2cpu >= node_c2->c4cp) {
                                node_c2->setFlagBit(FlagBit::BIN4);
                                fesible.push_back(constraintCheck(old, *node_c2));
                            }
                        } else {
                            if (node_c2->c2cpu < node_c2->c4cp)
                                node_c2->c2cpu = node_c2->c4cp;
                            fesible.push_back(constraintCheck(old, *node_c2));
                        }
                    }
                    if (slip_u) {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c3 = nodes.rbegin();
                        node_c3->c3cp = old.c3cp + item.w;
                        if (node_c3->c1cpr < node_c3->c3cp)
                            node_c3->c1cpr = node_c3->c3cp;
                        node_c3->c4cp = old.c2cpb + slip_u + item.h;
                        node_c3->setFlagBit(FlagBit::DEFECT);
                        if (old.getFlagBit(FlagBit::BIN4)) {
                            if (node_c3->c2cpu >= node_c3->c4cp) {
                                node_c3->setFlagBit(FlagBit::BIN4);
                                fesible.push_back(constraintCheck(old, *node_c3));
                            }
                        } else {
                            if (node_c3->c2cpu < node_c3->c4cp)
                                node_c3->c2cpu = node_c3->c4cp;
                            fesible.push_back(constraintCheck(old, *node_c3));
                        }
                    }
                }
                if (old.c3cp + item.w > old.c1cpr) { // another branch, creat a new L2 and place item in it
                    slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c4 = nodes.rbegin();
                        node_c4->c3cp = old.c1cpl + slip_r + item.w;
                        if (node_c4->c1cpr < node_c4->c3cp)
                            node_c4->c1cpr = node_c4->c3cp;
                        node_c4->c2cpb = old.c2cpu;
                        node_c4->c4cp = node_c4->c2cpu = old.c2cpu + item.h;
                        node_c4->cut2 = old.cut2 + 1; // update new L2 id
                        if (slip_r)node_c4->setFlagBit(FlagBit::DEFECT);
                        fesible.push_back(constraintCheck(old, *node_c4));
                    }
                    if (slip_u) {
                        nodes.push_back(TreeNode(old, itemId, rotate));
                        auto node_c5 = nodes.rbegin();
                        node_c5->c3cp = old.c1cpl + item.w;
                        if (node_c5->c1cpr < node_c5->c3cp)
                            node_c5->c1cpr = node_c5->c3cp;
                        node_c5->c2cpb = old.c2cpu;
                        node_c5->c4cp = node_c5->c2cpu = old.c2cpu + slip_u + item.h;
                        node_c5->cut2 = old.cut2 + 1;
                        node_c5->setFlagBit(FlagBit::DEFECT);
                        fesible.push_back(constraintCheck(old, *node_c5));
                    }
                }
            }
        }
    }
    // choose the fesible and evaluate it
    List<NodeInfo> node_pool;
    node_pool.reserve(10);
    for (int i = 0; i < nodes.size(); ++i) {
        if (fesible[i]) { // a fesible node
            node_pool.push_back(NodeInfo(
                i, getBranchScore(old, nodes[i]), aux.items[nodes[i].item].w));
        }
    }
    if (node_pool.size()) {
        // sort the node by it's score, if score is the same, bigger item first
        sort(node_pool.begin(), node_pool.end(), [](NodeInfo &lhs, NodeInfo &rhs) {
            return lhs.item_w > rhs.item_w; }); // place bigger item first
        stable_sort(node_pool.begin(), node_pool.end(), [](NodeInfo &lhs, NodeInfo &rhs) {
            return lhs.score < rhs.score; }); // sort by score
        // random choose some nodes to branch
        int left_nodes = node_pool.size()*cfg.cut_rate;
        if (left_nodes < 1) // make sure at least choose one branch node
            left_nodes = 1;
        if (left_nodes < cfg.sampling_num) { // no need to sampling, just copy nodes to live_nodes
            for (int i = 0; i < left_nodes; ++i) {
                live_nodes.push_back(nodes[node_pool[i].id]);
            }
        } else { // sampling cfg.sampling_num branch nodes and push into live_nodes
            for (int i = left_nodes; i > left_nodes - cfg.sampling_num; --i) {
                const int rd_num = rand.pick(i); // random number range from 0 to i-1
                live_nodes.push_back(nodes[node_pool[i].id]);
                swap(node_pool.begin() + rd_num, node_pool.begin() + i - 1);
            }
        }
    } else { // no place to put item, creat new
        if (old.c2cpu == 0) { // place item in new plate
            TreeNode false_node(old.depth, old.plate + 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            branch(false_node, batch, live_nodes);
        } else { // place item in new cut1
            TreeNode false_node(old.depth, old.plate, -1, old.c1cpr, old.c1cpr, 0, 0, old.c1cpr, 0, old.cut1 + 1, 0, 0);
            branch(false_node, batch, live_nodes);
        }
    }
}

/* input: old branch node, new branch node
   function: check if new branch satisfy all the constraints
   if some constraints not staisfy, try to fix is by move item
*/
const bool TreeSearch::constraintCheck(const TreeNode &old, TreeNode &node) {
    bool moved_cut1 = false, moved_cut2 = false;

    // precheck some important constraints to speed up
    if (node.c1cpr > input.param.plateWidth || node.c2cpu > input.param.plateHeight) {
        return false;
    }
    // check minimum waste width/height constraint and try to fix it.
    if (node.c1cpr - node.c3cp < input.param.minWasteWidth &&
        node.c1cpr - node.c4cp > 0) { // check item right side and c1cpr interval.
        node.c1cpr = node.c3cp + input.param.minWasteWidth; // move c1cpr right to staisfy constraint.
        moved_cut1 = true;
    }
    if (node.c2cpu - node.c4cp < input.param.minWasteHeight &&
        node.c2cpu - node.c4cp > 0) { // check item up side and c2cpu interval.
        if (node.getFlagBit(FlagBit::BIN4)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu = node.c4cp + input.param.minWasteHeight; // move c2cpu up to satisfy constraint.
            moved_cut2 = true;
        }
    }
    // check not cut through defect constraint and try to fix it.
    if (moved_cut1 || old.c1cpr != node.c1cpr) {
        for (int i = 0; i < aux.plates_x[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_x[node.plate][i]];
            if (defect.x <= node.c1cpr) {
                if (defect.x + defect.w >= node.c1cpr) { // c1cpr cut through defect.
                    node.c1cpr = defect.x + defect.w; // move c1cpr -> defect right side to satisfy the constraint.
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
                if (defect.y + defect.h >= node.c2cpu) { // c2cpu cut through defect.
                    if (node.getFlagBit(FlagBit::BIN4)) { // can't move c2cpu to satisfy the constraint.
                        return false;
                    } else {
                        node.c2cpu = defect.y + defect.h; // move c2cpu -> defect up side to satisfy the constraint.
                    }
                }
            } else { // the leftover defects is in the upper of c2cpu.
                break;
            }
        }
    }
    // check stock bound exceed and minimum cut1/cut2 and maximum cut1 constraint.
    if (node.c1cpr > input.param.plateWidth) { // cut1 exceed stock right side.
        return false;
    }
    if (node.c2cpu > input.param.plateHeight) { // cut2 exceed stock up side.
        return false;
    }
    if (node.c1cpr - node.c1cpl > input.param.maxL1Width) { // maximum cut1 width not staisfy.
        return false;
    }
    if (node.cut1 > old.cut1 && old.c1cpr - old.c1cpl < input.param.minL1Width) { // minimum cut1 width not staisfy.
        return false;
    }
    if (node.cut2 > old.cut2 && old.c2cpu - old.c2cpb < input.param.minL2Height) { // minimum cut2 height not staisfy.
        return false;
    }
    // check clcpr/c2cpu and stock side's minimum waste width/height constraint
    if (input.param.plateWidth - node.c1cpr < input.param.minWasteWidth &&
        input.param.plateWidth - node.c1cpr != 0) { // cut1 and stock right side interval less than minimum waste width.
        return false;
    }
    if (input.param.plateHeight - node.c2cpu < input.param.minWasteHeight &&
        input.param.plateHeight - node.c2cpu != 0) { // cut2 and stock up side interval less than minimum waste height.
        return false;
    }
    return true; // every constraints satisfied.
}

/* input:rectangle area to place item, plate id
   function: if rectangle area conflict with one or more defects
   slip the area right to the rightmost defects right side
*/// output: slip length to avoid conflict with defect
const Length TreeSearch::sliptoDefectRight(const RectArea & area, const ID plate) const {
    Length slip = area.x;
    for (auto d : aux.plates_x[plate]) {
        if (aux.defects[d].x > slip + area.w) {
            break; // the defects is sorted by x position, so in this case just break
        }
        if (aux.defects[d].x > slip && aux.defects[d].y > area.y &&
            aux.defects[d].x + aux.defects[d].w < slip + area.w &&
            aux.defects[d].y + aux.defects[d].h < area.y + area.h) { // defect[d] conflict with area
            slip = aux.defects[d].x + aux.defects[d].w; // slip to defect right side
        }
    }
    if (slip - area.x < input.param.minWasteWidth && slip - area.x != 0)
        return input.param.minWasteWidth;
    return slip - area.x;
}

/* input:rectangle area to place item, plate id
   function: if rectangle area conflict with one or more defects
   slip the area up to the upmost defects up side
*/// output: slip length to avoid conflict with defect
const Length TreeSearch::sliptoDefectUp(const RectArea & area, const ID plate) const {
    Length slip = area.y;
    for (auto d : aux.plates_y[plate]) {
        if (aux.defects[d].y > slip + area.h) {
            break; // the defects is sorted by y position, so in this case just break
        }
        if (aux.defects[d].x > area.x && aux.defects[d].y > slip &&
            aux.defects[d].x + aux.defects[d].w < area.x + area.w &&
            aux.defects[d].y + aux.defects[d].h < slip + area.h) { // defect[d] conflict with area
            slip = aux.defects[d].y + aux.defects[d].h; // slip to defect up side
        }
    }
    if (slip - area.y < input.param.minWasteHeight && slip - area.y != 0)
        return input.param.minWasteHeight;
    return slip - area.y;
}

/* input: branch node to evaluate
   function: calculate the local waste area caused by the branch node
   the evaluate is not so precise, just as reference
*/// output: waste area
const double TreeSearch::getBranchScore(const TreeNode &old, const TreeNode & node) const {
    Area waste = 0;
    const Coord item_left = node.c3cp - aux.items[node.item].w;
    const Coord item_bottom = node.c4cp - aux.items[node.item].h;
    if (item_left > old.c3cp) { // place item in defect right caused waste
        waste += (item_left - old.c3cp)*(node.c2cpu - node.c2cpb);
    }
    if (item_bottom > node.c2cpb && node.getFlagBit(FlagBit::DEFECT)) { // place item in defect upper caused waste
        waste += (node.c3cp - old.c3cp)*(item_bottom - node.c2cpb);
    }
    if (node.c2cpu > node.c4cp) { // c2cpu and c4cp interval caused waste
        waste += (node.c2cpu - node.c4cp)*aux.items[node.item].w;
    }
    if (node.c2cpu > old.c2cpu) { // move c2cpu caused waste
        waste += (node.c2cpu - old.c2cpu)*(old.c3cp - old.c1cpl);
    }
    if (node.c1cpr > old.c1cpr) { // move c1cpr caused waste
        waste += (node.c1cpr - old.c1cpr)*node.c2cpb;
    }
    if (node.cut2 > old.cut2) { // last L2 and last item waste when creat new L2
        waste += (old.c1cpr - old.c3cp)*(old.c2cpu - old.c2cpb);
    }
    return waste/aux.item_area[node.item];
}

// this function convert TreeNode type solution into output type 
void TreeSearch::toOutput(const List<TreeNode> &sol) {
    using SpecialType = Problem::Output::Node::SpecialType;
    const Length PW = input.param.plateWidth, PH = input.param.plateHeight;
    // define current plate/cut1/cut2 identity in solution node
    ID cur_plate = -1, cur_cut1 = -1, cur_cut2 = -1;
    // define current node/parentL0.. identity in output solution tree node
    ID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    // define current c1cpl/c1cpr... when constructing solution tree
    Coord c1cpl = 0, c1cpr = 0, c2cpb=0, c2cpu = 0, c3cp = 0;
    // construct solution tree by visit each sol TreeNode
    for (int n = 0; n < sol.size(); ++n) {
        if (cur_plate < sol[n].plate) { // a new plate
            // add waste for last plate
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (c2cpu < input.param.plateHeight) { // add waste between c2cpu and plate up bound
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            if (c1cpr < input.param.plateWidth) { // add waste between c1cpr and plate right bound
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Waste, 1, parent_L0));
            }
            // update cut position and cut identity information
            cur_plate = sol[n].plate;
            cur_cut1 = 0;
            cur_cut2 = 0;
            c1cpl = 0;
            c1cpr = getC1cpr(sol, n, cur_cut1);
            c2cpb = 0;
            c2cpu = getC2cpu(sol, n, cur_cut2);
            c3cp = 0;
            // creat new nodes
            parent_L0 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new plate
                cur_plate, nodeId++, 0, 0, PW, PH, SpecialType::Branch, 0, -1));
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1
                cur_plate, nodeId++, 0, 0, c1cpr, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2
                cur_plate, nodeId++, 0, 0, c1cpr, c2cpu, SpecialType::Branch, 2, parent_L1));
        } 
        else if (cur_cut1 < sol[n].cut1) { // a new cut1
            // add waste for last L1
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (c2cpu < input.param.plateHeight) { // add waste between c2cpu and plate up bound
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            // update cut position and cut identity information
            cur_cut1 = sol[n].cut1;
            cur_cut2 = 0;
            c1cpl = c1cpr;
            c1cpr = getC1cpr(sol, n, cur_cut1);
            c2cpb = 0;
            c2cpu = getC2cpu(sol, n, cur_cut2);
            c3cp = c1cpl;
            // creat new nodes
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        } 
        else if (cur_cut2 < sol[n].cut2) { // a new cut2
            // add waste for last L2
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            // update cut position and cut identity information
            cur_cut2 = sol[n].cut2;
            c2cpb = c2cpu;
            c2cpu = getC2cpu(sol, n, cur_cut2);
            c3cp = c1cpl;
            // creat new nodes
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        const Length item_w = sol[n].getFlagBit(FlagBit::ROTATE) ? aux.items[sol[n].item].h : aux.items[sol[n].item].w;
        const Length item_h = sol[n].getFlagBit(FlagBit::ROTATE) ?aux.items[sol[n].item].w : aux.items[sol[n].item].h;
        // if item placed in defect right bound, creat waste between c3cp and item left bound
        if (sol[n].getFlagBit(FlagBit::DEFECT) && sol[n].c3cp - item_w > c3cp) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // if has bin4, place this node into last L3, creat a new L3
        if (!sol[n].getFlagBit(FlagBit::BIN4)) {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - c3cp, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        // if item placed in defect up bound, creat waste between item bottom and c2cpb
        if (sol[n].getFlagBit(FlagBit::DEFECT) && sol[n].c4cp - item_h > c2cpb) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - c3cp, sol[n].c4cp - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
        }
        // creat a item in L3
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c3cp, sol[n].c4cp - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
        // if item up and c2cpu have interval, creat waste between item up and c2cpu
        if (c2cpu > sol[n].c4cp) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, sol[n].c4cp, sol[n].c3cp - c3cp, c2cpu -  sol[n].c4cp, SpecialType::Waste, 4, parent_L3));
        }
        // if next item is not in the same L3, update c3cp
        if (n+1 < sol.size() && !sol[n + 1].getFlagBit(FlagBit::BIN4)) {
            c3cp = sol[n].c3cp;
        }
    }
    if (c3cp < c1cpr) { // add last waste between c3cp and c1cpr
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
    }
    if (c2cpu < input.param.plateHeight) { // add last waste between c2cpu and plate up bound
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
    }
    if (c1cpr < input.param.plateWidth) { // add last residual between c1cpr and plate right bound
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Residual, 1, parent_L0));
    }
}

/* input: solution TreeNode type, solution start index, current cut1 id
*/// ouput: current cut1's c1cpr
const Coord TreeSearch::getC1cpr(const List<TreeNode> &sol, const int index, const ID cur_cut1) const {
    Coord res = 0;
    for (int i = index; i < sol.size(); ++i) {
        if (cur_cut1 == sol[i].cut1) {
            if (res < sol[i].c1cpr)
                res = sol[i].c1cpr;
        } else {
            break;
        }
    }
    return res;
}

/* input: solution TreeNode type, solution start index, current cut2 id
*/// ouput: current cut2's c2cpu
const Coord TreeSearch::getC2cpu(const List<TreeNode> &sol, const int index, const ID cur_cut2) const {
    Coord res = 0;
    for (int i = index; i < sol.size(); ++i) {
        if (cur_cut2 == sol[i].cut2) {
            if (res < sol[i].c2cpu)
                res = sol[i].c2cpu;
        } else {
            break;
        }
    }
    return res;
}

#pragma endregion Achievement

}