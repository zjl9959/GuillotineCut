#include "Solver/algorithm/header/PlateSearch.h"

#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/algorithm/header/Picker.h"
#include "Solver/data/header/Global.h"

using namespace std;

namespace szx{

#define PLATE_BRANCH_USE_PICKER   // 该宏用于控制分支时是否挑选物品。

#pragma region interface
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

void PlateSearch::get_best_sol(Solution & sol) const {
    if (!sol_cache_.empty())
        sol = sol_cache_.begin()->second;
}

void PlateSearch::get_good_sols(List<Solution>& sols) const {
    for (auto it = sol_cache_.begin(); it != sol_cache_.end(); ++it) {
        sols.push_back(it->second);
    }
}

Area PlateSearch::best_obj() const {
    return sol_cache_.empty() ? -1 : sol_cache_.begin()->first;
}

void PlateSearch::get_good_objs(List<Area>& objs) const {
    for (auto it = sol_cache_.begin(); it != sol_cache_.end(); ++it) {
        objs.push_back(it->first);
    }
}
#pragma endregion

void PlateSearch::branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, bool opt_tail, size_t nb_branch) {
    CutSearch::Setting set(opt_tail);
    #ifdef PLATE_BRANCH_USE_PICKER
    sols.resize(nb_branch);
    CutSearch solver(plate_, start_pos, 1, set);
    Picker picker(source_batch);
    size_t index = 0;
    for (size_t i = 0; i < nb_branch; ++i) {
        if (gv::timer.isTimeOut())break;
        Batch sub_batch(source_batch);    // 从batch中挑选的子集
        Picker::Filter filter(gv::param.plateWidth - start_pos);
        Picker::Terminator terminator(gv::cfg.mppn);
        if (!picker.rand_pick(sub_batch, terminator, filter))continue;
        solver.run(sub_batch);
        if (solver.best_obj().valid()) {
            solver.get_best_sol(sols[index++]);
        }
    }
    sols.resize(index);
    #else
    CutSearch solver(plate_, start_pos, nb_branch, aux_, set);
    Batch batch(source_batch);
    solver.run(batch);
    sols.clear();
    solver.get_good_sols(sols);
    #endif // PLATE_BRANCH_USE_PICKER
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
    update_sol_cache(cur_sol, res);
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
                update_sol_cache(cur_sol, new_res);
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
void PlateSearch::update_sol_cache(const Solution &sol, Area obj) {
    if(obj == 0)
        obj = item_area(sol);
    lock_guard<mutex> guard(sol_mutex_);
    assert(valid_plate_sol(sol));
    if (sol_cache_.size() < nb_sol_cache_) {
        sol_cache_.insert(make_pair(obj, sol));
    } else if (sol_cache_.rbegin()->first < obj) {
        sol_cache_.insert(make_pair(obj, sol));
        if(sol_cache_.size() > nb_sol_cache_)
            sol_cache_.erase(prev(sol_cache_.end()));
    }
}

}