#include "Solver/algorithm/header/TopSearch.h"

#include <numeric>

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
        } else {
            break;
        }
    }
    cout << endl;
}

void TopSearch::local_search() {
    Solution s0;    // 初始解
    generate_init_sol(s0);
    update_best_sol(s0);
    int iter = 0;   // 迭代步数
    while (!gv::timer.isTimeOut()) {
        if (find_first_improvement(s0) == false) {
            // 如果找不到可以提升的动作，则进行随机扰动。
            // [zjl][TODO]:该如何扰动？
        }
        assert(s0.size() == gv::items.size());
        update_best_sol(s0);
        ++iter;
    }
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
* 功能：贪心的走到底，并评估sol的好坏
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
    Length obj = (plate_id - 1) * gv::param.plateWidth + cur_sol.back().c1cpr;
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
    return (plate_num - 1)* gv::param.plateWidth + sol.back().c1cpr;
}

/* 检查sol是否优于bestsol_，如是则更新bestsol_ */
void TopSearch::update_best_sol(const Solution &sol, Length obj) {
    if (sol.empty() || sol.size() != gv::items.size())
        return;
    if (obj < 0)
        obj = get_obj(sol);
    lock_guard<mutex> guard(sol_mutex_);
    if (best_obj_ > obj) {
        Log(Log::Debug) << "=> " << total_item_area_ / (double)(obj * gv::param.plateHeight) * 100 << "%" << endl;
        best_obj_ = obj;
        best_sol_ = sol;
    }
}

/*
* 功能：构造一个初始解（贪心的策略）。
* 输出：构造的解。
*/
void TopSearch::generate_init_sol(Solution &sol) {
    Batch b(gv::stacks);
    TID plate = 0;
    while (b.size() != 0) {
        PlateSearch solver(plate, 1);
        solver.run(b);
        if (solver.best_obj() > 0) {
            Solution temp_sol;
            solver.get_best_sol(temp_sol);
            sol += temp_sol;
            b.remove(temp_sol);
            ++plate;
        } else {
            throw "Error in generate_init_sol.";
        }
    }
}

/*
* 功能：寻找对当前解来说，第一个能够改进局部较差原料的提升动作。
* 输入：cur_sol（当前完整的解）。
* 输出：cur_sol（新的解），返回值（true:改进，false:未改进）。
*/
bool TopSearch::find_first_improvement(Solution &cur_sol) {
    static constexpr size_t max_k = 3;          // 最多回溯前k块原料。

    List<int> plates_index = get_plates_index(cur_sol);
    List<double> usage_rates = get_plates_usage_rate(cur_sol);
    double average = accumulate(usage_rates.begin(), usage_rates.end(), 0.0) / plates_index.size();
    List<int> bad_plates;                       // 利用率低于平均值的原料列表。
    for (int i = 0; i < plates_index.size(); ++i) {
        if (usage_rates[i] < average)
            bad_plates.push_back(i);
    }
    // 因为是first_improvement，通过随机打乱原料id顺序来提高算法的疏散性。
    for (int nb_rand_suff = 0; nb_rand_suff < bad_plates.size(); ++nb_rand_suff) {
        int temp = gv::rand.pick(bad_plates.size());
        swap(bad_plates.begin() + temp, bad_plates.begin() + bad_plates.size() - 1);
    }
    for (int plate : bad_plates) {              // 对于每一块较差的原料。
        for (size_t k = 1; k <= max_k; ++k) {   // 邻域从小到大。
            Batch batch(gv::stacks);
            if (plate < k) break;               // 前面没有这么多原料可供回溯了。
            for (int i = 0; i < plates_index[plate - k]; ++i) {
                batch.remove(cur_sol.at(i).item);
            }
            // 重新构造的局部解。
            Solution partial_sol;
            double total_partial_usage_rate = 0.0;  // 重新构造解的利用率统计和。
            double total_bad_usage_rate = 0.0;      // 老的局部解的利用率统计和。
            for (int j = plate - k; j <= plate; ++j) {
                total_bad_usage_rate += usage_rates[j];
                PlateSearch solver(j, 1);
                solver.run(batch);
                if (solver.best_obj() > 0) {
                    Solution temp_sol;
                    solver.get_best_sol(temp_sol);
                    partial_sol += temp_sol;
                    batch.remove(temp_sol);
                    total_partial_usage_rate += (double)solver.best_obj() / (gv::param.plateWidth * gv::param.plateHeight);
                } else {
                    return false;
                }
            }
            // 检查partial_sol是否优于老的解。
            if (total_partial_usage_rate > total_bad_usage_rate) {
                cur_sol.resize(plates_index[plate - k]);
                cur_sol += partial_sol;
                while (batch.size() != 0) {     // 构造后续完整的解。
                    PlateSearch solver(plate, 1);
                    solver.run(batch);
                    if (solver.best_obj() > 0) {
                        Solution temp_sol;
                        solver.get_best_sol(temp_sol);
                        cur_sol += temp_sol;
                        batch.remove(temp_sol);
                        ++plate;
                    } else {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

/*
* 功能：计算输入的解中，每块原料的利用率。
* 输入：sol（一个完整的解，解中最后一块原料作为余料来处理）。
* 输出：返回值（每块原料的利用率）。
*/
List<double> TopSearch::get_plates_usage_rate(const Solution &sol) {
    List<double> res;
    if (sol.empty()) return res;
    const double plate_area = gv::param.plateHeight * gv::param.plateWidth;
    Area item_area = gv::item_area[sol[0].item];    // 原料上物品面积。
    size_t plate_index = 0;                         // 原料起始位置在sol中的索引。
    for (size_t i = 1; i < sol.size(); ++i) {
        if (sol[i].getFlagBit(Placement::NEW_PLATE)) {
            res.push_back(item_area / plate_area);
            plate_index = i;
            item_area = gv::item_area[sol[i].item];
        } else {
            item_area += gv::item_area[sol[i].item];
        }
    }
    // 计算余料利用率。
    const double tail_plate_area = gv::param.plateHeight * sol.back().c1cpr;
    res.push_back(item_area / tail_plate_area);
    return res;
}

/*
* 功能：构建当前解的每块原料id对于在解中开始位置索引。
* 输入：sol（要处理的解）。
* 输出：返回值（索引列表）。
*/
List<int> TopSearch::get_plates_index(const Solution &sol) {
    List<int> res;
    for (int i = 0; i < sol.size(); ++i) {
        if (sol[i].getFlagBit(Placement::NEW_PLATE))
            res.push_back(i);
    }
    return res;
}

}
