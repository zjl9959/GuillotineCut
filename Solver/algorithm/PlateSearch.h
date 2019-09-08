#pragma once
#ifndef GUILLOTINECUT_PLATESEARCH_H
#define GUILLOTINECUT_PLATESEARCH_H

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
	ScorePair branch(Coord start_pos, const Batch &batch, List<Solution>& sols);
	Area evaluate(const Batch &batch, Solution &sol);
private:
	static constexpr ScorePair InvalidPair = std::make_pair(-1, 0.0);
    ID plate_;
    Configuration &cfg_;
    Random &rand_;
    Timer &timer_;
    Auxiliary aux_;
};

}

#endif // !GUILLOTINECUT_PLATESEARCH_H
