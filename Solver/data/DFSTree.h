#pragma once
#ifndef GUILLOTINE_CUT_DFSTREE_H
#define GUILLOTINE_CUT_DFSTREE_H

#include "Placement.h"

#include <vector>

namespace szx {

class DfsTree {
public:
    struct Node { /* 定义树搜索中的树节点 */
        Depth depth;    // 节点深度
        Placement *place;   // 放置位置

        Node() : depth(0), place(nullptr) {}
        Node(Node *parent, Placement *_place) : depth(parent->depth + 1), place(_place) {}
    };
public:
    // TODO: add code.
private:
    std::vector<Placement*> live_nodes;
};

}

#endif // !GUILLOTINE_CUT_DFSTREE_H
