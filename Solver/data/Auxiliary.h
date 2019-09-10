#pragma once
#ifndef GUILLOTINE_CUT_AUXILIARY_H
#define GUILLOTINE_CUT_AUXILIARY_H

#include "../Common.h"
#include "Problem.h"
#include "../utility/Utility.h"

namespace szx {

struct Rect { /* 矩形定义 */
    TLength w;
    TLength h;

    Rect() {}
    Rect(TLength _w, TLength _h) : w(_w), h(_h) {}
};

struct Defect { /* 瑕疵定义 */
    TID id;
    TCoord x;
    TCoord y;
    TLength w;
    TLength h;

    Defect() {}
    Defect(TID _id, TCoord _x, TCoord _y, TLength _w, TLength _h) :
        id(_id), x(_x), y(_y), w(_w), h(_h) {}
};

struct IdMap {
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxItemNum> item;
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxStackNum> stack;
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxDefectNum> defect;
    ZeroBasedConsecutiveIdMap<TID, TID, Problem::MaxPlateNum> plate;
};

struct Auxiliary { /* 辅助计算数据 */
    List<Rect> items;                   // 物品长宽信息
    List<Area> item_area;               // 物品面积
    List<List<TID>> stacks;              // 物品在每个栈中的顺序
    List<List<Defect>> plate_defect_x;  // 每块原料上的瑕疵，按照x坐标排序
    List<List<Defect>> plate_defect_y;  // 每块原料上的瑕疵，按照y坐标排序
    Problem::Input::Param param;        // 约束参数
    IdMap idMap;                        // 输入TID和连续TID之间的映射
    int support_thread;                 // 支持线程数目
};

}

#endif
