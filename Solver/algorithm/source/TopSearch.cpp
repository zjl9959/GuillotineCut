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
        } else {
            break;
        }
    }
    cout << endl;
}

void TopSearch::local_search() {
    Solution s0;    // ��ʼ��
    generate_init_sol(s0);
    update_best_sol(s0);
    int iter = 0;   // ��������
    while (!gv::timer.isTimeOut()) {
        if (find_first_improvement(s0) == false) {
            // ����Ҳ������������Ķ��������������Ŷ���
            // [zjl][TODO]:������Ŷ���
        }
        assert(s0.size() == gv::items.size());
        update_best_sol(s0);
        ++iter;
    }
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
* ���ܣ�̰�ĵ��ߵ��ף�������sol�ĺû�
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
    Length obj = (plate_id - 1) * gv::param.plateWidth + cur_sol.back().c1cpr;
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
    return (plate_num - 1)* gv::param.plateWidth + sol.back().c1cpr;
}

/* ���sol�Ƿ�����bestsol_�����������bestsol_ */
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
* ���ܣ�����һ����ʼ�⣨̰�ĵĲ��ԣ���
* ���������Ľ⡣
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
* ���ܣ�Ѱ�ҶԵ�ǰ����˵����һ���ܹ��Ľ��ֲ��ϲ�ԭ�ϵ�����������
* ���룺cur_sol����ǰ�����Ľ⣩��
* �����cur_sol���µĽ⣩������ֵ��true:�Ľ���false:δ�Ľ�����
*/
bool TopSearch::find_first_improvement(Solution &cur_sol) {
    static constexpr size_t max_k = 3;          // ������ǰk��ԭ�ϡ�

    List<int> plates_index = get_plates_index(cur_sol);
    List<double> usage_rates = get_plates_usage_rate(cur_sol);
    double average = accumulate(usage_rates.begin(), usage_rates.end(), 0.0) / plates_index.size();
    List<int> bad_plates;                       // �����ʵ���ƽ��ֵ��ԭ���б�
    for (int i = 0; i < plates_index.size(); ++i) {
        if (usage_rates[i] < average)
            bad_plates.push_back(i);
    }
    // ��Ϊ��first_improvement��ͨ���������ԭ��id˳��������㷨����ɢ�ԡ�
    for (int nb_rand_suff = 0; nb_rand_suff < bad_plates.size(); ++nb_rand_suff) {
        int temp = gv::rand.pick(bad_plates.size());
        swap(bad_plates.begin() + temp, bad_plates.begin() + bad_plates.size() - 1);
    }
    for (int plate : bad_plates) {              // ����ÿһ��ϲ��ԭ�ϡ�
        for (size_t k = 1; k <= max_k; ++k) {   // �����С����
            Batch batch(gv::stacks);
            if (plate < k) break;               // ǰ��û����ô��ԭ�Ͽɹ������ˡ�
            for (int i = 0; i < plates_index[plate - k]; ++i) {
                batch.remove(cur_sol.at(i).item);
            }
            // ���¹���ľֲ��⡣
            Solution partial_sol;
            double total_partial_usage_rate = 0.0;  // ���¹�����������ͳ�ƺ͡�
            double total_bad_usage_rate = 0.0;      // �ϵľֲ����������ͳ�ƺ͡�
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
            // ���partial_sol�Ƿ������ϵĽ⡣
            if (total_partial_usage_rate > total_bad_usage_rate) {
                cur_sol.resize(plates_index[plate - k]);
                cur_sol += partial_sol;
                while (batch.size() != 0) {     // ������������Ľ⡣
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
* ���ܣ���������Ľ��У�ÿ��ԭ�ϵ������ʡ�
* ���룺sol��һ�������Ľ⣬�������һ��ԭ����Ϊ������������
* ���������ֵ��ÿ��ԭ�ϵ������ʣ���
*/
List<double> TopSearch::get_plates_usage_rate(const Solution &sol) {
    List<double> res;
    if (sol.empty()) return res;
    const double plate_area = gv::param.plateHeight * gv::param.plateWidth;
    Area item_area = gv::item_area[sol[0].item];    // ԭ������Ʒ�����
    size_t plate_index = 0;                         // ԭ����ʼλ����sol�е�������
    for (size_t i = 1; i < sol.size(); ++i) {
        if (sol[i].getFlagBit(Placement::NEW_PLATE)) {
            res.push_back(item_area / plate_area);
            plate_index = i;
            item_area = gv::item_area[sol[i].item];
        } else {
            item_area += gv::item_area[sol[i].item];
        }
    }
    // �������������ʡ�
    const double tail_plate_area = gv::param.plateHeight * sol.back().c1cpr;
    res.push_back(item_area / tail_plate_area);
    return res;
}

/*
* ���ܣ�������ǰ���ÿ��ԭ��id�����ڽ��п�ʼλ��������
* ���룺sol��Ҫ����Ľ⣩��
* ���������ֵ�������б���
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
