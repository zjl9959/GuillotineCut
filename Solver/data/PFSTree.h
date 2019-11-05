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

    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // ���������������ڵ���Ŀ��
    static constexpr double memory_adjust_rate = 0.70;   // ���ڵ�������ʱ���½ڵ���Ŀռ�ֽڵ���Ŀ�İٷֱȡ�
private:
    class Score : public UsageRate {
        // TODO: add implement.
    };
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
public:
    PfsTree(TCoord start_pos) : nb_node_(0), start_pos_(start_pos) {}
    ~PfsTree() {
        for(auto it = score_queue_.begin(); it != score_queue_.end(); ++it) {
            delete_node_recursive(it->node);
        }
        assert(0 == nb_node_);
    }

    /* ��ȡ��һ������չ�ڵ㡣*/
    const Node* get() {
        Node *pnode = score_queue_.begin()->node;
        score_queue_.erase(score_queue_.begin());
        assert(pnode != nullptr);
        return pnode;
    }

    /* ����½ڵ㡣*/
    void add(Node *node) {
        score_queue_.insert(Score2Node(get_score(node), node));
        ++nb_node_;
        ++(node->parent->nb_child);
        if (nb_node_ > max_nb_node) {
            adjust_memory();
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

    /* �ڵ���Ŀ�����������ʱ���øú���ɾ�����ֽϲ�Ľڵ㡣*/
    void adjust_memory() {
        size_t new_nb_node = static_cast<double>(nb_node_) * memory_adjust_rate;
        auto it = score_queue_.rbegin();
        while (it != score_queue_.rend() && nb_node_ > new_nb_node) {
            delete_node_recursive(it->node);
        }
    }

    /* ��node�ڵ㿪ʼ����·������ɾ��Ҷ�ӽڵ㡣 */
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
    size_t nb_node_;    // ��ǰ����չ�Ľڵ���Ŀ��
    TCoord start_pos_;   // 1-cut�Ŀ�ʼλ�á�
    std::set<Score2Node, Score2NodeCompare> score_queue_;   // ����չ�ڵ㰴���Ŷ����С�
};

}

#endif
