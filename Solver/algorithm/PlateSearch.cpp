#include "PlateSearch.h"
#include "../utility/ThreadPool.h"
#include "Picker.h"

using namespace std;

namespace szx{

Score PlateSearch::run(const Batch &batch, Solution &sol, bool fast_opt = false) {
    beam_search(batch, sol);
    return Score();
}

Score PlateSearch::beam_search(const Batch &source_batch, Solution &sol) {
    // [zjl][TODO]: add code.
}

/* 评估某一个部分解的好坏
   输入：repeat_num（贪心向前看时重复调用CutSearch数目），batch（物品栈），sol（待评估的解）
   输出：该块原料上放置的物品面积和 */
Area PlateSearch::greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol) {
    if (sol.empty()) return -1;
    Coord c1cpr = sol.back().c1cpr;     // 当前1-cut推进情况
    Coord last_c1cpr = 0;               // 记录上一轮的c1cpr值，用于优化原料尾部
    Batch batch(source_batch);
    Solution cur_sol(sol);              // cur_sol被贪心的构造
    Solution cut_sol;                   // 储存每次贪心前解时1-cut的解
    // 贪心构造
    while (get_cutsol(repeat_num, c1cpr, batch, cut_sol) > -1.0) {
        last_c1cpr = c1cpr;
        c1cpr = cut_sol.back().c1cpr;
        for (auto it = cut_sol.begin(); it != cut_sol.end(); ++it) {
            cur_sol.push_back(*it);
            batch.pop(it->item);
        }
    }
    Area res = item_area(cur_sol);
    update_bestsol(cur_sol, res);
    // 优化最后一个1-cut
    for (auto it = cut_sol.begin(); it != cut_sol.end(); ++it) {
        batch.push(it->item);
        cur_sol.pop_back();
    }
    if (get_cutsol(repeat_num, last_c1cpr, batch, cut_sol, true) > -1.0) {
        Area new_res = item_area(cut_sol);
        for (auto it = cur_sol.begin(); it != cur_sol.end(); ++it)
            cur_sol.push_back(*it);
        if (new_res > res) {
            res = new_res;
            update_bestsol(cur_sol, res);
        }
    }
    return res;
}

/* 调用CutSearch共repeat_num次，并返回最好的1-cut解
   输入：repeat_num（重复调用CutSearch次数），start_pos（1-cut开始位置），source_batch（物品栈），tail（优化原料尾部）
   输出：sol（得到的最优1-cut解），返回：对应最优解的目标函数值/-2.0
*/
Score PlateSearch::get_cutsol(int repeat_num, Coord start_pos, const Batch &source_batch, Solution &sol, bool tail) {
    // 多次调用CutSearch，选一个最好的解
    CutSearch solver(plate_, start_pos, aux_);
    Picker picker(source_batch, rand_, aux_);
    Solution cut_sol;       // 储存每次调用solver算得的解
    Score best_obj = -2.0;  // 本轮最优1-cut解的目标函数值
    for (int i = 0; i < repeat_num; ++i) {
        Batch sub_batch;    // 从batch中挑选的子集
        if (!picker.rand_pick(cfg_.mcin, aux_.param.plateWidth - start_pos, sub_batch))
            continue;
        cut_sol.clear();
        Score obj = solver.run(sub_batch, cut_sol, tail);
        if (best_obj < obj) {   // 更新best_cut_sol;
            best_obj = obj;
            sol = cut_sol;
        }
    }
    return best_obj;
}

/* 计算sol中的所有物品面积之和
   输入：sol（部分解）；输出：面积和 */
Area PlateSearch::item_area(const Solution &sol) {
    Area total_area = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it)
        total_area += aux_.item_area[it->item];
    return total_area;
}

/* 检查sol是否优于bestsol_，如是则更新bestsol_ */
void PlateSearch::update_bestsol(const Solution &sol, Area obj) {
    if(obj == 0)
        obj = item_area(sol);
    lock_guard<mutex> gurad(sol_mutex_);
    if (bestobj_ < obj) {
        bestobj_ = obj;
        bestsol_ = sol;
    }
}

}