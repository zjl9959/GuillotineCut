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

void TreeSearch::branch(const TreeNode &old, const Coord bound, List<TreeNode> &live_nodes) {
    List<TreeNode> nodes;
    nodes.reserve(10);
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
            // classify and try to branch.
            TreeNode temp_node(old, itemId);
            if (rotate)temp_node.setFlagBit(FlagBit::ROTATE);
            // TODO: what if item is too larger?
            if (old.c2cpb == 0 && old.c2cpu == 0) {
                // TODO: add code...
            }
        }
    }
}

const bool TreeSearch::constraintCheck(const TreeNode &old, TreeNode &node) {
    bool moved_cut1 = false, moved_cut2 = false;

    // precheck some important constraints to speed up
    if (node.c1cpr > input.param.plateWidth) { // cut1 exceed stock right side.
        return false;
    }
    if (node.c2cpu > input.param.plateHeight) { // cut2 exceed stock up side.
        return false;
    }
    if (node.c1cpr - node.c1cpl > input.param.maxL1Width) { // maximum cut1 width not staisfy.
        return false;
    }

    // check minimum waste width/height constraint and try to fix it.
    if (node.getFlagBit(FlagBit::DEFECT)) { // item placed in defect side.
        // check item left side and old c3cp interval(when item palced in defect right).
        if (node.c3cp - aux.items[node.item].w - old.c3cp < input.param.minWasteWidth) {
            node.c3cp = old.c3cp + input.param.minWasteWidth + aux.items[node.item].w; // move item(c3cp) right to staisfy constraint.
            if (node.c3cp > node.c1cpr) { // update c1cpr if c3cp exceed c1cpr after move right.
                node.c1cpr = node.c3cp;
                moved_cut1 = true;
            }
        }
        // check item bottom side and c2cpb interval(when item palced in defect upper).
        if (node.c4cp - aux.items[node.item].h - node.c2cpb < input.param.minWasteHeight) {
            node.c4cp = node.c2cpb + input.param.minWasteHeight + aux.items[node.item].h; // move item(c4cp) up to staisfy constraint.
            if (node.c4cp > node.c2cpu) { // update c2cpu if c4cp exceed c2cpu after move up.
                node.c2cpu = node.c4cp;
                moved_cut2 = true;
            }
        }
    }
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
    if (node.c1cpr - old.c1cpr > 0 && node.cut1 == old.cut1 &&
        node.c1cpr - old.c1cpr < input.param.minWasteWidth) { // check new clcpr and old c1cpr interval.
        node.c1cpr = old.c1cpr + input.param.minWasteWidth; // move c1cpr right.
        moved_cut1 = true;
    }
    if (node.c2cpu - old.c2cpu > 0 && node.cut2 == old.cut2 &&
        node.c2cpu - old.c2cpu < input.param.minWasteHeight) { // check new c2cpu and old c2cpu interval.
        if (node.getFlagBit(FlagBit::BIN4)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu = old.c2cpu + input.param.minWasteHeight; // move c2cpu up.
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
    if (input.param.plateWidth - node.c1cpr < input.param.minWasteWidth && 
        input.param.plateWidth - node.c1cpr != 0) { // cut1 and stock right side interval less than minimum waste width.
        return false;
    }
    if (input.param.plateHeight - node.c2cpu < input.param.minWasteHeight &&
        input.param.plateHeight - node.c2cpu != 0) { // cut2 and stock up side interval less than minimum waste height.
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
    // every constraints satisfied.
    return true;
}
#pragma endregion Achievement

}