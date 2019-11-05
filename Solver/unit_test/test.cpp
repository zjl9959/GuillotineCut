#include "test.h"

#include "../data/Problem.h"
#include "../algorithm/Factory.h"
#include "../algorithm/TopSearch.h"

using namespace szx;

namespace unit_test {

/* ��������CutSearch��run�ӿ� */
Solution test_CutSearch(Auxiliary &aux) {
    // �ò�����Ҫ�ֶ����ò���
    TID plate = 0;
    TCoord start_pos = 1239;
    List<TID> items = {22, 0, 23, 24, 1};
    // ���в���
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);

	using CutSearch = CutSearch;
    CutSearch solver(plate, start_pos, aux);
    Solution sol;
    UsageRate score = solver.run(batch, sol);
	std::cout << score.str() << std::endl;
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
