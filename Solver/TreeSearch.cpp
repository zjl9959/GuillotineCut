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
void TreeSearch::depthFirstSearch(const ID plate, const Coord start, const Length ub, const List<List<ID>>& batch, List<TreeNode>& solution) {
    // TODO: add code...
}

/* input:last tree node(branch point)
   function:branch tree node by exceed method
*/// output:push branched nodes into live_nodes
void TreeSearch::branch(const TreeNode &old, List<TreeNode> &live_nodes) {
    List<TreeNode> nodes;
    List<bool> fesible;
    nodes.reserve(10);
    fesible.reserve(10);
    for (int rotate = 0; rotate <= 1; ++rotate) {
        for (auto stack : aux.stacks) {
            // pretreatment.
            if (!stack.size())continue; // skip empty stack.
            ID itemId = stack.back();
            Rect item = aux.items[itemId];
            if (item.w == item.h && rotate)continue; // skip rectangle item.
            if (rotate) { // rotate item.
                item.w = aux.items[itemId].h;
                item.h = aux.items[itemId].w;
            }
            // check if item conflict with defect and try to fix it
            const Length slip_r = sliptoDefectRight(
                RectArea(old.c3cp, old.c4cp, item.w, item.h), old.plate);
            const Length slip_u = sliptoDefectUp(
                RectArea(old.c3cp, old.c4cp, item.w, item.h), old.plate);
            // TODO: what if item is too larger?
            if (old.c2cpu == 0) { // case A
                if (slip_r) { // item conflict with defect(two choice)
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place in defect right
                    auto node_a1 = nodes.rbegin(); // get iterator to nodes last element
                    node_a1->c3cp = node_a1->c1cpr = old.c1cpr + slip_r + item.w;
                    node_a1->c4cp = node_a1->c2cpu = item.h;
                    node_a1->cut1 = old.cut1 + 1;
                    node_a1->cut2 = 0;
                    fesible.push_back(constraintCheck(old, *node_a1));
                    nodes.push_back(TreeNode(old, itemId, rotate)); // place in defect up
                    auto node_a2 = nodes.rbegin();
                    node_a2->c3cp = node_a2->c1cpr = old.c1cpr + item.w;
                    node_a2->c4cp = node_a2->c2cpu = slip_u + item.h;
                    node_a2->cut1 = old.cut1 + 1;
                    node_a2->cut2 = 0;
                    fesible.push_back(constraintCheck(old, *node_a2));
                } else { // item not conflict with defect(one choice)
                    nodes.push_back(TreeNode(old, itemId, rotate));
                    auto node_a3 = nodes.rbegin();
                    node_a3->c3cp = node_a3->c1cpr = old.c1cpr + item.w;
                    node_a3->c4cp = node_a3->c2cpu = item.h;
                    node_a3->cut1 = old.cut1 + 1;
                    node_a3->cut2 = 0;
                }
            } else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) { // case B
                // TODO: add code...
            } else { // case C
                // TODO: add code...
            }
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
    if (slip - area.y < input.param.minWasteHeight&&slip - area.y != 0)
        return input.param.minWasteHeight;
    return slip - area.y;
}

#pragma endregion Achievement

}