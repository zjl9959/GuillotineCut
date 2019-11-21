#pragma once

#ifndef GUILLOTINECUT_CUTSEARCH_H
#define GUILLOTINECUT_CUTSEARCH_H

#include "../data/Auxiliary.h"
#include "../data/Batch.h"
#include "../data/Placement.h"
#include "../data/Problem.h"

namespace szx {

class CutSearch {
public:
    struct Setting {
        bool opt_tail;  // 是否优化原料尾部。
        size_t max_iter;   // 最大扩展节点数目。
        size_t max_branch;  // 最大分支宽度。
        Setting(bool _opt_tail = false, int _max_iter = 10000, int _max_branch = 100) :
            opt_tail(_opt_tail), max_iter(_max_iter), max_branch(_max_branch) {}
    };
public:
    CutSearch(TID plate, TCoord start_pos, size_t nb_sol_cache, const Auxiliary &aux, const Setting &set) :
        defect_x_(aux.plate_defect_x[plate]), defect_y_(aux.plate_defect_y[plate]),
        start_pos_(start_pos), items_(aux.items), item_area_(aux.item_area), param_(aux.param),
        set_(set), tail_area_((aux.param.plateWidth - start_pos)*aux.param.plateHeight), nb_sol_cache_(nb_sol_cache) {}
    
    void run(Batch &batch);
    UsageRate best_obj() const;                         // 返回最优解的目标函数值。
    void get_best_sol(Solution &sol) const;             // 获取最优解。
    void get_good_sols(List<Solution> &sols) const;     // 获取较好的解。
    void get_good_objs(List<UsageRate> &objs) const;    // 获取较好的解的目标函数值。
private:
    void beam_search(Batch &batch);
    void dfs(Batch &batch);
    void pfs(Batch &batch);
    UsageRate greedy(Area used_item_area, Batch &batch, const Solution &fix_sol);
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
    const Area tail_area_;                   // 剩余面积
    const TCoord start_pos_;                // 1-cut开始位置。
    const Setting set_;                     // 设置参数。
    const List<Defect> &defect_x_;          // 瑕疵按照x坐标从小到大排列。
    const List<Defect> &defect_y_;          // 瑕疵按照y坐标从小到大排列。
    const List<Rect> &items_;               // 每个物品的长宽信息。
    const List<Area> &item_area_;           // 每个物品的面积信息。
    const Problem::Input::Param &param_;    // 全局参数。
    std::mutex sol_mutex_;                  // 更新最优解时需先获得该锁
    size_t nb_sol_cache_;                   // 缓存解的最大数量。
    std::map<UsageRate, Solution, UsageRateCmp> sol_cache_;   // 缓存解。
};

}

#endif // !SMART_ZJL_GUILLOTINECUT_CUTSEARCH_H
