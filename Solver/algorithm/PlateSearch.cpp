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

/* ����ĳһ�����ֽ�ĺû�
   ���룺repeat_num��̰����ǰ��ʱ�ظ�����CutSearch��Ŀ����batch����Ʒջ����sol���������Ľ⣩
   ������ÿ�ԭ���Ϸ��õ���Ʒ����� */
Area PlateSearch::greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol) {
    if (sol.empty()) return -1;
    Coord c1cpr = sol.back().c1cpr;     // ��ǰ1-cut�ƽ����
    Coord last_c1cpr = 0;               // ��¼��һ�ֵ�c1cprֵ�������Ż�ԭ��β��
    Batch batch(source_batch);
    Solution cur_sol(sol);              // cur_sol��̰�ĵĹ���
    Solution cut_sol;                   // ����ÿ��̰��ǰ��ʱ1-cut�Ľ�
    // ̰�Ĺ���
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
    // �Ż����һ��1-cut
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

/* ����CutSearch��repeat_num�Σ���������õ�1-cut��
   ���룺repeat_num���ظ�����CutSearch��������start_pos��1-cut��ʼλ�ã���source_batch����Ʒջ����tail���Ż�ԭ��β����
   �����sol���õ�������1-cut�⣩�����أ���Ӧ���Ž��Ŀ�꺯��ֵ/-2.0
*/
Score PlateSearch::get_cutsol(int repeat_num, Coord start_pos, const Batch &source_batch, Solution &sol, bool tail) {
    // ��ε���CutSearch��ѡһ����õĽ�
    CutSearch solver(plate_, start_pos, aux_);
    Picker picker(source_batch, rand_, aux_);
    Solution cut_sol;       // ����ÿ�ε���solver��õĽ�
    Score best_obj = -2.0;  // ��������1-cut���Ŀ�꺯��ֵ
    for (int i = 0; i < repeat_num; ++i) {
        Batch sub_batch;    // ��batch����ѡ���Ӽ�
        if (!picker.rand_pick(cfg_.mcin, aux_.param.plateWidth - start_pos, sub_batch))
            continue;
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
    lock_guard<mutex> gurad(sol_mutex_);
    if (bestobj_ < obj) {
        bestobj_ = obj;
        bestsol_ = sol;
    }
}

}