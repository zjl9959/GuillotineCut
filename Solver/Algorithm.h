#pragma once
#ifndef SMART_ZJL_GUILLOTINE_CUT_ALGORITHM_H
#define SMART_ZJL_GUILLOTINE_CUT_ALGORITHM_H

#include <sstream>
#include "Config.h"
#include "Utility.h"
#include "Common.h"
#include "Problem.h"

using namespace szx;

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

    const String tostr() {
        std::ostringstream os;
        os << "item: " << item
            << " c1cpl: " << c1cpl
            << " c1cpr: " << c1cpr
            << " c2cpb: " << c2cpb
            << " c2cpu: " << c2cpu
            << " c3cp: " << c3cp
            << " c4cp: " << c4cp;
        if (this->getFlagBit(NEW_PLATE))
            os << " +P ";
        if (this->getFlagBit(NEW_L1))
            os << " +L1 ";
        if (this->getFlagBit(NEW_L2))
            os << " +L2 ";
        return os.str();
    }
};

struct ItemNode : public SolutionNode {
    Depth depth;  // node depth in the tree.
    Score score;
    ItemNode(TCoord C1cpl) : SolutionNode(C1cpl), depth(-1), score(0.0) {}
    ItemNode(const ItemNode &node, const TID item_id, const Status _flag, const Score _score = 0.0) :
        SolutionNode(node, item_id, _flag), depth(node.depth + 1), score(_score) {}
};

struct Configuration {
	int mcin; // maximum choose item number.
	int mbpn; // maximum branch plate number.
	int mbcn; // maximum branch 1-cut number.
	String toBriefStr() const {
		std::ostringstream os;
		os << "GB2"
			<< ";mcin=" << mcin
			<< ";mbpn=" << mbpn
			<< ";mbcn=" << mbcn;
		return os.str();
	}
	Configuration(int MCIN = 8, int MBPN = 4, int MHCN = 1) :
		mcin(MCIN), mbpn(MBPN), mbcn(MHCN) {}
};

namespace GV {  // global variables
	extern List<Rect> items;
	extern List<Area> item_area;
	extern List<List<TID>> stacks; // stacks[s][i] is the `i`_th item in stack `s`.
	extern List<TID> item2stack;   // item2stack[i] is the stack of item `i`.
	extern List<List<Defect>> plate_defect_x; // plate_defect_x[p][i] is the `i`_th defect on plate p, sorted by defect x position.
	extern List<List<Defect>> plate_defect_y; // plate_defect_y[p][i] is the `i`_th defect on plate p, sorted by defect y position.
	extern IdMap idMap;
	extern Problem::Input::Param param;
	extern int item_num;
	extern int stack_num;
	extern int support_thread;
}

using MySolution = List<SolutionNode>;
using ScorePair = std::pair<int, Score>;
using AreaPair = std::pair<int, Area>;
using LengthPair = std::pair<int, Length>;

struct MyStack {
	TID begin;
	TID end;
	MyStack(TID b, TID e) : begin(b), end(e) {}
	MyStack() : MyStack(0, 0) {}

	bool empty() const { return begin == end; }
	int size() const { return end - begin; }
	TID top() const { return end - 1; }
	TID bottom() const { return begin; }
	void pop() { --end; }
	void push() { ++end; }
	List<TID> recoverStack(TID stack_id) {
		List<TID> recover_stack; recover_stack.reserve(this->size());
		for (int i = begin; i < end; ++i) { recover_stack.push_back(GV::stacks[stack_id][i]); }
		return recover_stack;
	}
};

#endif // !SMART_ZJL_GUILLOTINE_CUT_ALGORITHM_H
