#include "Placement.h"

#include <fstream>
#include <unordered_set>
#include <cassert>

using namespace std;

namespace szx {

Solution & szx::operator+=(Solution & lhs, const Solution & rhs) {
    for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        lhs.push_back(*it);
    }
    return lhs;
}

Solution & operator-=(Solution & lhs, const Solution & rhs) {
    for (auto it = rhs.crbegin(); it != rhs.crend(); ++it) {
        assert(it->item == lhs.back().item);
        lhs.pop_back();
    }
    return lhs;
}

bool item_repeat(const Solution &sol) {
    unordered_set<TID> item_set;
    for (auto it = sol.cbegin(); it != sol.cend(); ++it) {
        if (item_set.find(it->item) != item_set.end()) {
            return true;
        } else {
            item_set.insert(it->item);
        }
    }
    return false;
}

TID nb_used_plate(const Solution &sol) {
    TID res = 0;
    for (auto it = sol.cbegin(); it != sol.cend(); ++it)
        if (it->getFlagBit(Placement::NEW_PLATE))
            ++res;
    return res;
}

bool valid_plate_sol(const Solution &sol) {
    auto it = sol.cbegin();
    if (!it->getFlagBit(Placement::NEW_PLATE))  // 第一个节点一定要开一个新的原料。
        return false;
    for (++it; it != sol.cend(); ++it)  // 剩余的节点不能再开新的原料了。
        if (it->getFlagBit(Placement::NEW_PLATE))
                return false;
    return true;
}

void save_solution(const Solution &sol, const String &path) {
    ofstream ofs(path);
    if (!ofs.is_open())return;
    ofs << "item,c1cpl,c1cpr,c2cpb,c2cpu,c3cp,c4cp,flag" << endl;
    for (auto it = sol.cbegin(); it != sol.cend(); ++it) {
        ofs << it->item << "," << it->c1cpl << ","
            << it->c1cpr << "," << it->c2cpb << ","
            << it->c2cpu << "," << it->c3cp << ","
            << it->c4cp << "," << it->flag << "("
            << (it->getFlagBit(Placement::ROTATE) ? "ROATAE;" : "")
            << (it->getFlagBit(Placement::DEFECT_R) ? "DEFECT_R;" : "")
            << (it->getFlagBit(Placement::DEFECT_U) ? "DEFECT_U;" : "")
            << (it->getFlagBit(Placement::BIN4) ? "BIN4;" : "")
            << (it->getFlagBit(Placement::LOCKC2) ? "LOCKC2;" : "")
            << (it->getFlagBit(Placement::NEW_PLATE) ? "NEW_PLATE;" : "")
            << (it->getFlagBit(Placement::NEW_L1) ? "NEW_L1;" : "")
            << (it->getFlagBit(Placement::NEW_L2) ? "NEW_L2;" : "")
            << ")" << endl;
    }
}

}
