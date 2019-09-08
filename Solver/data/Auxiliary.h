#pragma once
#ifndef GUILLOTINE_CUT_AUXILIARY_H
#define GUILLOTINE_CUT_AUXILIARY_H

#include "../Common.h"
#include "Problem.h"
#include "../utility/Utility.h"

namespace szx {

struct Rect { /* ���ζ��� */
    Length w;
    Length h;

    Rect() {}
    Rect(Length _w, Length _h) : w(_w), h(_h) {}
};

struct Defect { /* 覴ö��� */
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

struct Auxiliary { /* ������������ */
    List<Rect> items;                   // ��Ʒ������Ϣ
    List<Area> item_area;               // ��Ʒ���
    List<List<ID>> stacks;              // ��Ʒ��ÿ��ջ�е�˳��
    List<List<Defect>> plate_defect_x;  // ÿ��ԭ���ϵ�覴ã�����x��������
    List<List<Defect>> plate_defect_y;  // ÿ��ԭ���ϵ�覴ã�����y��������
    Problem::Input::Param param;        // Լ������
    IdMap idMap;                        // ����ID������ID֮���ӳ��
    int support_thread;                 // ֧���߳���Ŀ
};

}

#endif
