#pragma once
#ifndef GUILLOTINE_CUT_PFSTREE_H
#define GUILLOTINE_CUT_PFSTREE_H

#include <map>

#include "Solver/data/header/Placement.h"
#include "Solver/data/header/Batch.h"

namespace szx {

class PfsTree {
public:
    struct Node {
        Depth depth;    // 节点深度
        TID nb_child;   // 子节点数量
        Node *parent;   // 父节点地址
        Placement place;   // 放置位置

        /* 用于创建第一层节点 */
        Node(Placement &_place, Area _item_area) : depth(0),
            nb_child(0), parent(nullptr), place(_place) {}
        /* 用于创建后续节点 */
        Node(Node *_parent, Placement &_place, Area _item_area) : depth(_parent->depth + 1),
            nb_child(0), parent(_parent), place(_place) {}
    };
private:
    class Score {
    public:
        explicit Score(double rate, Depth depth) : rate_(rate), depth_(depth) {}
        bool operator< (const Score &rhs) const {
            return abs(this->rate_ - rhs.rate_) < tolerance_ ?
                this->depth_ > rhs.depth_ : this->rate_ > rhs.rate_;
        }
    private:
        static constexpr double tolerance_ = 0.01;  // 分数值在一定差别内视为相等。
        double rate_;   // 利用率值（加权后）。
        Depth depth_;   // 当前节点扩展深度。
    };
    struct ScoreCmp {
        bool operator() (const Score &lhs, const Score &rhs) {
            return lhs < rhs;
        }
    };
public:
    ~PfsTree();
    bool empty() const { return live_nodes_.empty(); }  // 判断树中是否有未被扩展的节点。
    Node* get();    // 获取下一个待扩展节点。
    void add(Node *parent, Placement &place, double score); // 添加新节点。
    void add(Placement &place, double score);   // 添加新节点（第一个节点）。
    void add_leaf_node(Node *node) { leaf_nodes_.push_back(node); } // 添加已被完全扩展的叶子节点。
    // 在搜索过程中更新batch；输入：last_node（上一次扩展的节点），cur_node（当前待扩展的节点），batch（物品栈）
    void update_batch(Node *last_node, Node *cur_node, Batch &batch);
    void get_tree_path(Node *node, Solution &sol) const;        // 获取当前解到树根的路径。
private:
    void adjust_memory();
    void delete_node_recursive(Node *node);
private:
    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // 限制搜索树的最大节点数目。
    static constexpr double memory_adjust_rate = 0.70;   // 当节点数过多时，新节点数目占现节点数目的百分比。

    size_t nb_node_ = 0;    // 当前已扩展的节点数目。
    std::multimap<Score, Node*, ScoreCmp> live_nodes_;  // 按照Score优度排列的待扩展节点。
    List<Node*> leaf_nodes_;    // 已经被完全扩展的节点。
};

}

#endif
