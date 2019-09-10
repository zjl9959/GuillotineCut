#include "Placement.h"

namespace szx {

Solution & szx::operator+=(Solution & lhs, Solution & rhs) {
    for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        lhs.push_back(*it);
    }
    return lhs;
}

}
