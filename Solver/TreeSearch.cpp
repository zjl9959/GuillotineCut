/*note: use absolute coordinates in the algoritm
*/
#include "TreeSearch.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>

#include <cmath>

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
    aux.plates.resize(Problem::MaxPlateNum);
    for (ID p = 0; p < Problem::MaxPlateNum; ++p) { idMap.plate.toConsecutiveId(p); }

    // map defects to plates.
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        ID defectId = idMap.defect.toConsecutiveId(d->id);
        aux.defects.push_back(RectArea(d->x, d->y, d->width, d->height));

        ID plateId = idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plates.size() <= plateId) { aux.plates.resize(plateId + 1); } // create new plates.
        aux.plates[plateId].push_back(defectId);
    }

    aux.item_area.reserve(aux.items.size());
    //calculate item areas
    for (int i = 0; i < aux.items.size(); ++i)
        aux.item_area.push_back(aux.items[i].h*aux.items[i].w);
    // reverse item sequence convicent for pop_back
    for (int i = 0; i < aux.stacks.size(); i++)
        reverse(aux.stacks[i].begin(), aux.stacks[i].end());
}

/* input:plate id, start 1-cut position, maximum used width, the batch to be used, solution vector
   use depth first search to optimize partial solution
*/
void TreeSearch::depthFirstSearch(const ID plate, const Coord start, const Length ub, const List<List<ID>>& batch, List<TreeNode>& solution) {
    // TODO: add code...
}

void TreeSearch::branch(const TreeNode &node,const Coord bound, List<TreeNode> &live_nodes) {
    List<TreeNode> nodes;
    nodes.reserve(10);
    for (int rotate = 0; rotate <= 1; ++rotate) {
        for (auto stack : aux.stacks) {
            // pretreatment
            if (!stack.size())continue; // skip empty stack
            ID itemId = stack.back();
            Rect item = aux.items[itemId];
            if (item.w == item.h && rotate)continue; // skip rectangle item                                     //no need to rotate square
            if (rotate) { // rotate item                                                               //rotate item
                item.w = aux.items[itemId].h;
                item.h = aux.items[itemId].w;
            }
            // classify and try to branch
            TreeNode temp_node(node, itemId);
            if (rotate)temp_node.setFlagBit(FlagBit::ROTATE);
            // TODO: what if item is too larger?
            if (node.c2cpb == 0 && node.c2cpu == 0) {

            }
        }
    }
}

const bool TreeSearch::constraintCheck(const TreeNode &node,const Coord bound) {
    // TODO: add code...
    return false;
}
#pragma endregion Achievement

}