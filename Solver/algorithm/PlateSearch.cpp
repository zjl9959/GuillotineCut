#include "PlateSearch.h"
#include "Picker.h"
#include "../utility/ThreadPool.h"

using namespace std;

namespace szx{

/* �����������룺��Ʒջ */
void PlateSearch::beam_search(const Batch &source_batch) {
    Solution fix_sol;                           // �Ѿ��̶��Ľ�
    TCoord c1cpr = 0;
    Batch batch(source_batch);
    Solution best_cutsol, cutsol;               // ÿһ�ֵ�����1-cut�ͻ���1-cut
    while (!timer_.isTimeOut()) {               // ÿ�ι̶�һ��1-cut��ֱ��ԭ��ĩβ
        Area best_obj = 0;
        for (int i = 0; i < cfg_.mpbn; ++i) {   // ÿ�ֳַ������֧
            if (get_cutsol(1, c1cpr, batch, cutsol) < -1.0)
                continue;
            batch >> cutsol;
            Area obj = greedy_evaluate(cfg_.mcrn, batch, cutsol);   // ����
            batch << cutsol;
            if (best_obj < obj) {               // ��¼��õ�1-cut
                best_cutsol = cutsol;
                best_obj = obj;
            }
        }
        if (best_obj > 0) {                     // �̶���õ�1-cut
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

/* ����ĳһ�����ֽ�ĺû�
   ���룺repeat_num��̰����ǰ��ʱ�ظ�����CutSearch��Ŀ����batch����Ʒջ����sol���������Ľ⣩
   ������ÿ�ԭ���Ϸ��õ���Ʒ����� */
Area PlateSearch::greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol) {
    if (sol.empty()) return -1;
    TCoord c1cpr = sol.back().c1cpr;     // ��ǰ1-cut�ƽ����
    TCoord last_c1cpr = 0;               // ��¼��һ�ֵ�c1cprֵ�������Ż�ԭ��β��
    Batch batch(source_batch);
    Solution fix_sol(sol);              // fix_sol��̰�ĵĹ��죬ÿ�ι̶�һ��1-cut
    Solution cut_sol;                   // ����ÿ��̰��ǰ��ʱ1-cut�Ľ�
    // ̰�Ĺ���
    while (get_cutsol(repeat_num, c1cpr, batch, cut_sol) > -1.0) {
        last_c1cpr = c1cpr;
        c1cpr = cut_sol.back().c1cpr;
        fix_sol += cut_sol;
        batch >> cut_sol;
        if (timer_.isTimeOut())break;
    }
    Area res = item_area(fix_sol);
    update_bestsol(fix_sol, res);
    // �Ż����һ��1-cut
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

/* ����CutSearch��repeat_num�Σ���������õ�1-cut��
   ���룺repeat_num���ظ�����CutSearch��������start_pos��1-cut��ʼλ�ã���source_batch����Ʒջ����tail���Ż�ԭ��β����
   �����sol���õ�������1-cut�⣩�����أ���Ӧ���Ž��Ŀ�꺯��ֵ/-2.0
*/
Score PlateSearch::get_cutsol(int repeat_num, TCoord start_pos, const Batch &source_batch, Solution &sol, bool tail) {
    // ��ε���CutSearch��ѡһ����õĽ�
    CutSearch solver(plate_, start_pos, aux_);
    Picker picker(source_batch, rand_, aux_);
    Solution cut_sol;       // ����ÿ�ε���solver��õĽ�
    Score best_obj = -2.0;  // ��������1-cut���Ŀ�꺯��ֵ
    for (int i = 0; i < repeat_num; ++i) {
        if (timer_.isTimeOut())break;
        Batch sub_batch;    // ��batch����ѡ���Ӽ�
        Picker::Filter filter(aux_.param.plateWidth - start_pos);
        Picker::Terminator terminator(cfg_.mcin);
        if (!picker.rand_pick(sub_batch, terminator, filter))continue;
        cut_sol.clear();
        Score obj = solver.run(sub_batch, cut_sol, tail);
        if (best_obj < obj) {   // ����best_cut_sol;
            best_obj = obj;
            sol = cut_sol;
        }
    }
    return best_obj;
}

/* ����sol�е�������Ʒ���֮��
   ���룺sol�����ֽ⣩������������ */
Area PlateSearch::item_area(const Solution &sol) {
    Area total_area = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it)
        total_area += aux_.item_area[it->item];
    return total_area;
}

/* ���sol�Ƿ�����bestsol_�����������bestsol_ */
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