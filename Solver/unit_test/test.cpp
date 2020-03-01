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

using MS = std::chrono::milliseconds;
using US = std::chrono::microseconds;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using Clock = std::chrono::steady_clock;

void UnitTest::run() {
    // [zjl][TODO]:���Ʋ��Ժ���������Զ�����������롣
    // ����cutSearch
    test_CutSearch();
    // ����plateSearch
    //test_PlateSearch();
}

/*
* ��������CutSearch��run�ӿڡ�
*/
void UnitTest::test_CutSearch() {
    cout << "test cut search" << endl;
    TID plate = 0;
    TCoord start_pos = 1239;
    List<List<TID>> stacks;
    stacks = { {3, 2, 1, 24, 23, 22, 0} }; // �ֶ�д�����ݡ�
    //stacks = gv::stacks;            // ��ȫ�������ж���batch��
    Batch batch(stacks);
    TimePoint start_time(Clock::now());
    CutSearch solver(plate, start_pos, 0);
    solver.run(batch);
    std::cout << solver.best_obj().str() << std::endl;
    cout << "cost time:" << std::chrono::duration_cast<US>(Clock::now() - start_time).count() << "ms";
    Solution sol;
    solver.get_best_sol(sol);
    cout << sol << endl;
}

/*
* ����PlateSearch��beam_search�ӿڡ�
*/
void UnitTest::test_PlateSearch() {
    cout << "test plate search" << endl;
    TID plate = 0;
    List<List<TID>> stacks;
    stacks = { {6, 25, 5, 4, 3, 2, 1, 24, 23, 22, 0, 21, 20} };     // �ֶ�д�����ݡ�
    //stacks = gv::stacks;    // ��ȫ�������ж���batch��
    Batch batch(stacks);
    TimePoint start_time(Clock::now());
    PlateSearch solver(plate);
    solver.run(batch);
    cout << "cost time:" << std::chrono::duration_cast<MS>(Clock::now() - start_time).count() << "ms";
    Solution sol;
    solver.get_best_sol(sol);
    cout << sol << endl;
}

/*
* ����Placement�ṹ���ָ���ڹ���͸���ʱ���ٶȲ��
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
