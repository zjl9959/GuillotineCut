///////////////////////////
///usage: the tree search algorithm for the problem.
///
//////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H
#define SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H

#include "Config.h"
#include "Solver.h"

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
    enum FlagBit {ROTATE = 0, DEFECT_R = 1, DEFECT_U = 2, BIN4 = 3, LOCKC2 = 4};

    struct Configuration {
        TID sampling_num = 100; // maximum sample node number from the good branch nodes
        double cut_rate = 1; // cut half of the branch node and sample
        String toBriefStr() const {
            std::ostringstream os;
            os << "sampling_num=" << sampling_num
                << ";cut_rate=" << cut_rate;
            return os.str();
        }
    };

    struct Rect {
        Rect() {}
        Rect(TLength width, TLength height) : w(width), h(height) {}

        TLength w; // width.
        TLength h; // height.
    };
    struct RectArea : public Rect { // a rectangular area on the plate.
        RectArea() {}
        RectArea(TCoord left, TCoord bottom, TLength width, TLength height) : Rect(width, height), x(left), y(bottom) {}

        TCoord x; // left.
        TCoord y; // bottom.
    };

    struct TreeNode {
        Depth depth; // node depth in the tree.
        TID plate; // plate id.
        TID item = Problem::InvalidItemId; // item id.
        TCoord c1cpl; // current 1-cut position left.
        TCoord c1cpr; // current 1-cut position right.
        TCoord c2cpb; // current 2-cut position bottom.
        TCoord c2cpu; // current 2-cut position up.
        TCoord c3cp; // current 3-cut position right.
        TCoord c4cp; // current 4-cut position up.
        TID cut1; // 1-cut id.
        TID cut2; // 2-cut id.
        // (flag&0x01)->(rotate);(flag&0x02)->(item place in defect side);(flag&0x04)->(item place in bin4);(flag&0x08)->(1:c2cpu change not allowed)
        Status flag = 0;

        TreeNode(Depth node_depth, TID plate_id, TID item_id, TCoord C1cpl, TCoord C1cpr,
            TCoord C2cpb, TCoord C2cpu, TCoord C3cp, TCoord C4cp, TID cut1_id, TID cut2_id, Status flag)
            :depth(node_depth), plate(plate_id), item(item_id), c1cpl(C1cpl), c1cpr(C1cpr),
            c2cpb(C2cpb), c2cpu(C2cpu), c3cp(C3cp), c4cp(C4cp), cut1(cut1_id), cut2(cut2_id), flag(flag) {}

        TreeNode(const TreeNode &node, const TID item_id, const int dir) :depth(node.depth + 1), plate(node.plate), item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
            c2cpb(node.c2cpb), c2cpu(node.c2cpu), c3cp(node.c3cp), c4cp(node.c4cp), cut1(node.cut1), cut2(node.cut2), flag(dir) {}
        
        void setFlagBit(const int bit_pos = FlagBit::ROTATE) { flag |= (0x0001 << bit_pos); }
        const bool getFlagBit(const int bit_pos = FlagBit::ROTATE) const { return flag & (0x0001 << bit_pos); }
    };

    struct NodeInfo {
        int id;
        double score;
        TLength item_w;
        NodeInfo(int ID, double Score, TLength item_width)
            :id(ID), score(Score), item_w(item_width) {}
    };
    #pragma endregion Type
    
    #pragma region Constructor
public:
    TreeSearch(const Problem::Input &inputData, const Solver::Environment &environment, const Configuration &config)
        : input(inputData), env(environment), cfg(config), rand(environment.randSeed),
        timer(std::chrono::milliseconds(environment.msTimeout)), iteration(1), total_item_area(0) {}
    #pragma endregion Constructor

    #pragma region Method
public:
    void solve();
    void record() const;
protected:
    void init();
    void depthFirstSearch(const int upbound, const TreeNode &resume_point, List<List<TID>> &batch, List<TreeNode> &solution);
    void branch(const TreeNode &old, const List<List<TID>> &batch, List<TreeNode> &live_nodes);
    const bool constraintCheck(const TreeNode &old, TreeNode &node);
    const TLength sliptoDefectRight(const RectArea &area, const TID plate) const;
    const TLength sliptoDefectUp(const RectArea &area, const TID plate) const;
    const double getBranchScore(const TreeNode &old, const TreeNode &node) const;
    void toOutput(List<TreeNode> &sol);
    const TCoord getC1cpr(const List<TreeNode> &sol, const int index, const TID cur_cut1) const;
    const TCoord getC2cpu(const List<TreeNode> &sol, const int index, const TID cur_cut2) const;
    bool check(Length &checkerObj) const;
    #pragma endregion Method

    #pragma region Field
public:
    Problem::Input input;
    Problem::Output output;

    struct {
        List<Rect> items;
        List<RectArea> defects;
        List<Area> item_area; // item area size of every item.
        List<TID> defect2stack; //defect identity to stack identity

        List<List<TID>> stacks; // stacks[s][i] is the itemId of the i_th item in the stack s.
        List<List<TID>> plates_x; // plates[p][i] is the defectId of the i_th defect on plate p, sorted by defect x position.
        List<List<TID>> plates_y; // plates[p][i] is the defectId of the i_th defect on plate p, sorted by defect y position.
    } aux;

    struct {
        ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxItemNum> item;
        ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxStackNum> stack;
        ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxDefectNum> defect;
        ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxPlateNum> plate;
    } idMap;

    struct {
        // TODO: add information about tree search.
    } info;

    Configuration cfg;
    Solver::Environment env;

    Random rand; // all random number in TSolver must be generated by this.
    Timer timer; // the solve() should return before it is timeout.
    int iteration; // total iteration nodes
    Area total_item_area;

    #pragma endregion Field
};

}

#endif // SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H
