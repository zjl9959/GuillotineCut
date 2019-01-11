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
    enum FlagBit {
        ROTATE = 0, // indicate item direction
        DEFECT_R = 1, // item placed in defect right
        DEFECT_U = 2, // item placed in defect upper
        BIN4 = 3, // two items placed in the same L3
        LOCKC2 = 4, // c2cpu can't move
    };

    struct Configuration {
        TID mcin = 8; // maximum choose item number.
        int mbpn = 8; // maximum branch plate number.
        int mhcn = 3; // maximum hopeful 1-cut number.
        int rcin = 5; // repeat choose item number.
        String toBriefStr() const {
            std::ostringstream os;
            os << "searchForward"
                << ";mcin=" << mcin
                << ";mbpn=" << mbpn
                << ";mhcn=" << mhcn
                << ";rcin=" << rcin;
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

        TreeNode() {};

        TreeNode(Depth node_depth, TID plate_id, TID item_id, TCoord C1cpl, TCoord C1cpr,
            TCoord C2cpb, TCoord C2cpu, TCoord C3cp, TCoord C4cp, TID cut1_id, TID cut2_id, Status flag)
            :depth(node_depth), plate(plate_id), item(item_id), c1cpl(C1cpl), c1cpr(C1cpr),
            c2cpb(C2cpb), c2cpu(C2cpu), c3cp(C3cp), c4cp(C4cp), cut1(cut1_id), cut2(cut2_id), flag(flag) {}

        TreeNode(const TreeNode &node, const TID item_id, const int dir) :depth(node.depth + 1), plate(node.plate), item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
            c2cpb(node.c2cpb), c2cpu(node.c2cpu), c3cp(node.c3cp), c4cp(node.c4cp), cut1(node.cut1), cut2(node.cut2), flag(dir) {}
        
        void setFlagBit(const int bit_pos = FlagBit::ROTATE) { flag |= (0x0001 << bit_pos); }
        const bool getFlagBit(const int bit_pos = FlagBit::ROTATE) const { return flag & (0x0001 << bit_pos); }
    };

    struct PlateSol {
        Area item_area = 0; // total item area in this plate.
        List<TreeNode> nodes; // solution nodes in this plate.
    };

    #pragma endregion Type
    
    #pragma region Constructor
public:
    TreeSearch(const Problem::Input &inputData, const Solver::Environment &environment, const Configuration &config)
        : input(inputData), env(environment), cfg(config), rand(environment.randSeed),
        timer(std::chrono::milliseconds(environment.msTimeout)) {}
    #pragma endregion Constructor

    #pragma region Method
public:
    void solve();
    void record() const;
protected:
    void init();
    void greedyBranchOptimize();
    Length evaluateOnePlate(const List<List<TID>>& source_batch, const List<TreeNode>& fixed_sol, const List<TreeNode>& psol);
    void getSomePlateSolutions(const TID plateId, const List<List<TID>>& source_batch, List<List<TreeNode>>& psols);
    void getOptimalPlateSolution(const TID plateId, const List<List<TID>>& source_batch, List<TreeNode>& psol);
    Area evaluateOneCut(const List<List<TID>>& source_batch, List<TreeNode>& hopeful_sol);
    void iteratorImproveWorstPlate();
    void getPlatesUsageRate(const List<TreeNode>& solution, List<double>& usage_rate);
    const int estimateDefectNumber(const TreeNode& resume_point, const List<List<TID>>& source_bacth);
    const int createItemBatchs(int nums, const TreeNode& resume_point, const List<List<TID>>& source_batch, List<List<List<TID>>>& target_batch);
    double optimizeOneCut(const TreeNode &resume_point, List<List<TID>> &batch, List<TreeNode> &solution);
    double optimizePlateTail(const TreeNode &resume_point, List<List<TID>> &batch, List<TreeNode> &solution);
    void optimizeTotalProblem();
    void partialBranch(const TreeNode &old, const List<List<TID>> &batch, const List<TreeNode> &cur_parsol, List<TreeNode> &branch_nodes);
    void totalBranch(const TreeNode &old, const List<List<TID>> &batch, const List<TreeNode> &cur_parsol, List<TreeNode> &branch_nodes);
    const bool constraintCheck(const TreeNode &old, const List<TreeNode> &cur_parsol, TreeNode &node);
    const TCoord sliptoDefectRight(const RectArea &area, const TID plate) const;
    const TCoord sliptoDefectUp(const RectArea &area, const TID plate) const;
    const bool defectConflictArea(const RectArea &area, const TID plate) const;
    const Length getLowBound(const TreeNode &cur_node, Area left_item_area) const;
    void toOutput(List<TreeNode> &sol);
    const TCoord getC1cpr(const List<TreeNode> &sol, const int index, const TID cur_plate, const TID cur_cut1) const;
    const TCoord getC2cpu(const List<TreeNode> &sol, const int index, const TID cur_cut1, const TID cur_cut2) const;
    bool check(Length &checkerObj) const;
    const double getScrapWasteRate(List<TreeNode>& sol) const;
    #pragma endregion Method

    #pragma region Field
public:
    Problem::Input input;
    Problem::Output output;

    struct {
        List<Rect> items;
        List<RectArea> defects;
        List<Area> item_area; // item area size of every item.
        List<TID> item2stack; //defect identity to stack identity

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
        double scrap_rate = 0.0;
        int generation_stamp = 0;
        String toStr() const {
            std::ostringstream os;
            os << "sr=" << scrap_rate
                << ";gs=" << generation_stamp;
            return os.str();
        }
    } info;

    Configuration cfg;
    Solver::Environment env;
    List<TreeNode> best_solution;
    Length best_objective = input.param.plateNum*input.param.plateWidth;
    double best_usage_rate = 1.0;
    Area total_item_area = 0;

    Random rand; // all random number in TSolver must be generated by this.
    Timer timer; // the solve() should return before it is timeout.
    int generation = 0; // the constructed complete solution number.
    int support_thread = 1;

    #pragma endregion Field
};

}

#endif // SMART_SZX_GUILLOTINE_CUT_TREESEARCH_H
