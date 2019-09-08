#pragma once
#ifndef GUILLOTINECUT_PLATESEARCH_H
#define GUILLOTINECUT_PLATESEARCH_H

#include <mutex>

#include "CutSearch.h"
#include "../data/Configuration.h"
#include "../data/Auxiliary.h"

namespace szx{

class PlateSearch {
public:
    PlateSearch(ID plate, Configuration &cfg, Random &rand, Timer &timer, const Auxiliary &aux) :
		cfg_(cfg), rand_(rand), timer_(timer), plate_(plate), aux_(aux) {};
    Score run(const Batch &batch, Solution &sol, bool fast_opt = false);

protected:
	Score beam_search(const Batch &source_batch, Solution &sol);
	Area greedy_evaluate(int repeat_num, const Batch &source_batch, const Solution &sol);
    Score get_cutsol(int repeat_num, Coord start_pos, const Batch &source_batch, Solution &sol, bool tail = false);
private:
    Area item_area(const Solution &sol);
    void update_bestsol(const Solution &sol, Area obj = 0);
private:
	static constexpr ScorePair InvalidPair = std::make_pair(-1, 0.0);
    ID plate_;          // 优化的原料id
    Configuration &cfg_;
    Random &rand_;
    Timer &timer_;
    Auxiliary aux_;
    
    std::mutex sol_mutex_;  // 更新最优解时需先获得该锁
    Solution bestsol_;      // 最优解
    Area bestobj_;          // 最优目标函数值
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
