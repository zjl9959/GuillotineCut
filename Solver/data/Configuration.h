#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    size_t mtbn; // TopSearch最大分支数目
    size_t mpbn; // PlateSearch最大分支数目
    size_t mppn; // PlateSearch为每个1-cut最大挑选物品数目
    size_t mcbn; // CutSearch::beam_search最大分支数目

    Configuration(size_t _mtbn = 64, size_t _mpbn = 64, size_t _mppn = 16, size_t _mcbn = 30) :
        mtbn(_mtbn), mpbn(_mpbn), mppn(_mppn), mcbn(_mcbn) {}

    String toBriefStr() const {
        std::ostringstream os;
        os << "mtbn=" << mtbn
            << ";mpbn=" << mpbn
            << ";mppn=" << mppn
            << ";mcbn=" << mcbn;
        return os.str();
    }
};

}

#endif
