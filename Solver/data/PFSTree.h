#pragma once
#ifndef GUILLOTINE_CUT_PFSTREE_H
#define GUILLOTINE_CUT_PFSTREE_H

#include "Placement.h"

#include <set>
#include <unordered_set>
#include <cassert>

namespace szx {

class PfsTree {
    // [TODO]: test this class.
public:
    struct Node : public Placement {
        Depth depth;
        TID nb_child;
        Area area;
        Node *parent;

        Node(TCoord C1cpl) : Placement(C1cpl), depth(0), nb_child(0), area(0), parent(nullptr) {}
        Node(Node &_parent, Status _flag, TID _item, Area _item_area) : Placement(_parent, _item, _flag),
            depth(_parent.depth + 1), nb_child(0), area(_parent.area + _item_area), parent(&_parent) {}
        
        ~Node() { parent = nullptr; }
    };

    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // 限制搜索树的最大节点数目。
    static constexpr double memory_adjust_rate = 0.70;   // 当节点数过多时，新节点数目占现节点数目的百分比。
private:
    class Score : public UsageRate {
        // TODO: add implement.
    };
    struct Score2Node { // 保存每个节点的地址和对应分数，用于快速定位下一个带扩展节点。
        Score score;
        Node *node;
        Score2Node(Score _score, Node *_node) : score(_score), node(_node) {}
    };
    struct Score2NodeCompare {
        bool operator() (const Score2Node &lhs, const Score2Node &rhs) const {
            return lhs.score < rhs.score;
        }
    };
public:
    PfsTree(TCoord start_pos) : nb_node_(0), start_pos_(start_pos) {}
    ~PfsTree() {
        for(auto it = score_queue_.begin(); it != score_queue_.end(); ++it) {
            delete_node_recursive(it->node);
        }
        assert(0 == nb_node_);
    }

    /* 获取下一个待扩展节点。*/
    const Node* get() {
        Node *pnode = score_queue_.begin()->node;
        score_queue_.erase(score_queue_.begin());
        assert(pnode != nullptr);
        return pnode;
    }

    /* 添加新节点。*/
    void add(Node *node) {
        score_queue_.insert(Score2Node(get_score(node), node));
        ++nb_node_;
        ++(node->parent->nb_child);
        if (nb_node_ > max_nb_node) {
            adjust_memory();
        }
    }

    /* 获取路径上的完整解。*/
    void get_path(const Node *node, Solution &sol) const {
        Solution temp_sol; // 为了将节点顺序翻转回来。
        temp_sol.reserve(node->depth);
        while (node != nullptr) {
            temp_sol.push_back(*node);
            node = node->parent;
        }
        for (auto it = temp_sol.rbegin(); it != temp_sol.rend(); ++it) {
            sol.push_back(*it);
        }
    }
private:
    /* 计算当前节点的分数。*/
    Score get_score(const Node *node) const {
        // TODO ：如何计算分数？
    }

    /* 节点数目超过最大限制时调用该函数删除部分较差的节点。*/
    void adjust_memory() {
        size_t new_nb_node = static_cast<double>(nb_node_) * memory_adjust_rate;
        auto it = score_queue_.rbegin();
        while (it != score_queue_.rend() && nb_node_ > new_nb_node) {
            delete_node_recursive(it->node);
        }
    }

    /* 从node节点开始，沿路径依次删除叶子节点。 */
    void delete_node_recursive(Node *node) {
        while (node != nullptr && node->nb_child == 0) {
            Node *parent = node->parent;
            --(parent->nb_child);
            delete node;
            node = parent;
            --nb_node_;
        }
    }
private:
    size_t nb_node_;    // 当前已扩展的节点数目。
    TCoord start_pos_;   // 1-cut的开始位置。
    std::set<Score2Node, Score2NodeCompare> score_queue_;   // 待扩展节点按照优度排列。
};

}

#endif
