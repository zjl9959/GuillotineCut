#pragma once
#ifndef GUILLOTINE_CUT_FACTORY_H
#define GUILLOTINE_CUT_FACTORY_H

#include "Solver/data/header/Problem.h"
#include "Solver/data/header/Environment.h"

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

struct Statistics {
    int L1_count;
    double L1_max;
    double L1_min;
    double L1_total;

    int plate_count;
    double plate_max;
    double plate_min;
    double plate_total;

    void add_L1(double usage_rate);
    void add_plate(double usage_rate);
    void reset();                   // 复位统计数据。
    String toBriefStr() const;         // 格式化用于csv日志格式的字符串。
};

void init_global_variables(const Problem::Input &input, const Environment &env);

namespace gv {
extern Configuration cfg;                  // 配置参数
extern Random rand;                        // 随机种子发生器
extern Timer timer;                        // 计时器
extern List<Rect> items;                   // 物品长宽信息
extern List<Area> item_area;               // 物品面积
extern List<List<TID>> stacks;             // 物品在每个栈中的顺序
extern List<List<Defect>> defect_x;        // 每块原料上的瑕疵，按照x坐标排序
extern List<List<Defect>> defect_y;        // 每块原料上的瑕疵，按照y坐标排序
extern Problem::Input::Param param;        // 约束参数
extern IdMap idMap;                        // 输入TID和连续TID之间的映射
extern int support_thread;                 // 支持线程数目
extern Statistics info;
}

}

#endif // !GUILLOTINE_CUT_FACTORY_H
