#include "test.h"

#include "../data/Problem.h"
#include "../algorithm/Factory.h"
#include "../algorithm/TopSearch.h"

using namespace szx;

namespace unit_test {

/* 辅助测试CutSearch的run接口 */
Solution test_CutSearch(Auxiliary &aux) {
    // 该部分需要手动设置参数
    TID plate = 1;
    //TCoord start_pos = 2053;
    TCoord start_pos = 0;
    //List<TID> items = {80, 38, 39, 81};
    List<TID> items = {0, 1, 2, 3, 4};
    // 进行测试
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);

	using CutSearch = MyCutSearch;
    CutSearch solver(plate, start_pos, aux);
    Solution sol;
    Score score = solver.run(batch, sol);
	std::cout << score << std::endl;
    return sol;
}

/* 测试PlateSearch的beam_search接口 */
Solution test_PlateSearch(Timer &timer, Random &rand, Configuration &cfg, Auxiliary &aux) {
    // 手动设置参数
    TID plate = 1;
    List<TID> items = {78, 57, 58, 36, 16, 55, 41, 37, 42, 38, 43, 39, 6, 90, 67, 59, 7, 60, 91, 92, 93};
    // 进行测试
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
