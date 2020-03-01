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
* �����������룺��Ʒջ��
*/
void PlateSearch::beam_search(const Batch &source_batch)
{
    Solution fix_sol;                           // �Ѿ��̶��Ľ�
    TCoord c1cpr = 0;                           // ��ǰ���Ҳ�1-cut����
    Batch batch(source_batch);                  // ��ǰ������Ʒջ
    Solution best_cutsol;                       // ÿһ�ֵ�����1-cut�ͻ���1-cut
    List<Solution> branch_sols;                 // ����ÿ�ַ�֧�Ľ⡣
    
    // ÿ�ι̶�һ��1-cut��ֱ��ԭ��ĩβ
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
            // ��¼��õ�1-cut
            if (best_obj < obj)
            {
                best_cutsol = sol;
                best_obj = obj;
            }
        }
        // �̶���õ�1-cut
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
    Batch sub_batch;                    // ��batch����ѡ���Ӽ���
    size_t index = 0;                   // ��¼sols����һ����ŵ�λ�á�
    sols.resize(nb_branch);             // sols�Ĵ�С��󻹻������
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
    TCoord c1cpr = start_pos;                   // ��ǰ1-cut�ƽ������
    TCoord last_c1cpr = 0;                      // ��¼��һ�ֵ�c1cprֵ�������Ż�ԭ��β����
    Solution last_cut_sol;                      // ��¼��һ��1-cut�Ľ⡣
    Batch batch(source_batch);                  // ��������һ��batch��
    Solution cur_sol(fix_sol);                  // fix_sol��̰�ĵĹ��죬ÿ�ι̶�һ��1-cut��
    List<Solution> branch_sols;                 // ����ÿ��̰��ǰ��ʱ1-cut�Ľ⡣
    // ̰�Ĺ��졣
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
            break;  // �Ż���ԭ��β����
        }
    }
    Area res = item_area(cur_sol);
    update_best_sol(cur_sol, res);
    // �Ż����һ��1-cut��
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
                gv::info.nb_reoptimize_improve += 1;    // ͳ����Ϣ��
            }
        }
    }
    return res;
}

// ����̰�ķ�������һЩ�����ǻῼ��ǰ������1-cut��Ĺ����ԡ�
Area PlateSearch::greedy_serial_construct(
    TCoord start_pos,
    const Batch &source_batch,
    const Solution &fix_sol
) {
    TCoord c1cpr = start_pos;                   // ��ǰ1-cut�ƽ������
    Batch batch(source_batch);                  // ��������һ��batch��
    Solution cur_sol(fix_sol);                  // ��¼��ǰԭ�ϲ㲿�ֽ⡣
    List<Solution> branch_sols;                 // ����1-cut�⡣
    Area temp_obj = 0;                          // ��ʱ��¼ԭ����������
    bool exit_flag = false;
    
    while (!gv::timer.isTimeOut() && !exit_flag)
    {
        // ���Ⱦ�������1-cut�ڲ��������ʣ������������ȡ�
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
                // ����������ٷ����������Է���һ��
                break;
            }
            temp_obj = item_area(cur_sol);
            update_best_sol(cur_sol, temp_obj);
            exit_flag = true;
        }
        // Ȼ���ٴ��Ż���ǰ����һ��1-cut������ȹ̶���
        if (fix_sol.empty() || cur_sol[0].c1cpl == c1cpr)
        {
            // ��ǰ����ֻ��һ��һ��Ͱʱ����������Ĳ��衣
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
                gv::info.nb_reoptimize_improve += 1;    // ͳ����Ϣ��
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
* ����sol�е�������Ʒ���֮�͡�
* ���룺sol�����ֽ⣩��
* ���������ֵ������ͣ���
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
* ���sol�Ƿ�����bestsol_�����������bestsol_��
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
    int nb_new_L1 = 0;          // ��������NEW_L1��־����ֹͣ���ݡ�
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
