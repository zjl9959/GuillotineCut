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
    Solution fix_sol;                   // �Ѿ��̶��Ľ�
    ID cur_plate = 0;                   // ��ǰԭ��id
    Batch batch(gv::stacks);           // ��ǰ��Ʒջ
    Solution best_platesol;             // ÿһ�ֵ�����plate��ͻ���plate��
    List<Solution> branch_sols;         // �����֧�����Ľ�
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
            cout << "\r                     \r����ɣ�" <<
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
* ���ܣ���֧������batch������ѡ������
* ���룺plate_id������֧ԭ��id����source_batch����Ʒջ����
* �����sols����֧�����ľֲ�ԭ�Ͻ⣩��
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
                PlateSearch solver(plate_id, 1); // ֻҪһ�����Ž⼴�ɡ�
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
* ̰�ĵ��ߵ��ף�������sol�ĺû�
* ���룺plate_id��ԭ��id����source_batch����Ʒջ��
* �����sol��ԭ���ϵĽ⣩�����أ�sol������ֵ��ԭ��ʹ�ó��ȣ�
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

/* ����Ŀ�꺯��ֵ����ʹ��ԭ�ϵ��ܳ��� */
Length TopSearch::get_obj(const Solution &sol) {
    if(sol.empty()) return Problem::Output::MaxWidth;
    ID plate_num = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it) {
        if (it->getFlagBit(Placement::NEW_PLATE))
            ++plate_num;
    }
    return plate_num* gv::param.plateWidth + sol.back().c1cpr;
}

/* ���sol�Ƿ�����bestsol_�����������bestsol_ */
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
* Ѱ�ҶԵ�ǰ����˵����һ���ܹ��Ľ��ֲ��ϲ�ԭ�ϵ�����������
* ���룺cur_sol����ǰ�����Ľ⣩��
* �����improve��һ���ֲ��ĸĽ��⣩������size_t����cur_sol�ĸĽ���ʼλ�ã���
*/
size_t TopSearch::find_first_improvement(const Solution &cur_sol, Solution &improve) {
    static constexpr size_t max_k = 3;
    List<pair<size_t, UsageRate>> rates;
    get_plates_usage_rate(cur_sol, rates);
    for (size_t k = 1; k < max_k; ++k) {    // �����С����
        for (auto &rate : rates) {          // �����ʴӵ͵���
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
* ��������Ľ��У�ÿ��ԭ�ϵ������ʡ�
* ���룺sol��һ�������Ľ⣬�������һ��ԭ����Ϊ������������
* ���أ�usage_rates(�ý���ÿ��ԭ�ϵ�<��ʼλ��������ԭ��������>�ԣ����������ʴ�С�����˳������)��
*/
void TopSearch::get_plates_usage_rate(const Solution &sol, List<pair<size_t, UsageRate>> &usage_rates) {
    if (sol.empty()) return;
    const double plate_area = gv::param.plateHeight * gv::param.plateWidth;
    Area item_area = gv::item_area[sol[0].item];    // ԭ������Ʒ�����
    size_t plate_index = 0;      // ԭ����ʼλ����sol�е�������
    for (size_t i = 1; i < sol.size(); ++i) {
        if (sol[i].getFlagBit(Placement::NEW_PLATE)) {
            usage_rates.push_back(make_pair(plate_index, UsageRate(item_area / plate_area)));
            plate_index = i;
            item_area = gv::item_area[sol[i].item];
        } else {
            item_area += gv::item_area[sol[i].item];
        }
    }
    // �������������ʡ�
    const double tail_plate_area = gv::param.plateHeight * sol.back().c1cpr;
    usage_rates.push_back(make_pair(plate_index, UsageRate(item_area / tail_plate_area)));
    // �Ը�ԭ�������ʽ�������
    sort(usage_rates.begin(), usage_rates.end(),
        [](const pair<size_t, UsageRate> &lhs, const pair<size_t, UsageRate> &rhs) { return lhs.second < rhs.second; });
}

/*
* ���ڵ�ǰ�⣬Ѱ�ҵ�ǰԭ�Ͽ�ʼλ�õ�ǰһ��ԭ�ϵĿ�ʼλ��������
* ���룺sol��Ҫ����Ľ⣩��cur_index����ǰԭ�Ͽ�ʼλ���ڽ��е���������
* ���������ֵ��ǰһ��ԭ�ϵ���ʼλ����������
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
