#pragma once
#ifndef SMART_ZJL_GUILLOTINECUT_PLATESEARCH_H
#define SMART_ZJL_GUILLOTINECUT_PLATESEARCH_H

#include "CutSearch.h"

namespace szx {

class MCTNode { // monte-carlo tree node.
public:
    using MCTPointer = MCTNode* ;
    MCTNode() : partial_usage_rate(0.0), best_objective(0.0), expand_count(0), parent(nullptr) {}
    
    size_t get_expand_count() const { return expand_count; }
    void add_expand_count() { expand_count++; }

    void update_score(Score score) { best_objective = score; }

private:
    Score partial_usage_rate;
    Score best_objective;
    size_t expand_count;
    List<SolutionNode> partial_solution;

    MCTPointer parent;
    List<MCTPointer> childrens;
};


class PlateSearch {
public:
    PlateSearch() {};
    ~PlateSearch() {};
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_PLATESEARCH_H
