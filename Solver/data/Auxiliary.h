#pragma once
#ifndef GUILLOTINE_CUT_AUXILIARY_H
#define GUILLOTINE_CUT_AUXILIARY_H

#include "../Common.h"
#include "Problem.h"
#include "../utility/Utility.h"

namespace szx {

struct Rect { /* 矩形定义 */
    Length w;
    Length h;

    Rect() {}
    Rect(Length _w, Length _h) : w(_w), h(_h) {}
};

struct Defect { /* 瑕疵定义 */
    ID id;
    Coord x;
    Coord y;
    Length w;
    Length h;

    Defect() {}
    Defect(ID _id, Coord _x, Coord _y, Length _w, Length _h) :
        id(_id), x(_x), y(_y), w(_w), h(_h) {}
};

struct IdMap {
    ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxItemNum> item;
    ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxStackNum> stack;
    ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxDefectNum> defect;
    ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxPlateNum> plate;
};

struct Auxiliary { /* 辅助计算数据 */
    List<Rect> items;                   // 物品长宽信息
    List<Area> item_area;               // 物品面积
    List<List<ID>> stacks;              // 物品在每个栈中的顺序
    List<List<Defect>> plate_defect_x;  // 每块原料上的瑕疵，按照x坐标排序
    List<List<Defect>> plate_defect_y;  // 每块原料上的瑕疵，按照y坐标排序
    Problem::Input::Param param;        // 约束参数
    IdMap idMap;                        // 输入ID和连续ID之间的映射
    int support_thread;                 // 支持线程数目
};

}

#endif
