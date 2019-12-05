#include "Solver/algorithm/header/TopSearch.h"

#include "Solver/algorithm/header/Picker.h"
#include "Solver/algorithm/header/PlateSearch.h"
#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/data/header/Global.h"

using namespace std;

namespace szx {

void TopSearch::run() {
    if (gv::cfg.top_mode == Configuration::TBEAM)
        beam_search();
    else if (gv::cfg.top_mode == Configuration::TLOCAL)
        local_search();
}

void TopSearch::beam_search() {
    Solution fix_sol;                   // 已经固定的解
    ID cur_plate = 0;                   // 当前原料id
    Batch batch(gv::stacks);           // 当前物品栈
    Solution best_platesol;             // 每一轮的最优plate解和缓存plate解
    List<Solution> branch_sols;         // 缓存分支出来的解
    while (!gv::timer.isTimeOut()) {
        Length best_obj = Problem::Output::MaxWidth;
        branch(cur_plate, batch, branch_sols, gv::cfg.mtbn);
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
            cout << "\r                     \r已完成：" <<
                (1.0 - static_cast<double>(batch.size()) / gv::items.size()) * 100.0 << "%";
        } else {
            break;
        }
    }
    cout << endl;
}

void TopSearch::local_search() {
    // [zjl][TODO]:add implement.
}

/*
* 功能：分支，不对batch进行挑选操作。
* 输入：plate_id（待分支原料id），source_batch（物品栈）。
* 输出：sols（分支出来的局部原料解）。
*/
void TopSearch::branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch) {
    if (gv::cfg.pick_item) {
        Picker picker(source_batch);
        Batch batch;
        sols.resize(nb_branch);
        Picker::Terminator terminator(0, static_cast<Area>(gv::param.plateHeight * gv::param.plateWidth * 2.5));
        size_t index = 0;
        for (size_t i = 0; i < nb_branch; ++i) {
            if (picker.rand_pick(batch, terminator)) {
                PlateSearch solver(plate_id, 1); // 只要一个最优解即可。
                solver.run(batch);
                if (solver.best_obj() > 0) {
                    solver.get_best_sol(sols[index++]);
                }
            }
        }
        sols.resize(index);
    } else {
        sols.clear();
        PlateSearch solver(plate_id, nb_branch);
        solver.run(source_batch);
        solver.get_good_sols(sols);
    }
}

/* 
* 贪心的走到底，并评估sol的好坏
* 输入：plate_id（原料id），source_batch（物品栈）
* 输出：sol（原料上的解）；返回：sol的评估值（原料使用长度）
*/
Length TopSearch::greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol) {
    if (fix_sol.empty()) return gv::param.plateWidth *gv::param.plateNum;
    Batch batch(source_batch);
    List<Solution> branch_sols;
    Solution cur_sol(fix_sol);
    ++plate_id;
    while (!gv::timer.isTimeOut() && batch.size() != 0) {
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
    Length obj = plate_id * gv::param.plateWidth + cur_sol.back().c1cpr;
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
    return plate_num* gv::param.plateWidth + sol.back().c1cpr;
}

/* 检查sol是否优于bestsol_，如是则更新bestsol_ */
void TopSearch::update_best_sol(const Solution &sol, Length obj) {
    if (sol.empty() || sol.size() != gv::items.size())
        return;
    if (obj < 0)
        obj = get_obj(sol);
    lock_guard<mutex> guard(sol_mutex_);
    if (best_obj_ > obj) {
        best_obj_ = obj;
        best_sol_ = sol;
    }
}

/*
* 寻找对当前解来说，第一个能够改进局部较差原料的提升动作。
* 输入：cur_sol（当前完整的解）。
* 输出：improve（一个局部的改进解），返回size_t（对cur_sol的改进起始位置）。
*/
size_t TopSearch::find_first_improvement(const Solution &cur_sol, Solution &improve) {
    static constexpr size_t max_k = 3;
    List<pair<size_t, UsageRate>> rates;
    get_plates_usage_rate(cur_sol, rates);
    for (size_t k = 1; k < max_k; ++k) {    // 邻域从小到大
        for (auto &rate : rates) {          // 利用率从低到高
            Batch batch(gv::stacks);
            for (size_t i = 0; i < rate.first; ++i) {
                batch.remove(cur_sol.at(i).item);
            }
            // [zjl][TODO]:add code.
        }
    }
    return size_t();
}

/*
* 计算输入的解中，每块原料的利用率。
* 输入：sol（一个完整的解，解中最后一块原料作为余料来处理）。
* 返回：usage_rates(该解中每块原料的<起始位置索引，原料利用率>对，按照利用率从小到大的顺序排列)。
*/
void TopSearch::get_plates_usage_rate(const Solution &sol, List<pair<size_t, UsageRate>> &usage_rates) {
    if (sol.empty()) return;
    const double plate_area = gv::param.plateHeight * gv::param.plateWidth;
    Area item_area = gv::item_area[sol[0].item];    // 原料上物品面积。
    size_t plate_index = 0;      // 原料起始位置在sol中的索引。
    for (size_t i = 1; i < sol.size(); ++i) {
        if (sol[i].getFlagBit(Placement::NEW_PLATE)) {
            usage_rates.push_back(make_pair(plate_index, UsageRate(item_area / plate_area)));
            plate_index = i;
            item_area = gv::item_area[sol[i].item];
        } else {
            item_area += gv::item_area[sol[i].item];
        }
    }
    // 计算余料利用率。
    const double tail_plate_area = gv::param.plateHeight * sol.back().c1cpr;
    usage_rates.push_back(make_pair(plate_index, UsageRate(item_area / tail_plate_area)));
    // 对各原料利用率进行排序。
    sort(usage_rates.begin(), usage_rates.end(),
        [](const pair<size_t, UsageRate> &lhs, const pair<size_t, UsageRate> &rhs) { return lhs.second < rhs.second; });
}

/*
* 对于当前解，寻找当前原料开始位置的前一块原料的开始位置索引。
* 输入：sol（要处理的解），cur_index（当前原料开始位置在解中的索引）。
* 输出：返回值（前一块原料的起始位置索引）。
*/
size_t TopSearch::prev_plate_index(const Solution &sol, size_t cur_index) {
    while (cur_index >= 0) {
        if (cur_index == 0)break;
        if (sol[cur_index].getFlagBit(Placement::NEW_PLATE))break;
        --cur_index;
    }
    return cur_index;
}

}
