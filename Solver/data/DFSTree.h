#pragma once
#ifndef GUILLOTINE_CUT_DFSTREE_H
#define GUILLOTINE_CUT_DFSTREE_H

#include "Placement.h"

#include <vector>

namespace szx {

class DfsTree {
public:
    struct Node { /* �����������е����ڵ� */
        Depth depth;    // �ڵ����
        Placement *place;   // ����λ��

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
