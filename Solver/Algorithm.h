#pragma once
#ifndef SMART_ZJL_GUILLOTINE_CUT_ALGORITHM_H
#define SMART_ZJL_GUILLOTINE_CUT_ALGORITHM_H

#include "Config.h"

#include <mutex>

#include "Utility.h"
#include "Common.h"
#include "Problem.h"

namespace szx {

struct Rect {
    TLength w;
    TLength h;

    Rect() {}
    Rect(TLength _w, TLength _h): w(_w), h(_h) {}
};

struct Defect {
    TID id;
    TCoord x;
    TCoord y;
    TLength w;
    TLength h;

    Defect() {}
    Defect(TID _id, TCoord _x, TCoord _y, TLength _w, TLength _h) :
        id(_id), x(_x), y(_y), w(_w), h(_h) {}
};

struct IdMap {
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxItemNum> item;
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxStackNum> stack;
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxDefectNum> defect;
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxPlateNum> plate;
};

enum FlagBit {
    ROTATE = 0x0001,     // indicate item direction
    DEFECT_R = 0x0002,   // item placed in defect right
    DEFECT_U = 0x0004,   // item placed in defect upper
    BIN4 = 0x0008,       // two items placed in the same L3
    LOCKC2 = 0x0010,     // c2cpu can't move
    NEW_PLATE = 0x0020,  // place item in a new plate
    NEW_L1 = 0x0040,     // place item in a new L1
    NEW_L2 = 0x0080,     // place item in a new L2
};
struct SolutionNode {
    TID item;     // item id.
    TCoord c1cpl; // current 1-cut position left.
    TCoord c1cpr; // current 1-cut position right.
    TCoord c2cpb; // current 2-cut position bottom.
    TCoord c2cpu; // current 2-cut position up.
    TCoord c3cp;  // current 3-cut position right.
    TCoord c4cp;  // current 4-cut position up.
    Status flag;

    // construct a node based on c1cpl
    SolutionNode(const TCoord C1cpl) : item(Problem::InvalidItemId), c1cpl(C1cpl), c1cpr(C1cpl), c2cpb(0),
        c2cpu(0), c3cp(C1cpl), c4cp(0), flag(0) {};
    // construct a node based on last node.
    SolutionNode(const SolutionNode &node, const TID item_id, const Status _flag) : item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
        c2cpb(node.c2cpb), c2cpu(node.c2cpu), c3cp(node.c3cp), c4cp(node.c4cp), flag(_flag) {}

    void setFlagBit(const int bit_flag = ROTATE) { flag |= bit_flag; }
    const bool getFlagBit(const int bit_flag = ROTATE) const { return flag & bit_flag; }
};

struct ItemNode : public SolutionNode {
    Depth depth;
    Score score;
    ItemNode(TCoord C1cpl) : SolutionNode(C1cpl), depth(0), score(0.0) {}
    ItemNode(const ItemNode &node, const TID item_id, const Status _flag, const Score _score = 0.0) :
        SolutionNode(node, item_id, _flag), depth(node.depth + 1), score(_score) {}
};

struct CutNode {
    TLength width;      // this cut width.
    Score usage_rate;   // this cut usage rate.
    List<SolutionNode> nodes;   // this cut contain solution nodes.
    
    CutNode(const TLength _width, const Score _usage_rate) : width(_width), usage_rate(_usage_rate) {}
    CutNode(const CutNode &rhs) :nodes(rhs.nodes), width(rhs.width), usage_rate(rhs.usage_rate) {}
    CutNode(CutNode &&rhs) :width(rhs.width), usage_rate(rhs.usage_rate) { nodes.swap(rhs.nodes); }

    CutNode& operator=(const CutNode &rhs) {
        nodes = rhs.nodes;
        width = rhs.width;
        usage_rate = rhs.usage_rate;
        return *this;
    }
    CutNode& operator=(CutNode &&rhs) {
        width = rhs.width;
        usage_rate = rhs.usage_rate;
        nodes.swap(rhs.nodes);
        return *this;
    }

    void add_solution_nodes(List<ItemNode> &_nodes) {
        nodes.reserve(_nodes.size());
        for(auto it = _nodes.begin(); it != _nodes.end(); ++it) {
            nodes.push_back(*it);
        }
    }
};
bool operator<(const CutNode &lhs, const CutNode &rhs) {
    return Math::strongLess(lhs.usage_rate, rhs.usage_rate, 0.0001) ||
        (Math::weakEqual(lhs.usage_rate, rhs.usage_rate, 0.0001) && lhs.width < rhs.width);
}

struct PlateNode {
    TID plate_id;       // this plate identity.
    Score usage_rate;   // this plate usage rate.
    List<SolutionNode> nodes;   // this plate conatin solution nodes.

    PlateNode(const TID _plate_id) : plate_id(_plate_id), usage_rate(0.0) {}
    PlateNode(const TID _plate_id, const Score _usage_rate, const List<SolutionNode> &_nodes) :
        plate_id(_plate_id), usage_rate(_usage_rate), nodes(_nodes) {}
    PlateNode(const TID _plate_id, const Score _usage_rate, List<SolutionNode> &&_nodes) :
        plate_id(_plate_id), usage_rate(_usage_rate) { nodes.swap(_nodes); }
    PlateNode(const PlateNode &rhs) : plate_id(rhs.plate_id), usage_rate(rhs.usage_rate), nodes(rhs.nodes) {}
    PlateNode(PlateNode &&rhs) : plate_id(rhs.plate_id), usage_rate(rhs.usage_rate) { nodes.swap(rhs.nodes); }

    PlateNode& operator=(const PlateNode &rhs) {
        plate_id = rhs.plate_id;
        usage_rate = rhs.usage_rate;
        nodes = rhs.nodes;
        return *this;
    }
    PlateNode& operator=(PlateNode &&rhs) {
        plate_id = rhs.plate_id;
        usage_rate = rhs.usage_rate;
        nodes.swap(rhs.nodes);
        return *this;
    }
};
bool operator<(const PlateNode &lhs, const PlateNode &rhs) {
    return Math::strongLess(lhs.usage_rate, rhs.usage_rate, 0.0001);
}

namespace GV {  // global variables

extern List<Rect> items;
extern List<int> item2stack;
extern List<Area> item_area;
extern List<List<TID>> stacks;
extern List<List<Defect>> plate_defect_x;
extern List<List<Defect>> plate_defect_y;
extern IdMap idMap;
extern Problem::Input::Param param;

}

}

#endif
