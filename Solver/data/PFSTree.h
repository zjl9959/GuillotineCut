#pragma once
#ifndef GUILLOTINE_CUT_PFSTREE_H
#define GUILLOTINE_CUT_PFSTREE_H

#include "Placement.h"
#include <queue>
#include <cassert>

namespace szx {

class PfsTree {
public:
    struct Node : public Placement {
        Depth depth;
        Area area;
        Node *parent;

        Node(TCoord C1cpl) : Placement(C1cpl), depth(0), area(0), parent(nullptr) {}
        Node(Node &_parent, Status _flag, TID _item, Area _item_area) :
            Placement(_parent, _item, _flag), depth(_parent.depth + 1), area(_parent.area + _item_area), parent(&_parent) {}
        
        ~Node() {
            parent = nullptr;
        }
    };

    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // 限制所有Node所占内存不超过1G
public:
    PfsTree() : nb_node_(0) {}
    ~PfsTree() {
        // TODO: 清除所有new出来的节点（从叶子节点开始依次delete）
    }

    /* 获取下一个待扩展节点。*/
    const Node* get() {
        Node *pnode = score_queue_.top().node;
        score_queue_.pop();
        assert(pnode != nullptr);
        return pnode;
    }

    /* 添加新节点。*/
    void add(Node *node) {
        score_queue_.push(Score2Node(get_score(node), node));
        ++nb_node_;
        if (nb_node_ > max_nb_node) {
            // TODO: 处理内存不足的情况。
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
private:
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
private:
    size_t nb_node_;    // 当前已扩展的节点数目。
    TCoord start_pos_;   // 1-cut的开始位置。
    std::priority_queue<Score2Node, std::vector<Score2Node>, Score2NodeCompare()> score_queue_; // 待扩展节点按照优度排列。
};

}

#endif
