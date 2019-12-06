#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "Solver/data/header/Batch.h"

#include <mutex>

namespace szx {

class CutSearch {
public:
    enum BRANCH_MODE {
        CUT,    // ֻ�ڸ�1-cut�ڷ�֧��
        PLATE,  // ��֧ʱ���Կ��µ�1-cut�������ܿ��µ�plate��
    };
public:
    CutSearch(TID plate, TCoord start_pos, size_t nb_sol_cache, const BRANCH_MODE mode);
    
    void run(Batch &batch);
    UsageRate best_obj() const;                         // �������Ž��Ŀ�꺯��ֵ��
    void get_best_sol(Solution &sol) const;             // ��ȡ���Ž⡣
    void get_good_sols(List<Solution> &sols) const;     // ��ȡ�ϺõĽ⡣
    void get_good_objs(List<UsageRate> &objs) const;    // ��ȡ�ϺõĽ��Ŀ�꺯��ֵ��
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
    void update_sol_cache(UsageRate obj, const Solution &sol);
private:
    struct UsageRateCmp {
        bool operator()(const UsageRate &lhs, const UsageRate &rhs) {
            return rhs < lhs;
        }
    };
private:
    const TID plate_;                       // 1-cutλ��ԭ�ϵ�id��
    const Area tail_area_;                  // ʣ�����
    const TCoord start_pos_;                // 1-cut��ʼλ�á�
    const BRANCH_MODE mode_;                // ��֧ģʽ�����á�

    std::mutex sol_mutex_;                  // �������Ž�ʱ���Ȼ�ø���
    size_t nb_sol_cache_;                   // ���������������
    std::map<UsageRate, Solution, UsageRateCmp> sol_cache_;   // ����⡣
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
