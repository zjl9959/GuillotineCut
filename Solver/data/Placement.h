#pragma once
#ifndef GUILLOTINE_CUT_PLACEMENT_H
#define GUILLOTINE_CUT_PLACEMENT_H

#include "../Common.h"
#include "Problem.h"

namespace szx {

struct Placement { /* 一个物品的放置位置信息 */
public:
    ID item;        // 放置物品ID
    Coord c1cpl;    // 一层桶左边界
    Coord c1cpr;    // 一层桶右边界
    Coord c2cpb;    // 二层桶下边界
    Coord c2cpu;    // 二层桶上边界
    Coord c3cp;     // 三层桶右边界
    Coord c4cp;     // 四层桶上边界
    Status flag;    // 与枚举类型Flag对应

    Placement() {};

    /* 基于1-cut位置构造，一般用于起始放置位置 */
    Placement(Coord C1cpl) : item(Problem::InvalidItemId), c1cpl(C1cpl), c1cpr(C1cpl), c2cpb(0),
        c2cpu(0), c3cp(C1cpl), c4cp(0), flag(0) {};
    
    /* 基于上一个Placement构造 */
    Placement(const Placement &node, ID item_id, Status _flag) : item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
        c2cpb(node.c2cpb), c2cpu(node.c2cpu), c3cp(node.c3cp), c4cp(node.c4cp), flag(_flag) {};

    void setFlagBit(const int bit_flag = ROTATE) { flag |= bit_flag; }
    const bool getFlagBit(const int bit_flag = ROTATE) const { return flag & bit_flag; }
public:
    enum Flag { /* 指示flag变量中每一位的含义*/
        ROTATE = 0x0001,     // 旋转物品
        DEFECT_R = 0x0002,   // 物品位于瑕疵右侧
        DEFECT_U = 0x0004,   // 物品位于瑕疵上侧
        BIN4 = 0x0008,       // 物品位于一个四层桶中
        LOCKC2 = 0x0010,     // 当前二层桶的上边界不能移动
        NEW_PLATE = 0x0020,  // 新开一块原料
        NEW_L1 = 0x0040,     // 新开一个一层桶
        NEW_L2 = 0x0080,     // 新开一个二层桶
    };
};

using Solution = List<Placement>;   //代表部分解或完全解

}

#endif
