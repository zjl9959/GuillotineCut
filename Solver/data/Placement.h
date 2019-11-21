#pragma once
#ifndef GUILLOTINE_CUT_PLACEMENT_H
#define GUILLOTINE_CUT_PLACEMENT_H

#include "../Common.h"
#include "Problem.h"

namespace szx {

#pragma region Placement
struct Placement { /* 一个物品的放置位置信息 */
public:
    TID item;        // 放置物品ID
    TCoord c1cpl;    // 一层桶左边界
    TCoord c1cpr;    // 一层桶右边界
    TCoord c2cpb;    // 二层桶下边界
    TCoord c2cpu;    // 二层桶上边界
    TCoord c3cp;     // 三层桶右边界
    TCoord c4cp;     // 四层桶上边界
    Status flag;    // 与枚举类型Flag对应

    Placement() {};

    /* 基于1-cut位置构造，一般用于起始放置位置 */
    Placement(TCoord C1cpl) : item(Problem::InvalidItemId), c1cpl(C1cpl), c1cpr(C1cpl), c2cpb(0),
        c2cpu(0), c3cp(C1cpl), c4cp(0), flag(0) {};
    
    /* 基于上一个Placement构造 */
    Placement(const Placement &node, TID item_id, Status _flag) : item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
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

/* 连接两个局部解，将右值中的节点依次添加到左值中去 */
Solution& operator+=(Solution &lhs, const Solution &rhs);

/* 将rhs中的节点依次从lhs中拿出来 */
Solution& operator-=(Solution &lhs, const Solution &rhs);

bool item_repeat(const Solution &sol);  // 检查解中是否有重复物品

TID nb_used_plate(const Solution &sol);    // 计算当前解使用了多少块原料

bool valid_plate_sol(const Solution &sol);  // 检测plate solution是否合法

void save_solution(const Solution &sol, const String &path);    // 保存解到文件中
#pragma endregion

#pragma region UsageRate
class UsageRate {
    friend bool operator< (const UsageRate &lhs, const UsageRate &rhs) { return lhs.value_ < rhs.value_; }
public:
    UsageRate() : value_(invalid_value) {}
    explicit UsageRate(double rate) : value_(rate) {}

    bool valid() const { return abs(value_ - invalid_value) > 0.001; }

    double value() { return value_; }

    String str() const {
        std::ostringstream os;
        os << value_ << "%";
        return os.str();
    }
private:
    static constexpr double invalid_value = -1.0;
    double value_;
};

#pragma endregion

}

#endif
