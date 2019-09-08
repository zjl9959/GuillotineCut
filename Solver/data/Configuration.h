#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    int mcin; // CutSearch最大优化物品数目
    int mbpn; // TopSearch最大分支数目
    int mbcn; // PlateSearch最大分支数目

    Configuration(int MCIN = 8, int MBPN = 4, int MHCN = 10) :
        mcin(MCIN), mbpn(MBPN), mbcn(MHCN) {}

    String toBriefStr() const {
        std::ostringstream os;
        os << "mcin=" << mcin
            << ";mbpn=" << mbpn
            << ";mbcn=" << mbcn;
        return os.str();
    }
};

}

#endif
