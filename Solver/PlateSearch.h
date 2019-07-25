#pragma once
#ifndef SMART_ZJL_GUILLOTINECUT_PLATESEARCH_H
#define SMART_ZJL_GUILLOTINECUT_PLATESEARCH_H

#include "CutSearch.h"

namespace szx {

class MCTNode { // monte-carlo tree node.
public:
    enum NodeStatus {
        FULL_EXPAND = 0x0001,   // this bit means this node is fully expanded.
    };
    using MCTPointer = MCTNode* ;
public:
    MCTNode(MCTPointer _parent = nullptr) : partial_usage_rate(0.0), best_objective(0.0), expand_count(0), parent(_parent) {}

    void add_solution_nodes(const List<SolutionNode> &nodes, Score usage_rate) {
        partial_solution = nodes;
        partial_usage_rate = usage_rate;
    }
    void add_solution_nodes(List<SolutionNode> &&nodes, Score usage_rate) {
        partial_solution.swap(nodes);
        partial_usage_rate = usage_rate;
    }

    size_t get_expand_count() const { return expand_count; }
    void add_expand_count() { expand_count++; }

    void update_objective(Score obj) { best_objective = obj; }

    MCTPointer get_parent() const { return parent; }

    const List<MCTPointer>& get_childrens() const {
        return childrens;
    }
    void add_children(MCTPointer node) {
        childrens.push_back(node);
    }
    void erase_children(MCTPointer node) {
        auto it = childrens.begin();
        for(; it!=childrens.end(); ++it) {
            if (*it == node) break;
        }
        if (it != childrens.end()) {
            childrens.erase(it);
            freeSubNodes(*it);
        }
    }

    const List<SolutionNode>& get_solution_nodes() {
        return partial_solution;
    }

    void update_state(Status _state) { state |= state; }
    bool get_state(Status _flagbit) const { return state & _flagbit; }
protected:
    // free all the nodes in the sub tree.
    void freeSubNodes(MCTPointer root) {
        if (root) {
            for (MCTPointer node : root->get_childrens()) {
                freeSubNodes(node);
            }
            delete root;
        }
    }
private:
    Status state;
    Score partial_usage_rate;
    Score best_objective;
    size_t expand_count;
    List<SolutionNode> partial_solution;

    MCTPointer parent;
    List<MCTPointer> childrens;
};

class PlateSearch {
public:
    PlateSearch(Random &rand, Timer &timer, TID plate) :
        rand_(rand), timer_(timer), plate_(plate) {};
    Score mcts(List<List<TID>> &batch, List<SolutionNode> &sol, int max_iter = 10000);
protected:
    bool gen_item_sequence(int seq_len, MCTNode *node, List<List<TID>> &batch, List<TID> &seq);
private:
    Random &rand_;
    Timer &timer_;
    TID plate_;
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_PLATESEARCH_H
