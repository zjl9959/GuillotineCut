#include "Solver/data/header/PFSTree.h"

#include <cassert>
#include <iostream>

#include "Solver/data/header/Global.h"

using namespace std;

namespace szx {

PfsTree::~PfsTree() {
    // ɾ��ʣ��Ĵ���չ�ڵ㡣
    for (auto it = live_nodes_.begin(); it != live_nodes_.end(); ++it) {
        delete_node_recursive(it->second);
    }
    // ɾ���Ѿ�����ȫ��չ��Ҷ�ӽڵ㡣
    for (auto it = leaf_nodes_.begin(); it != leaf_nodes_.end(); ++it) {
        delete_node_recursive(*it);
    }
    if (nb_node_ > 0) {
        cout << "PfsTree not delete node number:" << nb_node_ << endl;
    }
}

PfsTree::Node* PfsTree::get() {
    Node *pnode = live_nodes_.begin()->second;
    live_nodes_.erase(live_nodes_.begin());
    return pnode;
}

void PfsTree::add(Node *parent, Placement &place, double score) {
    Node *node = new Node(parent, place, gv::item_area[place.item]);
    live_nodes_.insert(std::make_pair(Score(score, node->depth + 1), node));
    ++nb_node_;
    ++(node->parent->nb_child);
    if (nb_node_ > max_nb_node) {
        adjust_memory();
    }
}

void PfsTree::add(Placement &place, double score) {
    Node *node = new Node(place, gv::item_area[place.item]);
    live_nodes_.insert(std::make_pair(Score(score, node->depth + 1), node));
    ++nb_node_;
}

void PfsTree::update_batch(Node *last_node, Node *cur_node, Batch &batch) {
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

void PfsTree::get_tree_path(Node *node, Solution &sol) const {
    sol.clear();
    while (node != nullptr) {
        sol.push_back(node->place);
        node = node->parent;
    }
    std::reverse(sol.begin(), sol.end());
}

/*
* �ڵ���Ŀ�����������ʱ���øú���ɾ�����ֽϲ�Ľڵ㡣
*/
void PfsTree::adjust_memory() {
    // [zjl][TODO]: add implement.
    throw "not implement.";
}

/*
* ��node�ڵ㿪ʼ����·������ɾ��Ҷ�ӽڵ㡣
*/
void PfsTree::delete_node_recursive(Node *node) {
    while (node != nullptr && node->nb_child == 0) {
        Node *parent = node->parent;
        delete node;
        --nb_node_;
        if (parent == nullptr)break;
        --(parent->nb_child);
        node = parent;
    }
}

}
