#pragma once
#ifndef GUILLOTINE_CUT_AUXILIARY_H
#define GUILLOTINE_CUT_AUXILIARY_H

#include "../Common.h"
#include "Problem.h"
#include "../utility/Utility.h"

namespace szx {

struct Rect { /* ���ζ��� */
    TLength w;
    TLength h;

    Rect() {}
    Rect(TLength _w, TLength _h) : w(_w), h(_h) {}
};

struct Defect { /* 覴ö��� */
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

struct Auxiliary { /* ������������ */
    List<Rect> items;                   // ��Ʒ������Ϣ
    List<Area> item_area;               // ��Ʒ���
    List<List<TID>> stacks;              // ��Ʒ��ÿ��ջ�е�˳��
    List<List<Defect>> plate_defect_x;  // ÿ��ԭ���ϵ�覴ã�����x��������
    List<List<Defect>> plate_defect_y;  // ÿ��ԭ���ϵ�覴ã�����y��������
    Problem::Input::Param param;        // Լ������
    IdMap idMap;                        // ����TID������TID֮���ӳ��
    int support_thread;                 // ֧���߳���Ŀ
};

}

#endif
