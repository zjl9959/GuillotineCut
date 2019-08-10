#pragma once
#ifndef SMART_ZJL_GUILLOTINECUT_TOPSEARCH_H
#define SMART_ZJL_GUILLOTINECUT_TOPSEARCH_H

#include "Solver.h"
#include "PlateSearch.h"

using namespace szx;

class TopSearch : public Solver {
public:
    TopSearch(const Problem::Input &inputData, const Environment &environment, const Configuration &config)
		: Solver(inputData, environment, config) {}
    ~TopSearch() {}
	
	void solve();

protected:
	void init();
	void run();
	Length evaluateOnePlate(TID cur_plate, List<MyStack>& batch, MySolution &comp_sol);

public: // 只在run()中修改
	Length best_objective;
	List<MyStack> source_batch;  
};

#endif // !SMART_ZJL_GUILLOTINECUT_TOPSEARCH_H

