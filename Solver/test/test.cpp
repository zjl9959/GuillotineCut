#include "test.h"

#include <fstream>
#include <string>

#include "../CsvReader.h"

using namespace szx;
using namespace std;

namespace zjl_test {

void TestSolver::test_toOutput() {
    Log(Log::Info) << "Test function toOutput" << endl;
    /*在debug模式下测试toOutPut,在release模式下计算best_solution*/
    #ifdef _DEBUG
    // 加载best_solution
    ifstream ifs("tmp/" + env.friendlyInstName());
    if (!ifs.is_open())return;
    best_solution.clear();
    CsvReader cr;
    const vector<CsvReader::Row> &rows(cr.scan(ifs));
    for (auto &row : rows) {
        SolutionNode sol;
        sol.item = atoi(row[0]);
        sol.c1cpl = atoi(row[1]);
        sol.c1cpr = atoi(row[2]);
        sol.c2cpb = atoi(row[3]);
        sol.c2cpu = atoi(row[4]);
        sol.c3cp = atoi(row[5]);
        sol.c4cp = atoi(row[6]);
        sol.flag = atoi(row[7]);
        best_solution.push_back(sol);
    }
    ifs.close();
    // 测试toOutput函数
    toOutput();
    #else
    solve();
    ofstream ofs("tmp/" + env.friendlyInstName());
    if (!ofs.is_open())return;
    for (auto it = best_solution.begin(); it != best_solution.end(); ++it) {
        ofs << it->item << CsvReader::CommaChar
            << it->c1cpl << CsvReader::CommaChar
            << it->c1cpr << CsvReader::CommaChar
            << it->c2cpb << CsvReader::CommaChar
            << it->c2cpu << CsvReader::CommaChar
            << it->c3cp << CsvReader::CommaChar
            << it->c4cp << CsvReader::CommaChar
            << it->flag << endl;
    }
    ofs.close();
    #endif // _DEBUG
}

void TestCutSearch::test_branch(const ItemNode & old, szx::TID & item) {
    List<List<TID>> batch;
    batch.resize(1);
    batch[0].push_back(item);
    List<ItemNode> branch_nodes;
    branch(old, batch, branch_nodes, true);
}

void test_cutSearch_branch() {
    ifstream ifs("tmp/_test_cutSearch_branch_cfg");
    if (!ifs.is_open()) return;
    vector<short> data;
    string line;
    int count = 0;
    while (getline(ifs, line)) {
        if (count % 2) {
            data.push_back(atoi(line.data()));
        }
        ++count;
    }
    ItemNode old;
    old.item = data[2];
    old.c1cpl = data[3];
    old.c1cpr = data[4];
    old.c2cpb = data[5];
    old.c2cpu = data[6];
    old.c3cp = data[7];
    old.c4cp = data[8];
    old.flag = data[9];
    TestCutSearch ts(data[0], data[1]);
    ts.test_branch(old, data[10]);
}

}
