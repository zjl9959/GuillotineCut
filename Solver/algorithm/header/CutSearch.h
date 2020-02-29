#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "Solver/data/header/Batch.h"

#include <mutex>

namespace szx {

// 0 : CutSearch�в��ϸ����м���ļ�
// 1 ��PlateSearch�в��ϸ����м���ļ�
#define UPDATE_MIDDLE_SOL 2

class CutSearch {
public:
    CutSearch(TID plate, TCoord start_pos, bool opt_tail);
    
    void run(Batch &batch);
    UsageRate best_obj() const
    {
        return best_obj_;
    }
    void get_best_sol(Solution &sol) const
    {
        sol = best_sol_;
    }
private:
    void beam_search(Batch &batch);
    void dfs(Batch &batch);
    void pfs(Batch &batch);
    UsageRate greedy(Batch &batch, const Solution &fix_sol);
    void branch(const Placement &old, const Batch &batch, List<Placement> &branch_nodes);
    bool constraintCheck(const Placement &old, Placement &node);
    TCoord sliptoDefectRight(TCoord x, TCoord y, TLength w, TLength h) const;
    TCoord sliptoDefectUp(TCoord x, TCoord y, TLength w) const;
    TCoord cut1ThroughDefect(TCoord x) const;
    TCoord cut2ThroughDefect(TCoord y) const;
    Area envelope_area(const Placement &place) const;
    void update_best_sol(UsageRate obj, const Solution &sol);
private:
    const TID plate_;                       // 1-cutλ��ԭ�ϵ�id��
    const Area tail_area_;                  // ʣ�����
    const TCoord start_pos_;                // 1-cut��ʼλ�á�
    const bool opt_tail_;                   // �Ƿ��Ż����һ��1-cut��

    UsageRate best_obj_;                    // ����Ŀ�꺯��ֵ��
    Solution best_sol_;                     // ���Ž⡣
};

void update_middle_solution(TID pid, const Solution &sol);

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
