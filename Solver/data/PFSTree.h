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

    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // ��������Node��ռ�ڴ治����1G
public:
    PfsTree() : nb_node_(0) {}
    ~PfsTree() {
        // TODO: �������new�����Ľڵ㣨��Ҷ�ӽڵ㿪ʼ����delete��
    }

    /* ��ȡ��һ������չ�ڵ㡣*/
    const Node* get() {
        Node *pnode = score_queue_.top().node;
        score_queue_.pop();
        assert(pnode != nullptr);
        return pnode;
    }

    /* ����½ڵ㡣*/
    void add(Node *node) {
        score_queue_.push(Score2Node(get_score(node), node));
        ++nb_node_;
        if (nb_node_ > max_nb_node) {
            // TODO: �����ڴ治��������
        }
    }

    /* ��ȡ·���ϵ������⡣*/
    void get_path(const Node *node, Solution &sol) const {
        Solution temp_sol; // Ϊ�˽��ڵ�˳��ת������
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
    /* ���㵱ǰ�ڵ�ķ�����*/
    Score get_score(const Node *node) const {
        // TODO ����μ��������
    }
private:
    struct Score2Node { // ����ÿ���ڵ�ĵ�ַ�Ͷ�Ӧ���������ڿ��ٶ�λ��һ������չ�ڵ㡣
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
    size_t nb_node_;    // ��ǰ����չ�Ľڵ���Ŀ��
    TCoord start_pos_;   // 1-cut�Ŀ�ʼλ�á�
    std::priority_queue<Score2Node, std::vector<Score2Node>, Score2NodeCompare()> score_queue_; // ����չ�ڵ㰴���Ŷ����С�
};

}

#endif
