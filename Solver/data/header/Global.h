#pragma once
#ifndef GUILLOTINE_CUT_FACTORY_H
#define GUILLOTINE_CUT_FACTORY_H

#include "Solver/data/header/Problem.h"
#include "Solver/data/header/Environment.h"

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
    void reset();                   // ��λͳ�����ݡ�
    String toBriefStr() const;         // ��ʽ������csv��־��ʽ���ַ�����
};

void init_global_variables(const Problem::Input &input, const Environment &env);

namespace gv {
extern Configuration cfg;                  // ���ò���
extern Random rand;                        // ������ӷ�����
extern Timer timer;                        // ��ʱ��
extern List<Rect> items;                   // ��Ʒ������Ϣ
extern List<Area> item_area;               // ��Ʒ���
extern List<List<TID>> stacks;             // ��Ʒ��ÿ��ջ�е�˳��
extern List<List<Defect>> defect_x;        // ÿ��ԭ���ϵ�覴ã�����x��������
extern List<List<Defect>> defect_y;        // ÿ��ԭ���ϵ�覴ã�����y��������
extern Problem::Input::Param param;        // Լ������
extern IdMap idMap;                        // ����TID������TID֮���ӳ��
extern int support_thread;                 // ֧���߳���Ŀ
extern Statistics info;
}

}

#endif // !GUILLOTINE_CUT_FACTORY_H
