#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    size_t mtbn; // TopSearch最大分支数目
    size_t mpbn; // PlateSearch最大分支数目
    size_t mcin; // CutSearch最大优化物品数目
    size_t mcrn; // CutSearch最大重复优化次数

    Configuration(size_t MTBN = 1, size_t MPBN = 1, size_t MCIN = 16, size_t MCRN = 1) :
        mtbn(MTBN), mpbn(MPBN), mcin(MCIN), mcrn(MCRN) {}

    String toBriefStr() const {
        std::ostringstream os;
        os << "mtbn=" << mtbn
            << ";mpbn=" << mpbn
            << ";mcin=" << mcin
            << ";mcrn=" << mcrn;
        return os.str();
    }
};

}

#endif
