#pragma once
#ifndef GUILLOTINE_CUT_PLACEMENT_H
#define GUILLOTINE_CUT_PLACEMENT_H

#include "../Common.h"
#include "Problem.h"

namespace szx {

#pragma region Placement
struct Placement { /* һ����Ʒ�ķ���λ����Ϣ */
public:
    TID item;        // ������ƷID
    TCoord c1cpl;    // һ��Ͱ��߽�
    TCoord c1cpr;    // һ��Ͱ�ұ߽�
    TCoord c2cpb;    // ����Ͱ�±߽�
    TCoord c2cpu;    // ����Ͱ�ϱ߽�
    TCoord c3cp;     // ����Ͱ�ұ߽�
    TCoord c4cp;     // �Ĳ�Ͱ�ϱ߽�
    Status flag;    // ��ö������Flag��Ӧ

    Placement() {};

    /* ����1-cutλ�ù��죬һ��������ʼ����λ�� */
    Placement(TCoord C1cpl) : item(Problem::InvalidItemId), c1cpl(C1cpl), c1cpr(C1cpl), c2cpb(0),
        c2cpu(0), c3cp(C1cpl), c4cp(0), flag(0) {};
    
    /* ������һ��Placement���� */
    Placement(const Placement &node, TID item_id, Status _flag) : item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
        c2cpb(node.c2cpb), c2cpu(node.c2cpu), c3cp(node.c3cp), c4cp(node.c4cp), flag(_flag) {};

    void setFlagBit(const int bit_flag = ROTATE) { flag |= bit_flag; }
    const bool getFlagBit(const int bit_flag = ROTATE) const { return flag & bit_flag; }
public:
    enum Flag { /* ָʾflag������ÿһλ�ĺ���*/
        ROTATE = 0x0001,     // ��ת��Ʒ
        DEFECT_R = 0x0002,   // ��Ʒλ��覴��Ҳ�
        DEFECT_U = 0x0004,   // ��Ʒλ��覴��ϲ�
        BIN4 = 0x0008,       // ��Ʒλ��һ���Ĳ�Ͱ��
        LOCKC2 = 0x0010,     // ��ǰ����Ͱ���ϱ߽粻���ƶ�
        NEW_PLATE = 0x0020,  // �¿�һ��ԭ��
        NEW_L1 = 0x0040,     // �¿�һ��һ��Ͱ
        NEW_L2 = 0x0080,     // �¿�һ������Ͱ
    };
};

using Solution = List<Placement>;   //�����ֽ����ȫ��

/* ���������ֲ��⣬����ֵ�еĽڵ�������ӵ���ֵ��ȥ */
Solution& operator+=(Solution &lhs, const Solution &rhs);

/* ��rhs�еĽڵ����δ�lhs���ó��� */
Solution& operator-=(Solution &lhs, const Solution &rhs);

bool item_repeat(const Solution &sol);  // �������Ƿ����ظ���Ʒ

TID nb_used_plate(const Solution &sol);    // ���㵱ǰ��ʹ���˶��ٿ�ԭ��

bool valid_plate_sol(const Solution &sol);  // ���plate solution�Ƿ�Ϸ�

void save_solution(const Solution &sol, const String &path);    // ����⵽�ļ���
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
