#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

namespace szx {

struct Configuration {
    int mtbn; // TopSearch����֧��Ŀ
    int mpbn; // PlateSearch����֧��Ŀ
    int mcin; // CutSearch����Ż���Ʒ��Ŀ
    int mcrn; // CutSearch����ظ��Ż�����

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
