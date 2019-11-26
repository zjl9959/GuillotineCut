#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    size_t mtbn; // TopSearch����֧��Ŀ
    size_t mpbn; // PlateSearch����֧��Ŀ
    size_t mppn; // PlateSearchΪÿ��1-cut�����ѡ��Ʒ��Ŀ
    size_t mcbn; // CutSearch::beam_search����֧��Ŀ

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
