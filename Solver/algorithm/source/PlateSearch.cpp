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
* ����PlateSearch���������̣�ֱ��ʹ��CutSearch������ԭ�Ͻ����Ż���
*/
void PlateSearch::skip(const Batch &source_batch) {
    throw "not implement!";
}

/*
* �����������룺��Ʒջ��
*/
void PlateSearch::beam_search(const Batch &source_batch) {
    Solution fix_sol;                           // �Ѿ��̶��Ľ�
    TCoord c1cpr = 0;
    Batch batch(source_batch);
    Solution best_cutsol;               // ÿһ�ֵ�����1-cut�ͻ���1-cut
    List<Solution> branch_sols;                 // ����ÿ�ַ�֧�Ľ⡣
    while (!gv::timer.isTimeOut()) {               // ÿ�ι̶�һ��1-cut��ֱ��ԭ��ĩβ
        Area best_obj = 0;
        branch(c1cpr, batch, branch_sols, false, gv::cfg.mpbn);
        for (auto &sol : branch_sols) {   // ÿ�ֳַ������֧
            batch.remove(sol);
            fix_sol += sol;
            Area obj = greedy_evaluate(batch, fix_sol);   // ����
            batch.add(sol);
            fix_sol -= sol;
            if (best_obj < obj) {               // ��¼��õ�1-cut
                best_cutsol = sol;
                best_obj = obj;
            }
        }
        if (best_obj > 0) {                     // �̶���õ�1-cut
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
    Batch sub_batch;    // ��batch����ѡ���Ӽ���
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
* ����ĳһ�����ֽ�ĺû���
* ���룺repeat_num��̰����ǰ��ʱ�ظ�����CutSearch��Ŀ����batch����Ʒջ����sol���������Ľ⣩��
* ���������ֵ���ÿ�ԭ���Ϸ��õ���Ʒ����ͣ���
*/
Area PlateSearch::greedy_evaluate(const Batch &source_batch, const Solution &fix_sol) {
    if (fix_sol.empty()) return -1;
    TCoord c1cpr = fix_sol.back().c1cpr;        // ��ǰ1-cut�ƽ������
    TCoord last_c1cpr = 0;                      // ��¼��һ�ֵ�c1cprֵ�������Ż�ԭ��β����
    Solution last_cut_sol;                      // ��¼��һ��1-cut�Ľ⡣
    Batch batch(source_batch);                  // ��������һ��batch��
    Solution cur_sol(fix_sol);                  // fix_sol��̰�ĵĹ��죬ÿ�ι̶�һ��1-cut��
    List<Solution> branch_sols;                 // ����ÿ��̰��ǰ��ʱ1-cut�Ľ⡣
    // ̰�Ĺ��졣
    while (!gv::timer.isTimeOut()) {
        branch(c1cpr, batch, branch_sols, false, 1);
        if (!branch_sols.empty()) {
            last_c1cpr = c1cpr;
            last_cut_sol = branch_sols[0];
            c1cpr = branch_sols[0].back().c1cpr;
            cur_sol += branch_sols[0];
            batch.remove(branch_sols[0]);
        } else {
            break;  // �Ż���ԭ��β����
        }
    }
    Area res = item_area(cur_sol);
    update_best_sol(cur_sol, res);
    // �Ż����һ��1-cut��
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
* ����sol�е�������Ʒ���֮�͡�
* ���룺sol�����ֽ⣩��
* ���������ֵ������ͣ���
*/
Area PlateSearch::item_area(const Solution &sol) {
    Area total_area = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it)
        total_area += gv::item_area[it->item];
    return total_area;
}

/* 
* ���sol�Ƿ�����bestsol_�����������bestsol_��
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