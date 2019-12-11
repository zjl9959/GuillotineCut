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

using namespace std;

namespace szx {

void UnitTest::run() {
    // [zjl][TODO]:完善测试函数，添加自动读入参数代码。
    // 测试cutSearch
    test_CutSearch();
    // 测试plateSearch
    //test_PlateSearch();
}

/*
* 辅助测试CutSearch的run接口。
*/
void UnitTest::test_CutSearch() {
    // 该部分需要手动设置参数
    TID plate = 0;
    TCoord start_pos = 0;
    List<List<TID>> stacks = { {1}, {42}, {20}, {23}, {33} };
    Batch batch(stacks);
    CutSearch solver(plate, start_pos, 1, true);
    solver.run(batch);
    std::cout << solver.best_obj().str() << std::endl;
    Solution sol;
    solver.get_best_sol(sol);
    cout << sol << endl;
}

/*
* 测试PlateSearch的beam_search接口。
*/
void UnitTest::test_PlateSearch() {
    // 手动设置参数
    TID plate = 1;
    List<TID> items = { 78, 57, 58, 36, 16, 55, 41, 37, 42, 38, 43, 39, 6, 90, 67, 59, 7, 60, 91, 92, 93 };
    // 进行测试
    std::reverse(items.begin(), items.end());
    List<List<TID>> stacks;
    stacks.push_back(items);
    Batch batch(stacks);
    PlateSearch solver(plate, 1);
    solver.run(batch);
    Solution sol;
    solver.get_best_sol(sol);
}

/*
* 测试Placement结构体和指针在构造和复制时的速度差别。
*/
void UnitTest::test_copy_speed_of_placement(int repeat, int nb_element) {
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
