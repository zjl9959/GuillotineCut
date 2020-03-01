#include "Solver/algorithm/header/PlateSearch.h"

#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/algorithm/header/Picker.h"
#include "Solver/data/header/Global.h"

using namespace std;

namespace szx{

#pragma region interface

void PlateSearch::run(const Batch &source_batch)
{
    beam_search(source_batch);
    if (best_obj_ > 0)
    {
        gv::info.nb_plate_sol += 1;
    }
}

/*
* 束搜索，输入：物品栈。
*/
void PlateSearch::beam_search(const Batch &source_batch)
{
    Solution fix_sol;                           // 已经固定的解
    TCoord c1cpr = 0;                           // 当前最右侧1-cut坐标
    Batch batch(source_batch);                  // 当前可用物品栈
    Solution best_cutsol;                       // 每一轮的最优1-cut和缓存1-cut
    List<Solution> branch_sols;                 // 缓存每轮分支的解。
    
    // 每次固定一个1-cut，直到原料末尾
    while (!gv::timer.isTimeOut())
    {
        Area best_obj = 0;
        branch(c1cpr, batch, branch_sols, 0, gv::cfg.mpbn);
        for (auto &sol : branch_sols)
        {
            batch.remove(sol);
            fix_sol += sol;
            Area obj = greedy_evaluate(fix_sol.back().c1cpr, batch, fix_sol);
            batch.add(sol);
            fix_sol -= sol;
            // 记录最好的1-cut
            if (best_obj < obj)
            {
                best_cutsol = sol;
                best_obj = obj;
            }
        }
        // 固定最好的1-cut
        if (best_obj > 0)
        {
            fix_sol += best_cutsol;
            batch.remove(best_cutsol);
            c1cpr = best_cutsol.back().c1cpr;
        } else
        {
            break;
        }
    }
}
#pragma endregion

void PlateSearch::branch(TCoord start_pos, const Batch &source_batch, List<Solution> &sols, TCoord end_pos, size_t nb_branch)
{
    Picker picker(source_batch);
    Batch sub_batch;                    // 从batch中挑选的子集。
    size_t index = 0;                   // 记录sols中下一个解放的位置。
    sols.resize(nb_branch);             // sols的大小最后还会调整。
    CutSearch solver(plate_, start_pos, end_pos);
    for (size_t i = 0; i < nb_branch; ++i)
    {
        if (gv::timer.isTimeOut())break;
        Picker::Filter filter(gv::param.plateWidth - start_pos);
        Picker::Terminator terminator(gv::cfg.mcbn);
        if (!picker.rand_pick(sub_batch, terminator, filter))continue;
        solver.run(sub_batch);
        if (solver.best_obj().valid())
        {
            solver.get_best_sol(sols[index++]);
        }
    }
    sols.resize(index);
}

Area PlateSearch::greedy_evaluate(
    TCoord start_pos,
    const Batch &source_batch,
    const Solution &fix_sol
) {
    TCoord c1cpr = start_pos;                   // 当前1-cut推进情况。
    TCoord last_c1cpr = 0;                      // 记录上一轮的c1cpr值，用于优化原料尾部。
    Solution last_cut_sol;                      // 记录上一轮1-cut的解。
    Batch batch(source_batch);                  // 拷贝复制一份batch。
    Solution cur_sol(fix_sol);                  // fix_sol被贪心的构造，每次固定一个1-cut。
    List<Solution> branch_sols;                 // 储存每次贪心前解时1-cut的解。
    // 贪心构造。
    while (!gv::timer.isTimeOut())
    {
        branch(c1cpr, batch, branch_sols, 0, 1);
        if (!branch_sols.empty())
        {
            last_c1cpr = c1cpr;
            last_cut_sol = branch_sols[0];
            c1cpr = branch_sols[0].back().c1cpr;
            cur_sol += branch_sols[0];
            batch.remove(branch_sols[0]);
        } else
        {
            break;  // 优化到原料尾部。
        }
    }
    Area res = item_area(cur_sol);
    update_best_sol(cur_sol, res);
    // 优化最后一个1-cut。
    if (!gv::timer.isTimeOut() && last_c1cpr > 0)
    {
        batch.add(last_cut_sol);
        cur_sol -= last_cut_sol;
        branch(last_c1cpr, batch, branch_sols, gv::param.plateWidth, 1);
        if (!branch_sols.empty())
        {
            cur_sol += branch_sols[0];
            Area new_res = item_area(cur_sol);
            if (new_res > res)
            {
                res = new_res;
                update_best_sol(cur_sol, new_res);
                gv::info.nb_reoptimize_improve += 1;    // 统计信息。
            }
        }
    }
    return res;
}

// 这种贪心方法更慢一些，但是会考虑前后两个1-cut间的关联性。
Area PlateSearch::greedy_serial_construct(
    TCoord start_pos,
    const Batch &source_batch,
    const Solution &fix_sol
) {
    TCoord c1cpr = start_pos;                   // 当前1-cut推进情况。
    Batch batch(source_batch);                  // 拷贝复制一份batch。
    Solution cur_sol(fix_sol);                  // 记录当前原料层部分解。
    List<Solution> branch_sols;                 // 缓存1-cut解。
    Area temp_obj = 0;                          // 临时记录原料填充面积。
    bool exit_flag = false;
    
    while (!gv::timer.isTimeOut() && !exit_flag)
    {
        // 首先尽量增大1-cut内部的利用率，而不考虑其宽度。
        branch(c1cpr, batch, branch_sols, 0, 1);
        if (!branch_sols.empty())
        {
            c1cpr = branch_sols[0].back().c1cpr;
            cur_sol += branch_sols[0];
            batch.remove(branch_sols[0]);
        } else
        {
            if(cur_sol.empty())
            {
                // 这种情况很少发生，但是以防万一。
                break;
            }
            temp_obj = item_area(cur_sol);
            update_best_sol(cur_sol, temp_obj);
            exit_flag = true;
        }
        // 然后再次优化当前和上一个1-cut，将宽度固定。
        if (fix_sol.empty() || cur_sol[0].c1cpl == c1cpr)
        {
            // 当前解中只有一个一层桶时，跳过下面的步骤。
            continue;
        }
        Solution last_2bin_sol;
        get_last_n_1cut(cur_sol, last_2bin_sol, 2);
        batch.add(last_2bin_sol);
        branch(last_2bin_sol[0].c1cpl, batch, branch_sols, cur_sol.back().c1cpr, 1);
        if (!branch_sols.empty())
        {
            if (branch_sols[0].back().c1cpr < cur_sol.back().c1cpr)
            {
                cur_sol -= last_2bin_sol;
                cur_sol += branch_sols[0];
                batch.remove(branch_sols[0]);
                c1cpr = cur_sol.back().c1cpr;
                gv::info.nb_reoptimize_improve += 1;    // 统计信息。
                if (exit_flag)
                {
                    Area obj = item_area(cur_sol);
                    if (obj > temp_obj)
                    {
                        temp_obj = obj;
                        update_best_sol(cur_sol, obj);
                    }
                }
            } else
            {
                batch.remove(last_2bin_sol);
            }
        }
    }
    return temp_obj;
}

/* 
* 计算sol中的所有物品面积之和。
* 输入：sol（部分解）。
* 输出：返回值（面积和）。
*/
Area PlateSearch::item_area(const Solution &sol)
{
    Area total_area = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it)
    {
        total_area += gv::item_area[it->item];
    }
    return total_area;
}

/* 
* 检查sol是否优于bestsol_，如是则更新bestsol_。
*/
void PlateSearch::update_best_sol(const Solution &sol, Area obj)
{
    if (obj == 0)
    {
        obj = item_area(sol);
    }
    if (obj > best_obj_)
    {
        best_obj_ = obj;
        best_sol_ = sol;
    }
}

int PlateSearch::get_last_n_1cut(const Solution& sol, Solution& last_n_1cut, int n) {
    auto& cur_sol_it = sol.rbegin();
    int nb_new_L1 = 0;          // 出现两个NEW_L1标志后则停止回溯。
    while (nb_new_L1 < n && cur_sol_it != sol.rend())
    {
        last_n_1cut.push_back(*cur_sol_it);
        if (cur_sol_it->getFlagBit(Placement::NEW_L1))
        {
            nb_new_L1 += 1;
        }
        cur_sol_it++;
    }
    std::reverse(last_n_1cut.begin(), last_n_1cut.end());
    return nb_new_L1;
}

}
