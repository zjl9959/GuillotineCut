#pragma once
#ifndef GUILLOTINE_CUT_PFSTREE_H
#define GUILLOTINE_CUT_PFSTREE_H

#include "Placement.h"

#include <unordered_set>
#include <cassert>

namespace szx {

class PfsTree {
    // [TODO]: test this class.
private:
    struct Node {
        Depth depth;    // �ڵ����
        TID nb_child;   // �ӽڵ�����
        Area area;  // ��ǰʹ����Ʒ�����
        Node *parent;   // ���ڵ��ַ
        Placement place;   // ����λ��

        Node() : depth(0), nb_child(0), area(0), parent(nullptr) {}
        Node(Node *_parent, Placement _place, Area _item_area) : depth(_parent->depth + 1),
            nb_child(0), area(_parent->area + _item_area), parent(_parent), place(_place) {}
    };

    class Score : public UsageRate {    /* score��usage_rate��ǰ�벿�֣���depth����벿�֣���� */
    public:
        explicit Score(double rate, Depth depth) :
            UsageRate(static_cast<int>(rate*usage_rate_base)*offset + depth) {}
        String str() const override {
            std::ostringstream os;
            os << usage_rate_int / (usage_rate_base * offset) << "."
                << (usage_rate_int / (offset)) % usage_rate_base << "% "
                << usage_rate_int % offset;
            return os.str();
        }
    private:
        static constexpr int usage_rate_base = 10;  // 10^�����ʱ�����С��λ��
        static constexpr int offset = 1000;  // 10^Ϊdepth�����λ��
    };
public:
    PfsTree(TCoord start_pos, Auxiliary aux) : start_pos_(start_pos), aux_(aux) {}
    ~PfsTree() {
        for(auto it = leaf_nodes_.begin(); it != leaf_nodes_.end(); ++it) {
            delete_node_recursive(it->second);
        }
        assert(0 == nb_node_);
    }

    /* �ж������Ƿ���δ����չ�Ľڵ㡣 */
    bool empty() const {
        return leaf_nodes_.empty();
    }

    /* ��ȡ��һ������չ�ڵ㡣*/
    const Node* get() {
        Node *pnode = leaf_nodes_.begin()->second;
        leaf_nodes_.erase(leaf_nodes_.begin());
        return pnode;
    }

    /* ����½ڵ㡣*/
    void add(Node *parent, Placement &place) {
        Node *node = new Node(parent, place, aux_.item_area[place.item]);
        leaf_nodes_.insert(std::make_pair(get_score(node), node));
        ++nb_node_;
        ++(node->parent->nb_child);
        if (nb_node_ > max_nb_node) {
            adjust_memory();
        }
    }

    /* ��ȡ��ǰ�⵽������·�� */
    void get_tree_path(Node *node, Solution &sol) const {
        while (node != nullptr) {
            sol.push_back(node->place);
            node = node->parent;
        }
        std::reverse(sol.begin(), sol.end());
    }
private:
    /* ���㵱ǰ�ڵ�ķ�����*/
    Score get_score(const Node *node) const {
        Area used_area =
            (node->place.c1cpl - start_pos_)*aux_.param.plateHeight +
            node->place.c2cpb*(node->place.c1cpr - node->place.c1cpl) +
            (node->place.c3cp - node->place.c1cpl)*(node->place.c2cpu - node->place.c2cpb);
        assert(used_area > 0);
        return Score(static_cast<double>(node->area) / static_cast<double>(used_area), node->depth);
    }

    /* �ڵ���Ŀ�����������ʱ���øú���ɾ�����ֽϲ�Ľڵ㡣*/
    void adjust_memory() {
        size_t new_nb_node = static_cast<double>(nb_node_) * memory_adjust_rate;
        auto it = leaf_nodes_.rbegin();
        while (it != leaf_nodes_.rend() && nb_node_ > new_nb_node) {
            delete_node_recursive(it->second);
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
    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // ���������������ڵ���Ŀ��
    static constexpr double memory_adjust_rate = 0.70;   // ���ڵ�������ʱ���½ڵ���Ŀռ�ֽڵ���Ŀ�İٷֱȡ�

    Auxiliary &aux_;    // ������Ϣ��
    size_t nb_node_ = 0;    // ��ǰ����չ�Ľڵ���Ŀ��
    TCoord start_pos_;   // 1-cut�Ŀ�ʼλ�á�
    std::map<Score, Node*> leaf_nodes_;  // ����Score�Ŷ����еĴ���չ�ڵ㡣
};

}

#endif
