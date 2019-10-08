#include "test.h"

#include "../data/Problem.h"
#include "../algorithm/Factory.h"
#include "../algorithm/TopSearch.h"

using namespace szx;

namespace unit_test {

/* ��������CutSearch��run�ӿ� */
Solution test_CutSearch(Auxiliary &aux) {
    // �ò�����Ҫ�ֶ����ò���
    TID plate = 1;
    TCoord start_pos = 2053;
    List<TID> items = {80, 38, 39, 81};
    // ���в���
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);
    CutSearch solver(plate, start_pos, aux);
    Solution sol;
    solver.run(batch, sol);
    return sol;
}

/* ����PlateSearch��beam_search�ӿ� */
Solution test_PlateSearch(Timer &timer, Random &rand, Configuration &cfg, Auxiliary &aux) {
    // �ֶ����ò���
    TID plate = 1;
    List<TID> items = {78, 57, 58, 36, 16, 55, 41, 37, 42, 38, 43, 39, 6, 90, 67, 59, 7, 60, 91, 92, 93};
    // ���в���
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);
    PlateSearch solver(plate, cfg, rand, timer, aux);
    solver.beam_search(batch);
    Solution sol;
    solver.get_bestsol(sol);
    return sol;
}

}
