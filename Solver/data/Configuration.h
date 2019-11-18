#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    int mtbn; // TopSearch最大分支数目
    int mpbn; // PlateSearch最大分支数目
    int mcin; // CutSearch最大优化物品数目
    int mcrn; // CutSearch最大重复优化次数

    Configuration(int MTBN = 32, int MPBN = 8, int MCIN = 8, int MCRN = 1) :
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
