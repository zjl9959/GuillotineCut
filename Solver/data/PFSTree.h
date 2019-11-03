#pragma once
#ifndef GUILLOTINE_CUT_PFSTREE_H
#define GUILLOTINE_CUT_PFSTREE_H

#include "Placement.h"
#include <queue>

namespace szx {

class PfsTree {
public:
    struct Node : public Placement {
        Depth depth;
        Score score;
        Node *parent;

        Node(TCoord C1cpl) : Placement(C1cpl), depth(0), score(0.0), parent(nullptr) {}
        Node(Node &_parent, Status _flag, TID _item, Score _score) :
            Placement(_parent, _item, _flag), depth(_parent.depth + 1), score(_score), parent(&_parent) {}
        
        ~Node() {
            parent = nullptr;
        }
    };

    static constexpr size_t max_nb_tree_node = (1 << 30) / sizeof(Node); // ��������Node��ռ�ڴ治����1G
public:
    PfsTree() : nb_tree_node(0) {}
    ~PfsTree() {
        // TODO: �������new�����Ľڵ㣨��Ҷ�ӽڵ㿪ʼ����delete��
    }

    Node& get() {
        // TODO: ����ʵ��
    }

    void add(const Node &node) {
        // TODO: ����ʵ��
    }

private:
    // TODO: ���ڴ治��ʱ�Ĵ����ʩ
private:
    struct Score2Node {
        Score score;
        Node *node;
    };
    struct Score2NodeCompare {
        bool operator() (const Score2Node &lhs, const Score2Node &rhs) const {
            return lhs.score < rhs.score;
        }
    };
private:
    size_t nb_tree_node;
    std::priority_queue<Score2Node, std::vector<Score2Node>, Score2NodeCompare()> score_queue_;
};

}

#endif
