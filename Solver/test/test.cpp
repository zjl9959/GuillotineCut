#include "test.h"

#include <fstream>
#include <string>

#include "../CsvReader.h"

using namespace szx;
using namespace std;

namespace zjl_test {

void TestSolver::TestToOutput() {
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
    init();
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

}
