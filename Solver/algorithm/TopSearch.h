#pragma once
#ifndef GUILLOTINECUT_TOPSEARCH_H
#define GUILLOTINECUT_TOPSEARCH_H

#include "../Common.h"
#include "PlateSearch.h"

namespace szx {

class TopSearch {
public:
    TopSearch(Random &rand, Timer &timer, Configuration &cfg, const Auxiliary &aux) :
        rand_(rand), timer_(timer), cfg_(cfg), aux_(aux), best_obj_(Problem::Output::MaxWidth) {};

    /* 调用束搜索算法进行优化 */
    void beam_search();
    /* 获取算得的最优解 */
    Length get_bestsol(Solution &sol);
protected:
    Area get_platesol(ID plate_id, const Batch &source_batch, Solution &sol);
    Length greedy_evaluate(ID plate_id, const Batch &source_batch, const Solution &sol);
private:
    Length get_obj(const Solution &sol);
    void update_bestsol(const Solution &sol, Length obj = -1);
private:
    Random &rand_;              // 随机种子发生器
    Timer &timer_;              // 计时器
    Configuration &cfg_;        // 配置参数
    const Auxiliary &aux_;      // 辅助数据

    std::mutex sol_mutex_;      // 更新最优解时需先获得该锁
    Solution best_sol_;         // 最优解
    Length best_obj_;           // 最优解对应的使用原料长度
};

}

#endif
