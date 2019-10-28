#include "Placement.h"

#include <fstream>

using namespace std;

namespace szx {

Solution & szx::operator+=(Solution & lhs, Solution & rhs) {
    for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        lhs.push_back(*it);
    }
    return lhs;
}

void save_solution(const Solution &sol, const String &path) {
    ofstream ofs(path);
    if (!ofs.is_open())return;
    ofs << "item,c1cpl,c1cpr,c2cpb,c2cpu,c3cp,c4cp,flag" << endl;
    for (auto it = sol.cbegin(); it != sol.cend(); ++it) {
        ofs << it->item << "," << it->c1cpl << ","
            << it->c1cpr << "," << it->c2cpb << ","
            << it->c2cpu << "," << it->c3cp << ","
            << it->c4cp << "," << it->flag << endl;
    }
}

}
