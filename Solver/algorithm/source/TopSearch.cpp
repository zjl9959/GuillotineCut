#include "Solver/algorithm/header/TopSearch.h"

#include <numeric>

#include "Solver/algorithm/header/Picker.h"
#include "Solver/algorithm/header/PlateSearch.h"
#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/data/header/Global.h"

using namespace std;

namespace szx {

void TopSearch::run()
{
    beam_search();
}

void TopSearch::beam_search()
{
    Solution fix_sol;                   // �Ѿ��̶��Ľ�
    ID cur_plate = 0;                   // ��ǰԭ��id
    Batch batch(gv::stacks);            // ��ǰ��Ʒջ
    Solution best_platesol;             // ÿһ�ֵ�����plate��ͻ���plate��
    List<Solution> branch_sols;         // �����֧�����Ľ�
    while (!gv::timer.isTimeOut())
    {
        Length best_obj = Problem::Output::MaxWidth;
        branch(cur_plate, batch, branch_sols, gv::cfg.mtbn);
        for (auto &psol : branch_sols)
        {
            batch.remove(psol);
            fix_sol += psol;
            Length obj = greedy_evaluate(cur_plate, batch, fix_sol);
            batch.add(psol);
            fix_sol -= psol;
            if (best_obj > obj)
            {
                best_obj = obj;
                best_platesol = psol;
            }
        }
        if (best_obj < Problem::Output::MaxWidth)
        {
            fix_sol += best_platesol;
            batch.remove(best_platesol);
            cur_plate++;
        }
        else
        {
            break;
        }
    }
    cout << endl;
}

/*
* ���ܣ���֧������batch������ѡ������
* ���룺plate_id������֧ԭ��id����source_batch����Ʒջ����
* �����sols����֧�����ľֲ�ԭ�Ͻ⣩��
*/
void TopSearch::branch(ID plate_id, const Batch &source_batch, List<Solution> &sols, size_t nb_branch)
{
    Picker picker(source_batch);
    Batch batch;
    sols.resize(nb_branch);
    Picker::Terminator terminator(0, static_cast<Area>(gv::param.plateHeight * gv::param.plateWidth * 2.5));
    size_t index = 0;
    for (size_t i = 0; i < nb_branch; ++i)
    {
        if (picker.rand_pick(batch, terminator))
        {
            PlateSearch solver(plate_id); // ֻҪһ�����Ž⼴�ɡ�
            solver.run(batch);
            if (solver.best_obj() > 0)
            {
                solver.get_best_sol(sols[index++]);
            }
        }
    }
    sols.resize(index);
}

/* 
* ���ܣ�̰�ĵ��ߵ��ף�������sol�ĺû�
* ���룺plate_id��ԭ��id����source_batch����Ʒջ��
* �����sol��ԭ���ϵĽ⣩�����أ�sol������ֵ��ԭ��ʹ�ó��ȣ�
*/
Length TopSearch::greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &fix_sol)
{
    if (fix_sol.empty()) return gv::param.plateWidth *gv::param.plateNum;
    Batch batch(source_batch);
    List<Solution> branch_sols;
    Solution cur_sol(fix_sol);
    ++plate_id;
    while (!gv::timer.isTimeOut() && batch.size() != 0)
    {
        branch_sols.clear();
        branch(plate_id, batch, branch_sols);
        if (!branch_sols.empty())
        {
            cur_sol += branch_sols[0];
            batch.remove(branch_sols[0]);
            plate_id++;
            assert(nb_used_plate(cur_sol) == plate_id);
        }
        else
        {
            return Problem::Output::MaxWidth;
        }
    }
    Length obj = (plate_id - 1) * gv::param.plateWidth + cur_sol.back().c1cpr;
    update_best_sol(cur_sol, obj);
    return obj;
}

/* ����Ŀ�꺯��ֵ����ʹ��ԭ�ϵ��ܳ��� */
Length TopSearch::get_obj(const Solution &sol)
{
    if(sol.empty()) return Problem::Output::MaxWidth;
    ID plate_num = 0;
    for (auto it = sol.begin(); it != sol.end(); ++it)
    {
        if (it->getFlagBit(Placement::NEW_PLATE))
        {
            ++plate_num;
        }
    }
    return (plate_num - 1)* gv::param.plateWidth + sol.back().c1cpr;
}

/* ���sol�Ƿ�����bestsol_�����������bestsol_ */
void TopSearch::update_best_sol(const Solution &sol, Length obj)
{
    if (sol.empty() || sol.size() != gv::items.size())
    {
        return;
    }
    if (obj < 0)
    {
        obj = get_obj(sol);
    }
    lock_guard<mutex> guard(sol_mutex_);
    if (best_obj_ > obj)
    {
        Log(Log::Debug) << "=> " << total_item_area_ / (double)(obj * gv::param.plateHeight) * 100 << "%" << endl;
        best_obj_ = obj;
        best_sol_ = sol;
    }
    // ��¼ÿ��ԭ�ϵ���Ϣ��
    gv::info.iter_plate_wastes.push_back(get_plates_waste(sol));
}

List<Area> TopSearch::get_plates_waste(const Solution &sol)
{
    List<Area> res;
    if (sol.empty()) return res;
    const Area plate_area = gv::param.plateHeight * gv::param.plateWidth;
    Area item_area = gv::item_area[sol[0].item];    // ԭ������Ʒ�����
    for (size_t i = 1; i < sol.size(); ++i)
    {
        if (sol[i].getFlagBit(Placement::NEW_PLATE))
        {
            res.push_back(plate_area - item_area);
            item_area = gv::item_area[sol[i].item];
        }
        else
        {
            item_area += gv::item_area[sol[i].item];
        }
    }
    // �������������ʡ�
    const Area tail_plate_area = gv::param.plateHeight * sol.back().c1cpr;
    res.push_back(tail_plate_area - item_area);
    return res;
}

}
