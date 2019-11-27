#include "test.h"

#include <iostream>
#include <chrono>

#include "Solver/data/header/Problem.h"
#include "Solver/data/header/Global.h"
#include "Solver/data/header/PFSTree.h"
#include "Solver/algorithm/header/TopSearch.h"
#include "Solver/algorithm/header/PlateSearch.h"
#include "Solver/algorithm/header/CutSearch.h"
#include "Solver/utility/Utility.h"

using namespace szx;
using namespace std;

namespace unit_test {

/* 辅助测试CutSearch的run接口 */
Solution test_CutSearch() {
    // 该部分需要手动设置参数
    TID plate = 0;
    TCoord start_pos = 1239;
    List<TID> items = {22, 0, 23, 24, 1};
    // 进行测试
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);
    CutSearch::Setting set;
    CutSearch solver(plate, 1, start_pos, set);
    solver.run(batch);
	std::cout << solver.best_obj().str() << std::endl;
    Solution sol;
    solver.get_best_sol(sol);
    return sol;
}

/* 测试PlateSearch的beam_search接口 */
Solution test_PlateSearch() {
    // 手动设置参数
    TID plate = 1;
    List<TID> items = {78, 57, 58, 36, 16, 55, 41, 37, 42, 38, 43, 39, 6, 90, 67, 59, 7, 60, 91, 92, 93};
    // 进行测试
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);
    PlateSearch solver(plate, 1);
    solver.beam_search(batch);
    Solution sol;
    solver.get_best_sol(sol);
    return sol;
}

/* 测试Placement结构体和指针在构造和复制时的速度差别 */
void test_copy_speed_of_placement(int repeat, int nb_element) {
    cout << "test copy speed of placement and placement*" << endl;
    cout << "repeat:" << repeat << " nb_element:" << nb_element << endl;
    List<Placement> source1, dest1;
    List<Placement*> source2, dest2;
    source1.reserve(nb_element); dest1.reserve(nb_element);
    source2.reserve(nb_element); dest2.reserve(nb_element);
    chrono::time_point<chrono::steady_clock> start, end;
    start = chrono::steady_clock::now();
    for (int i = 0; i < repeat; ++i) {
        source1.clear();
        for (int j = 0; j < nb_element; ++j) {
            source1.push_back(Placement());
        }
        dest1 = source1;
    }
    end = chrono::steady_clock::now();
    cout << "copy placement cost time:" << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
    start = chrono::steady_clock::now();
    for (int i = 0; i < repeat; ++i) {
        source2.clear();
        Placement *pp = new Placement();
        for (int j = 0; j < nb_element; ++j) {
            source2.push_back(pp);
        }
        dest2 = source2;
        delete pp;
    }
    end = chrono::steady_clock::now();
    cout << "copy pointer cost time:" << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
}

}
