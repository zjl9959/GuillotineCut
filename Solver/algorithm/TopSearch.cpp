#include "TopSearch.h"
#include "Picker.h"

using namespace std;

namespace szx {

void TopSearch::beam_search() {
    Solution fix_sol;                   // �Ѿ��̶��Ľ�
    ID cur_plate = 0;                   // ��ǰԭ��id
    Batch batch(aux_.stacks);           // ��ǰ��Ʒջ
    Solution best_platesol, platesol;   // ÿһ�ֵ�����plate��ͻ���plate��
    while (!timer_.isTimeOut()) {
        Length best_obj = Problem::Output::MaxWidth;
        for (int i = 0; i < cfg_.mtbn; ++i) {
            if (get_platesol(cur_plate, batch, platesol) < -1.0)
                continue;
            batch.remove(platesol);
            fix_sol += platesol;
            Length obj = greedy_evaluate(cur_plate, batch, fix_sol);
            batch.add(platesol);
            fix_sol -= platesol;
            if (best_obj > obj) {
                best_obj = obj;
                best_platesol = platesol;
            }
        }
        if (best_obj < Problem::Output::MaxWidth) {
            fix_sol += best_platesol;
            batch.remove(best_platesol);
            Log(Log::Debug) << "[TopSearch] fix plate:" << cur_plate
                << " add item num:" << best_platesol.size() << endl;
            cur_plate++;
        } else {
            break;
        }
    }
}

/* 
   �õ�һ��ԭ�ϵĽ�
   ���룺plate_id��ԭ��id����source_batch����Ʒջ��
   �����sol��ԭ���ϵĽ⣩�����أ�sol��Ŀ�꺯��ֵ��ԭ�������ʣ�
*/
Score TopSearch::get_platesol(ID plate_id, const Batch &source_batch, Solution &sol) {
    Picker picker(source_batch, rand_, aux_);
    Batch batch;
    Picker::Terminator terminator(0, static_cast<Area>(aux_.param.plateHeight * aux_.param.plateWidth * 1.2));
    if (picker.rand_pick(batch, terminator)) {
        PlateSearch solver(plate_id, cfg_, rand_, timer_, aux_);
        solver.beam_search(batch);
        return solver.get_bestsol(sol);  // [?] bug
    }
    return -2.0;
}

/* 
   ̰�ĵ��ߵ��ף�������sol�ĺû�
   ���룺plate_id��ԭ��id����source_batch����Ʒջ��
   �����sol��ԭ���ϵĽ⣩�����أ�sol������ֵ��ԭ��ʹ�ó��ȣ�
*/
Length TopSearch::greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &sol) {
    if (sol.empty()) return aux_.param.plateWidth *aux_.param.plateNum;
    Batch batch(source_batch);
    Solution fix_sol(sol);
    Solution plate_sol;
    ++plate_id;
    while (!timer_.isTimeOut() && batch.size() != 0) {
        if (get_platesol(plate_id, batch, plate_sol) > -1.0) {
            //Log(Log::Debug) << "\t[TopSearch : greedy] optimize plate:" << plate_id << endl;
            fix_sol += plate_sol;
            batch.remove(plate_sol);
            plate_id++;
        } else {
            return Problem::Output::MaxWidth;
        }
    }
    Length obj = plate_id * aux_.param.plateWidth + fix_sol.back().c1cpr;
    update_bestsol(fix_sol, obj);
    return obj;
}

/* ����Ŀ�꺯��ֵ����ʹ��ԭ�ϵ��ܳ��� */
Length TopSearch::get_obj(const Solution &sol) {
    if(sol.empty()) return Problem::Output::MaxWidth;
    ID plate_num = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it) {
        if (it->getFlagBit(Placement::NEW_PLATE))
            ++plate_num;
    }
    return plate_num*aux_.param.plateWidth + sol.back().c1cpr;
}

/* ���sol�Ƿ�����bestsol_�����������bestsol_ */
void TopSearch::update_bestsol(const Solution &sol, Length obj) {
    if (sol.empty() || sol.size() != aux_.items.size())
        return;
    if (obj < 0)
        obj = get_obj(sol);
    lock_guard<mutex> guard(sol_mutex_);
    if (best_obj_ > obj) {
        best_obj_ = obj;
        best_sol_ = sol;
        Log(Log::Debug) << "A BETTER SOLUTION!" << endl;
    }
}

Length TopSearch::get_bestsol(Solution &sol) {
    sol = best_sol_;
    return best_obj_;
}

}
