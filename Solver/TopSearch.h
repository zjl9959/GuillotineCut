#pragma once
#ifndef SMART_ZJL_GUILLOTINECUT_TOPSEARCH_H
#define SMART_ZJL_GUILLOTINECUT_TOPSEARCH_H

#include "Solver.h"
#include "PlateSearch.h"

namespace szx {

class TopSearch : public Solver {
public:
    TopSearch(const Problem::Input &inputData, const Environment &environment, const Configuration &config) :
        Solver(inputData, environment, config) {}
    ~TopSearch() {}
private:
    
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_TOPSEARCH_H

