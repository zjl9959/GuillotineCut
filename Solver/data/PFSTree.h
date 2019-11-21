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
        Depth depth;    // 节点深度
        TID nb_child;   // 子节点数量
        Area area;  // 当前使用物品的面积
        Node *parent;   // 父节点地址
        Placement place;   // 放置位置

        /* 用于创建第一层节点 */
        Node(Placement &_place, Area _item_area) : depth(0),
            nb_child(0), area(_item_area), parent(nullptr), place(_place) {}
        /* 用于创建后续节点 */
        Node(Node *_parent, Placement &_place, Area _item_area) : depth(_parent->depth + 1),
            nb_child(0), area(_parent->area + _item_area), parent(_parent), place(_place) {}
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
        static constexpr double tolerance_ = 0.01;  // 分数值在一定差别内视为相等。
        double rate_;   // 利用率值（加权后）。
        Depth depth_;   // 当前节点扩展深度。
    };
    struct ScoreCmp {
        bool operator() (const Score &lhs, const Score &rhs) {
            return lhs < rhs;
        }
    };
public:
    PfsTree(const List<Area> &item_area) : item_area_(item_area) {}
    ~PfsTree() {
        /* 删除剩余的待扩展节点 */
        for(auto it = live_nodes_.begin(); it != live_nodes_.end(); ++it) {
            delete_node_recursive(it->second);
        }
        /* 删除已经被完全扩展的叶子节点 */
        for (auto it = leaf_nodes_.begin(); it != leaf_nodes_.end(); ++it) {
            delete_node_recursive(*it);
        }
        if (nb_node_ > 0) {
            std::cout << "PfsTree not delete node number:" << nb_node_ << std::endl;
        }
    }

    /* 判断树中是否有未被扩展的节点。 */
    bool empty() const {
        return live_nodes_.empty();
    }

    /* 获取下一个待扩展节点。*/
    Node* get() {
        Node *pnode = live_nodes_.begin()->second;
        live_nodes_.erase(live_nodes_.begin());
        return pnode;
    }

    /* 添加新节点。*/
    void add(Node *parent, Placement &place, double score) {
        Node *node = new Node(parent, place, item_area_[place.item]);
        live_nodes_.insert(std::make_pair(Score(score, node->depth + 1), node));
        ++nb_node_;
        ++(node->parent->nb_child);
        if (nb_node_ > max_nb_node) {
            adjust_memory();
        }
    }
    void add(Placement &place, double score) {
        Node *node = new Node(place, item_area_[place.item]);
        live_nodes_.insert(std::make_pair(Score(score, node->depth + 1), node));
        ++nb_node_;
    }

    /* 添加已被完全扩展的叶子节点 */
    void add_leaf_node(Node *node) {
        leaf_nodes_.push_back(node);
    }

    /* 
    * 在搜索过程中更新batch
    * 输入：last_node（上一次扩展的节点），cur_node（当前待扩展的节点），batch（物品栈）
    */
    void update_batch(Node *last_node, Node *cur_node, Batch &batch) {
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

    /* 获取当前解到树根的路径 */
    void get_tree_path(Node *node, Solution &sol) const {
        sol.clear();
        while (node != nullptr) {
            sol.push_back(node->place);
            node = node->parent;
        }
        std::reverse(sol.begin(), sol.end());
    }
private:
    /* 节点数目超过最大限制时调用该函数删除部分较差的节点。*/
    void adjust_memory() {
        // [zjl][TODO]: add implement.
        throw "not implement.";
    }

    /* 从node节点开始，沿路径依次删除叶子节点。 */
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
    static constexpr size_t max_nb_node = (1 << 30) / sizeof(Node); // 限制搜索树的最大节点数目。
    static constexpr double memory_adjust_rate = 0.70;   // 当节点数过多时，新节点数目占现节点数目的百分比。

    const List<Area> &item_area_; // 每个物品的面积大小
    size_t nb_node_ = 0;    // 当前已扩展的节点数目。
    std::multimap<Score, Node*, ScoreCmp> live_nodes_;  // 按照Score优度排列的待扩展节点。
    List<Node*> leaf_nodes_;    // 已经被完全扩展的节点。
};

}

#endif
