#include "PlateSearch.h"
#include "Picker.h"
#include "../utility/ThreadPool.h"

using namespace std;

namespace szx{

/* 束搜索，输入：物品栈 */
void PlateSearch::beam_search(const Batch &source_batch) {
    Solution fix_sol;                           // 已经固定的解
    TCoord c1cpr = 0;
    Batch batch(source_batch);
    Solution best_cutsol, cutsol;               // 每一轮的最优1-cut和缓存1-cut
    while (!timer_.isTimeOut()) {               // 每次固定一个1-cut，直到原料末尾
        Area best_obj = 0;
        for (int i = 0; i < cfg_.mpbn; ++i) {   // 每轮分出多个分支
            if (get_cutsol(1, c1cpr, batch, cutsol) < -1.0)
                continue;
            batch >> cutsol;
            Area obj = greedy_evaluate(cfg_.mcrn, batch, cutsol);   // 评估
            batch << cutsol;
            if (best_obj < obj) {               // 记录最好的1-cut
                best_cutsol = cutsol;
                best_obj = obj;
            }
        }
        if (best_obj > 0) {                     // 固定最好的1-cut
            fix_sol += best_cutsol;
            batch >> best_cutsol;
            c1cpr = best_cutsol.back().c1cpr;
            //Log(Log::Debug) << "[PlateSearch] plate:" << plate_
            //    << " c1cpr:" << c1cpr << endl;
        } else {
            break;
        }
    }
}

/* 评估某一个部分解的好坏
   输入：repeat_num（贪心向前看时重复调用CutSearch数目），batch（物品栈），sol（待评估的解）
   输出：该块原料上放置的物品面积和 */
Area PlateSearch::greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol) {
    if (sol.empty()) return -1;
    TCoord c1cpr = sol.back().c1cpr;     // 当前1-cut推进情况
    TCoord last_c1cpr = 0;               // 记录上一轮的c1cpr值，用于优化原料尾部
    Batch batch(source_batch);
    Solution fix_sol(sol);              // fix_sol被贪心的构造，每次固定一个1-cut
    Solution cut_sol;                   // 储存每次贪心前解时1-cut的解
    // 贪心构造
    while (get_cutsol(repeat_num, c1cpr, batch, cut_sol) > -1.0) {
        last_c1cpr = c1cpr;
        c1cpr = cut_sol.back().c1cpr;
        fix_sol += cut_sol;
        batch >> cut_sol;
        if (timer_.isTimeOut())break;
    }
    Area res = item_area(fix_sol);
    update_bestsol(fix_sol, res);
    // 优化最后一个1-cut
    batch << cut_sol;
    fix_sol.erase(fix_sol.begin() + (fix_sol.size() - cut_sol.size()), fix_sol.end());
    if (get_cutsol(repeat_num, last_c1cpr, batch, cut_sol, true) > -1.0) {
        Area new_res = item_area(cut_sol);
        fix_sol += cut_sol;
        if (new_res > res) {
            res = new_res;
            update_bestsol(fix_sol, new_res);
        }
    }
    return res;
}

/* 调用CutSearch共repeat_num次，并返回最好的1-cut解
   输入：repeat_num（重复调用CutSearch次数），start_pos（1-cut开始位置），source_batch（物品栈），tail（优化原料尾部）
   输出：sol（得到的最优1-cut解），返回：对应最优解的目标函数值/-2.0
*/
Score PlateSearch::get_cutsol(int repeat_num, TCoord start_pos, const Batch &source_batch, Solution &sol, bool tail) {
    // 多次调用CutSearch，选一个最好的解
    CutSearch solver(plate_, start_pos, aux_);
    Picker picker(source_batch, rand_, aux_);
    Solution cut_sol;       // 储存每次调用solver算得的解
    Score best_obj = -2.0;  // 本轮最优1-cut解的目标函数值
    for (int i = 0; i < repeat_num; ++i) {
        if (timer_.isTimeOut())break;
        Batch sub_batch;    // 从batch中挑选的子集
        Picker::Filter filter(aux_.param.plateWidth - start_pos);
        Picker::Terminator terminator(cfg_.mcin);
        if (!picker.rand_pick(sub_batch, terminator, filter))continue;
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
    lock_guard<mutex> guard(sol_mutex_);
    if (bestobj_ < obj) {
        bestobj_ = obj;
        bestsol_ = sol;
    }
}

Area PlateSearch::get_bestsol(Solution &sol) {
    sol = bestsol_;
    return bestobj_;
}

}