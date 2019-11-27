#include "Solver/data/header/PFSTree.h"

#include <cassert>
#include <iostream>

#include "Solver/data/header/Global.h"

using namespace std;

namespace szx {

PfsTree::~PfsTree() {
    // 删除剩余的待扩展节点。
    for (auto it = live_nodes_.begin(); it != live_nodes_.end(); ++it) {
        delete_node_recursive(it->second);
    }
    // 删除已经被完全扩展的叶子节点。
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
    List<TID> item_cache;   // 缓存待从batch中拿出的物品。
    item_cache.reserve(10);
    // 保证last_node和cur_node的深度相同。
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
    // 向上回溯直到last_node和cur_node的公共父节点。
    while (last_node != cur_node) {
        assert(last_node != nullptr && cur_node != nullptr);
        batch.add(last_node->place.item);
        item_cache.push_back(cur_node->place.item);
        last_node = last_node->parent;
        cur_node = cur_node->parent;
    }
    // 将item_cache中的物品拿出batch。
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
* 节点数目超过最大限制时调用该函数删除部分较差的节点。
*/
void PfsTree::adjust_memory() {
    // [zjl][TODO]: add implement.
    throw "not implement.";
}

/*
* 从node节点开始，沿路径依次删除叶子节点。
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
