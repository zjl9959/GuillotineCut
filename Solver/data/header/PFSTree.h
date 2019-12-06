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
        Depth depth;    // �ڵ����
        TID nb_child;   // �ӽڵ�����
        Node *parent;   // ���ڵ��ַ
        Placement place;   // ����λ��

        /* ���ڴ�����һ��ڵ� */
        Node(Placement &_place, Area _item_area) : depth(0),
            nb_child(0), parent(nullptr), place(_place) {}
        /* ���ڴ��������ڵ� */
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
        static constexpr double tolerance_ = 0.01;  // ����ֵ��һ���������Ϊ��ȡ�
        double rate_;   // ������ֵ����Ȩ�󣩡�
        Depth depth_;   // ��ǰ�ڵ���չ��ȡ�
    };
    struct ScoreCmp {
        bool operator() (const Score &lhs, const Score &rhs) {
            return lhs < rhs;
        }
    };
public:
    ~PfsTree();
    bool empty() const { return live_nodes_.empty(); }  // �ж������Ƿ���δ����չ�Ľڵ㡣
    Node* get();    // ��ȡ��һ������չ�ڵ㡣
    void add(Node *parent, Placement &place, double score); // ����½ڵ㡣
    void add(Placement &place, double score);   // ����½ڵ㣨��һ���ڵ㣩��
    void add_leaf_node(Node *node) { leaf_nodes_.push_back(node); } // ����ѱ���ȫ��չ��Ҷ�ӽڵ㡣
    // �����������и���batch�����룺last_node����һ����չ�Ľڵ㣩��cur_node����ǰ����չ�Ľڵ㣩��batch����Ʒջ��
    void update_batch(Node *last_node, Node *cur_node, Batch &batch);
    void get_tree_path(Node *node, Solution &sol) const;        // ��ȡ��ǰ�⵽������·����
private:
    void adjust_memory();
    void delete_node_recursive(Node *node);
private:
    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // ���������������ڵ���Ŀ��
    static constexpr double memory_adjust_rate = 0.70;   // ���ڵ�������ʱ���½ڵ���Ŀռ�ֽڵ���Ŀ�İٷֱȡ�

    size_t nb_node_ = 0;    // ��ǰ����չ�Ľڵ���Ŀ��
    std::multimap<Score, Node*, ScoreCmp> live_nodes_;  // ����Score�Ŷ����еĴ���չ�ڵ㡣
    List<Node*> leaf_nodes_;    // �Ѿ�����ȫ��չ�Ľڵ㡣
};

}

#endif
