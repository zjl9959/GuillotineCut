///////////////////////////
///usage: the tree search algorithm for the problem.
///
//////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H
#define SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H

#include "Config.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <sstream>
#include <thread>

#include "Common.h"
#include "Utility.h"
#include "Problem.h"

namespace szx {

class TreeSearch {
    #pragma region Type
public:
    enum FlagBit {ROTATE = 0, DEFECT = 1, BIN4 = 2, LOCKCUT2 = 3};

    struct Configuration {
        ID sampling_num = 3; // maximum sample node number from the good branch nodes
        double cut_rate = 0.5; // cut half of the branch node and sample
    };

    struct Rect {
        Rect() {}
        Rect(Length width, Length height) : w(width), h(height) {}

        Length w; // width.
        Length h; // height.
    };
    struct RectArea : public Rect { // a rectangular area on the plate.
        RectArea() {}
        RectArea(Coord left, Coord bottom, Length width, Length height) : Rect(width, height), x(left), y(bottom) {}

        Coord x; // left.
        Coord y; // bottom.
    };

    struct TreeNode {
        Depth depth; // node depth in the tree.
        ID plate; // plate id.
        ID item = Problem::InvalidItemId; // item id.
        Coord c1cpl; // current 1-cut position left.
        Coord c1cpr; // current 1-cut position right.
        Coord c2cpb; // current 2-cut position bottom.
        Coord c2cpu; // current 2-cut position up.
        Coord c3cp; // current 3-cut position right.
        Coord c4cp; // current 4-cut position up.
        ID cut1; // 1-cut id.
        ID cut2; // 2-cut id.
        Status flag = 0; // (flag&0x0001)->(1:rotate);(flag&0x0002)->(1:create L4 not allowed);(flag&0x0004)->(1:c2cpu change not allowed).

        TreeNode(Depth node_depth, ID plate_id, ID item_id, Coord C1cpl, Coord C1cpr,
            Coord C2cpb, Coord C2cpu, Coord C3cp, Coord C4cp, ID cut1_id, ID cut2_id, Status flag)
            :depth(node_depth), plate(plate_id), item(item_id), c1cpl(C1cpl), c1cpr(C1cpr),
            c2cpb(C2cpb), c2cpu(C2cpu), c3cp(C3cp), c4cp(C4cp), cut1(cut1_id), cut2(cut2_id), flag(flag) {}

        TreeNode(const TreeNode &node, const ID item_id, const int dir) :depth(node.depth + 1), plate(node.plate), item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
            c2cpb(node.c2cpb), c2cpu(node.c2cpu), c3cp(node.c3cp), c4cp(node.c4cp), cut1(node.cut1), cut2(node.cut2), flag(dir) {}
        
        void setFlagBit(const int bit_pos = FlagBit::ROTATE) { flag |= (0x0001 << bit_pos); }
        const bool getFlagBit(const int bit_pos = FlagBit::ROTATE) const { return flag & (0x0001 << bit_pos); }
    };

    struct NodeInfo {
        int id;
        double score;
        Length item_w;
        NodeInfo(int ID, double Score, Length item_width)
            :id(ID), score(Score), item_w(item_width) {}
    };
    #pragma endregion Type
    
    #pragma region Constructor
public:
    TreeSearch(const Problem::Input &inputData, const Configuration &config, Duration timeout)
        : input(inputData), cfg(config), timer(std::chrono::milliseconds(timeout)) {};
    #pragma endregion Constructor

    #pragma region Method
public:
    void solve();
protected:
    void init();
    void depthFirstSearch(const int upbound, List<List<ID>> &batch, List<TreeNode> &solution);
    void branch(const TreeNode &old, const List<List<ID>> &batch, List<TreeNode> &live_nodes);
    const bool constraintCheck(const TreeNode &old, TreeNode &node);
    const Length sliptoDefectRight(const RectArea &area, const ID plate) const;
    const Length sliptoDefectUp(const RectArea &area, const ID plate) const;
    const double getBranchScore(const TreeNode &old, const TreeNode &node) const;
    void toOutput(const List<TreeNode> &sol);
    const Coord getC1cpr(const List<TreeNode> &sol, const int index, const ID cur_cut1) const;
    const Coord getC2cpu(const List<TreeNode> &sol, const int index, const ID cur_cut2) const;
    #pragma endregion Method

    #pragma region Field
public:
    Problem::Input input;
    Problem::Output output;

    struct {
        List<Rect> items;
        List<RectArea> defects;
        List<Area> item_area; // item area size of every item.
        List<ID> defect2stack; //defect identity to stack identity

        List<List<ID>> stacks; // stacks[s][i] is the itemId of the i_th item in the stack s.
        List<List<ID>> plates_x; // plates[p][i] is the defectId of the i_th defect on plate p, sorted by defect x position.
        List<List<ID>> plates_y; // plates[p][i] is the defectId of the i_th defect on plate p, sorted by defect y position.
    } aux;

    struct {
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxItemNum> item;
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxStackNum> stack;
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxDefectNum> defect;
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxPlateNum> plate;
    } idMap;

    struct {
        // TODO: add information about tree search.
    } info;

    Configuration cfg;

    Random rand; // all random number in TSolver must be generated by this.
    Timer timer; // the solve() should return before it is timeout.

    #pragma endregion Field
};

}

#endif // SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H
