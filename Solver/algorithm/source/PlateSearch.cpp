#include "Solver/algorithm/header/PlateSearch.h"

#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/algorithm/header/Picker.h"
#include "Solver/data/header/Global.h"

using namespace std;

namespace szx{

#pragma region interface

void PlateSearch::run(const Batch &source_batch) {
    if (gv::cfg.plate_mode == Configuration::PBEAM)
        beam_search(source_batch);
    else if (gv::cfg.plate_mode == Configuration::P0)
        skip(source_batch);
    //if (best_obj() > 0)
      //  gv::info.add_plate();
}

/*
* 跳过PlateSearch的搜索过程，直接使用CutSearch对整块原料进行优化。
*/
void PlateSearch::skip(const Batch &source_batch) {
    throw "not implement!";
}

/*
* 束搜索，输入：物品栈。
*/
void PlateSearch::beam_search(const Batch &source_batch) {
    Solution fix_sol;                           // 已经固定的解
    TCoord c1cpr = 0;
    Batch batch(source_batch);
    Solution best_cutsol;               // 每一轮的最优1-cut和缓存1-cut
    List<Solution> branch_sols;                 // 缓存每轮分支的解。
    while (!gv::timer.isTimeOut()) {               // 每次固定一个1-cut，直到原料末尾
        Area best_obj = 0;
        branch(c1cpr, batch, branch_sols, false, gv::cfg.mpbn);
        for (auto &sol : branch_sols) {   // 每轮分出多个分支
            batch.remove(sol);
            fix_sol += sol;
            Area obj = greedy_evaluate(batch, fix_sol);   // 评估
            batch.add(sol);
            fix_sol -= sol;
            if (best_obj < obj) {               // 记录最好的1-cut
                best_cutsol = sol;
                best_obj = obj;
            }
        }
        if (best_obj > 0) {                     // 固定最好的1-cut
            fix_sol += best_cutsol;
            batch.remove(best_cutsol);
            c1cpr = best_cutsol.back().c1cpr;
        } else {
            break;
        }
    }
}
#pragma endregion

void PlateSearch::branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, bool tail, size_t nb_branch) {
    sols.resize(nb_branch);
    CutSearch solver(plate_, start_pos, tail);
    Picker picker(source_batch);
    Batch sub_batch;    // 从batch中挑选的子集。
    size_t index = 0;
    for (size_t i = 0; i < nb_branch; ++i) {
        if (gv::timer.isTimeOut())break;
        Picker::Filter filter(gv::param.plateWidth - start_pos);
        Picker::Terminator terminator(12);
        if (!picker.rand_pick(sub_batch, terminator, filter))continue;
        solver.run(sub_batch);
        if (solver.best_obj().valid()) {
            solver.get_best_sol(sols[index++]);
        }
    }
    sols.resize(index);
}

/* 
* 评估某一个部分解的好坏。
* 输入：repeat_num（贪心向前看时重复调用CutSearch数目），batch（物品栈），sol（待评估的解）。
* 输出：返回值（该块原料上放置的物品面积和）。
*/
Area PlateSearch::greedy_evaluate(const Batch &source_batch, const Solution &fix_sol) {
    if (fix_sol.empty()) return -1;
    TCoord c1cpr = fix_sol.back().c1cpr;        // 当前1-cut推进情况。
    TCoord last_c1cpr = 0;                      // 记录上一轮的c1cpr值，用于优化原料尾部。
    Solution last_cut_sol;                      // 记录上一轮1-cut的解。
    Batch batch(source_batch);                  // 拷贝复制一份batch。
    Solution cur_sol(fix_sol);                  // fix_sol被贪心的构造，每次固定一个1-cut。
    List<Solution> branch_sols;                 // 储存每次贪心前解时1-cut的解。
    // 贪心构造。
    while (!gv::timer.isTimeOut()) {
        branch(c1cpr, batch, branch_sols, false, 1);
        if (!branch_sols.empty()) {
            last_c1cpr = c1cpr;
            last_cut_sol = branch_sols[0];
            c1cpr = branch_sols[0].back().c1cpr;
            cur_sol += branch_sols[0];
            batch.remove(branch_sols[0]);
        } else {
            break;  // 优化到原料尾部。
        }
    }
    Area res = item_area(cur_sol);
    update_best_sol(cur_sol, res);
    // 优化最后一个1-cut。
    if (!gv::timer.isTimeOut() && last_c1cpr > 0) {
        batch.add(last_cut_sol);
        cur_sol -= last_cut_sol;
        branch(last_c1cpr, batch, branch_sols, true, 1);
        if (!branch_sols.empty()) {
            cur_sol += branch_sols[0];
            Area new_res = item_area(cur_sol);
            if (new_res > res) {
                res = new_res;
                update_best_sol(cur_sol, new_res);
            }
        }
    }
    return res;
}

/* 
* 计算sol中的所有物品面积之和。
* 输入：sol（部分解）。
* 输出：返回值（面积和）。
*/
Area PlateSearch::item_area(const Solution &sol) {
    Area total_area = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it)
        total_area += gv::item_area[it->item];
    return total_area;
}

/* 
* 检查sol是否优于bestsol_，如是则更新bestsol_。
*/
void PlateSearch::update_best_sol(const Solution &sol, Area obj) {
    if(obj == 0)
        obj = item_area(sol);
    if (obj > best_obj_)
    {
        best_obj_ = obj;
        best_sol_ = sol;
    }
}

}