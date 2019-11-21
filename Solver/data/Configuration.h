#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    size_t mtbn; // TopSearch����֧��Ŀ
    size_t mpbn; // PlateSearch����֧��Ŀ
    size_t mcin; // CutSearch����Ż���Ʒ��Ŀ
    size_t mcrn; // CutSearch����ظ��Ż�����

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
