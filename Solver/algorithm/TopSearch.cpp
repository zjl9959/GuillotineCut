#include "TopSearch.h"
#include "Picker.h"

using namespace std;

namespace szx {

//#define TOP_BRANCH_USE_PICKER // 该宏用于控制分支时是否挑选物品。

void TopSearch::beam_search() {
    Solution fix_sol;                   // 已经固定的解
    ID cur_plate = 0;                   // 当前原料id
    Batch batch(aux_.stacks);           // 当前物品栈
    Solution best_platesol;             // 每一轮的最优plate解和缓存plate解
    List<Solution> branch_sols;         // 缓存分支出来的解
    while (!timer_.isTimeOut()) {
        Length best_obj = Problem::Output::MaxWidth;
        branch_sols.clear();
        branch(cur_plate, batch, branch_sols, cfg_.mtbn);
        for (auto &psol : branch_sols) {
            batch.remove(psol);
            fix_sol += psol;
            Length obj = greedy_evaluate(cur_plate, batch, fix_sol);
            batch.add(psol);
            fix_sol -= psol;
            if (best_obj > obj) {
                best_obj = obj;
                best_platesol = psol;
            }
        }
        if (best_obj < Problem::Output::MaxWidth) {
            fix_sol += best_platesol;
            batch.remove(best_platesol);
            cur_plate++;
        } else {
            break;
        }
    }
}

/*
* 功能：分支，不对batch进行挑选操作。
* 输入：plate_id（待分支原料id），source_batch（物品栈）。
* 输出：sols（分支出来的局部原料解）。
*/
void TopSearch::branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch) {
    #ifdef TOP_BRANCH_USE_PICKER
    Picker picker(source_batch, rand_, aux_);
    Batch batch;
    sols.resize(nb_branch);
    Picker::Terminator terminator(0, static_cast<Area>(aux_.param.plateHeight * aux_.param.plateWidth * 2.5));
    size_t index = 0;
    for (size_t i = 0; i < nb_branch; ++i) {
        if (picker.rand_pick(batch, terminator)) {
            PlateSearch solver(plate_id, 1, cfg_, rand_, timer_, aux_); // 只要一个最优解即可。
            solver.beam_search(batch);
            if (solver.best_obj() > 0) {
                solver.get_best_sol(sols[index++]);
            }
        }
    }
    sols.resize(index);
    #else
    PlateSearch solver(plate_id, nb_branch, cfg_, rand_, timer_, aux_);
    solver.beam_search(source_batch);
    solver.get_good_sols(sols);
    #endif // BRANCH_PICKER
}

/* 
* 贪心的走到底，并评估sol的好坏
* 输入：plate_id（原料id），source_batch（物品栈）
* 输出：sol（原料上的解）；返回：sol的评估值（原料使用长度）
*/
Length TopSearch::greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol) {
    if (fix_sol.empty()) return aux_.param.plateWidth *aux_.param.plateNum;
    Batch batch(source_batch);
    List<Solution> branch_sols;
    Solution cur_sol(fix_sol);
    ++plate_id;
    while (!timer_.isTimeOut() && batch.size() != 0) {
        branch_sols.clear();
        branch(plate_id, batch, branch_sols);
        if (!branch_sols.empty()) {
            cur_sol += branch_sols[0];
            batch.remove(branch_sols[0]);
            plate_id++;
            assert(nb_used_plate(cur_sol) == plate_id);
        } else {
            return Problem::Output::MaxWidth;
        }
    }
    Length obj = plate_id * aux_.param.plateWidth + cur_sol.back().c1cpr;
    update_best_sol(cur_sol, obj);
    return obj;
}

/* 返回目标函数值，已使用原料的总长度 */
Length TopSearch::get_obj(const Solution &sol) {
    if(sol.empty()) return Problem::Output::MaxWidth;
    ID plate_num = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it) {
        if (it->getFlagBit(Placement::NEW_PLATE))
            ++plate_num;
    }
    return plate_num*aux_.param.plateWidth + sol.back().c1cpr;
}

/* 检查sol是否优于bestsol_，如是则更新bestsol_ */
void TopSearch::update_best_sol(const Solution &sol, Length obj) {
    if (sol.empty() || sol.size() != aux_.items.size())
        return;
    if (obj < 0)
        obj = get_obj(sol);
    lock_guard<mutex> guard(sol_mutex_);
    if (best_obj_ > obj) {
        best_obj_ = obj;
        best_sol_ = sol;
    }
}

}
