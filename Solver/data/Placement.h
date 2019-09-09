#pragma once
#ifndef GUILLOTINE_CUT_PLACEMENT_H
#define GUILLOTINE_CUT_PLACEMENT_H

#include "../Common.h"
#include "Problem.h"

namespace szx {

struct Placement { /* һ����Ʒ�ķ���λ����Ϣ */
public:
    ID item;        // ������ƷID
    Coord c1cpl;    // һ��Ͱ��߽�
    Coord c1cpr;    // һ��Ͱ�ұ߽�
    Coord c2cpb;    // ����Ͱ�±߽�
    Coord c2cpu;    // ����Ͱ�ϱ߽�
    Coord c3cp;     // ����Ͱ�ұ߽�
    Coord c4cp;     // �Ĳ�Ͱ�ϱ߽�
    Status flag;    // ��ö������Flag��Ӧ

    Placement() {};

    /* ����1-cutλ�ù��죬һ��������ʼ����λ�� */
    Placement(Coord C1cpl) : item(Problem::InvalidItemId), c1cpl(C1cpl), c1cpr(C1cpl), c2cpb(0),
        c2cpu(0), c3cp(C1cpl), c4cp(0), flag(0) {};
    
    /* ������һ��Placement���� */
    Placement(const Placement &node, ID item_id, Status _flag) : item(item_id), c1cpl(node.c1cpl), c1cpr(node.c1cpr),
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

using Solution = List<Placement>;   //�������ֽ����ȫ��

/* ���������ֲ��⣬����ֵ�еĽڵ��������ӵ���ֵ��ȥ */
Solution& operator+=(Solution &lhs, Solution &rhs) {
    for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        lhs.push_back(*it);
    }
    return lhs;
}

}

#endif