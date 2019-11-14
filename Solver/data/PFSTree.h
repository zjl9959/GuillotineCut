#pragma once
#ifndef GUILLOTINE_CUT_PFSTREE_H
#define GUILLOTINE_CUT_PFSTREE_H

#include "Placement.h"

#include <map>
#include <cassert>

namespace szx {

class PfsTree {
public:
    struct Node {
        Depth depth;    // �ڵ����
        TID nb_child;   // �ӽڵ�����
        Area area;  // ��ǰʹ����Ʒ�����
        Node *parent;   // ���ڵ��ַ
        Placement place;   // ����λ��

        /* ���ڴ�����һ��ڵ� */
        Node(Placement &_place, Area _item_area) : depth(0),
            nb_child(0), area(_item_area), parent(nullptr), place(_place) {}
        /* ���ڴ��������ڵ� */
        Node(Node *_parent, Placement &_place, Area _item_area) : depth(_parent->depth + 1),
            nb_child(0), area(_parent->area + _item_area), parent(_parent), place(_place) {}
    };
private:
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
    struct ScoreCmp {
        bool operator() (const Score &lhs, const Score &rhs) const {
            return rhs < lhs;
        }
    };
public:
    PfsTree(TCoord start_pos, TLength plate_height, const List<Area> &item_area) :
        start_pos_(start_pos), item_area_(item_area), plate_height_(plate_height) {}
    ~PfsTree() {
        /* ɾ��ʣ��Ĵ���չ�ڵ� */
        for(auto it = live_nodes_.begin(); it != live_nodes_.end(); ++it) {
            delete_node_recursive(it->second);
        }
        /* ɾ���Ѿ�����ȫ��չ��Ҷ�ӽڵ� */
        for (auto it = leaf_nodes_.begin(); it != leaf_nodes_.end(); ++it) {
            delete_node_recursive(*it);
        }
        if (nb_node_ > 0)
            throw "memory leak!";
    }

    /* �ж������Ƿ���δ����չ�Ľڵ㡣 */
    bool empty() const {
        return live_nodes_.empty();
    }

    /* ��ȡ��һ������չ�ڵ㡣*/
    Node* get() {
        Node *pnode = live_nodes_.begin()->second;
        live_nodes_.erase(live_nodes_.begin());
        return pnode;
    }

    /* ����½ڵ㡣*/
    void add(Node *parent, Placement &place) {
        Node *node = new Node(parent, place, item_area_[place.item]);
        live_nodes_.insert(std::make_pair(get_score(node), node));
        ++nb_node_;
        ++(node->parent->nb_child);
        if (nb_node_ > max_nb_node) {
            adjust_memory();
        }
    }
    void add(Placement &place) {
        Node *node = new Node(place, item_area_[place.item]);
        live_nodes_.insert(std::make_pair(get_score(node), node));
        ++nb_node_;
    }

    /* ����ѱ���ȫ��չ��Ҷ�ӽڵ� */
    void add_leaf_node(Node *node) {
        leaf_nodes_.push_back(node);
    }

    /* �����������и���batch
       ���룺last_node����һ����չ�Ľڵ㣩��cur_node����ǰ����չ�Ľڵ㣩��batch����Ʒջ��
    */
    void update_batch(Node *last_node, Node *cur_node, Batch &batch) {
        List<TID> item_cache;   // �������batch���ó�����Ʒ��
        item_cache.reserve(10);
        // ��֤last_node��cur_node�������ͬ��
        if (last_node == nullptr) {
            batch.remove(cur_node->place.item);
            return;
        } else if (last_node->depth > cur_node->depth) {
            while (last_node->depth != cur_node->depth) {
                batch.add(last_node->place.item);
                last_node = last_node->parent;
            }
        } else if (last_node->depth < cur_node->depth) {
            while (cur_node->depth != last_node->depth) {
                item_cache.push_back(cur_node->place.item);
                cur_node = cur_node->parent;
            }
        }
        // ���ϻ���ֱ��last_node��cur_node�Ĺ������ڵ㡣
        while (last_node != cur_node) {
            assert(last_node != nullptr && cur_node != nullptr);
            batch.add(last_node->place.item);
            item_cache.push_back(cur_node->place.item);
            last_node = last_node->parent;
            cur_node = cur_node->parent;
        }
        // ��item_cache�е���Ʒ�ó�batch��
        for (auto it = item_cache.rbegin(); it != item_cache.rend(); ++it) {
            batch.remove(*it);
        }
    }

    /* ��ȡ��ǰ�⵽������·�� */
    void get_tree_path(Node *node, Solution &sol) const {
        while (node != nullptr) {
            sol.push_back(node->place);
            node = node->parent;
        }
    }
private:
    /* ���㵱ǰ�ڵ�ķ�����*/
    Score get_score(const Node *node) const {
        Area used_area =
            (node->place.c1cpl - start_pos_)*plate_height_ +
            node->place.c2cpb*(node->place.c1cpr - node->place.c1cpl) +
            (node->place.c3cp - node->place.c1cpl)*(node->place.c2cpu - node->place.c2cpb);
        assert(used_area > 0);
        return Score(static_cast<double>(node->area) / static_cast<double>(used_area), node->depth);
    }

    /* �ڵ���Ŀ�����������ʱ���øú���ɾ�����ֽϲ�Ľڵ㡣*/
    void adjust_memory() {
        /*
        size_t new_nb_node = static_cast<double>(nb_node_) * memory_adjust_rate;
        auto it = live_nodes_.rbegin();
        while (it != live_nodes_.rend() && nb_node_ > new_nb_node) {
            delete_node_recursive(it->second);
        }
        */
    }

    /* ��node�ڵ㿪ʼ����·������ɾ��Ҷ�ӽڵ㡣 */
    void delete_node_recursive(Node *node) {
        while (node != nullptr && node->nb_child == 0) {
            Node *parent = node->parent;
            delete node;
            --nb_node_;
            if (parent == nullptr)break;
            --(parent->nb_child);
            node = parent;
        }
    }
private:
    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // ���������������ڵ���Ŀ��
    static constexpr double memory_adjust_rate = 0.70;   // ���ڵ�������ʱ���½ڵ���Ŀռ�ֽڵ���Ŀ�İٷֱȡ�

    const List<Area> &item_area_; // ÿ����Ʒ�������С
    const TCoord start_pos_;   // 1-cut�Ŀ�ʼλ�á�
    const TLength plate_height_;    // ԭ�ϸ߶ȡ�
    size_t nb_node_ = 0;    // ��ǰ����չ�Ľڵ���Ŀ��
    std::multimap<Score, Node*, ScoreCmp> live_nodes_;  // ����Score�Ŷ����еĴ���չ�ڵ㡣
    List<Node*> leaf_nodes_;    // �Ѿ�����ȫ��չ�Ľڵ㡣
};

}

#endif
