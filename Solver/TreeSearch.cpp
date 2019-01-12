#include "TreeSearch.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>

#include <cmath>
#include <cassert>

#include "LogSwitch.h"
#include "../Simulator/ThreadPool.h"

using namespace std;

namespace szx {

#pragma region Interface
void TreeSearch::solve() {
    Log(Log::Debug) << "hardware concurrency=" << thread::hardware_concurrency() << endl;
    init();
    //optimizeTotalProblem();
    //iteratorImproveWorstPlate();
    greedyBranchOptimize();
    toOutput(best_solution);
    info.scrap_rate = getScrapWasteRate(best_solution);
}

void TreeSearch::record() const {
    #if SZX_DEBUG

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Length obj = output.totalWidth * input.param.plateHeight - total_item_area;
    Length checkerObj = -1;
    bool feasible = check(checkerObj);

    // record basic information.
    log << env.friendlyLocalTime() << ","
        << env.rid << ","
        << env.instName << ","
        << feasible << "," << (obj - checkerObj) << ","
        << output.totalWidth << ","
        << timer.elapsedSeconds() << ","
        << mu.physicalMemory << "," << mu.virtualMemory << ","
        << env.randSeed << ","
        << cfg.toBriefStr() << ","
        << generation << "," << info.toStr() << ","
        << total_item_area / (output.totalWidth * input.param.plateHeight / 100.0) << "%," // util ratio.
        << checkerObj; // wasted area.

    // record solution vector.
    // EXTEND[szx][2]: save solution in log.
    log << endl;

    // append all text atomically.
    static mutex logFileMutex;
    lock_guard<mutex> logFileGuard(logFileMutex);

    ofstream logFile(env.logPath, ios::app);
    logFile.seekp(0, ios::end);
    if (logFile.tellp() <= 0) {
        logFile << "Time,ID,Instance,Feasible,ObjMatch,Width,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration,Util,Waste,Solution" << endl;
    }
    logFile << log.str();
    logFile.close();
    #endif // SZX_DEBUG
}
#pragma endregion Interface

#pragma region Achievement
void TreeSearch::init() {
    constexpr TID InvalidItemId = Problem::InvalidItemId;

    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());
    aux.item2stack.resize(input.batch.size());
    // assign internal id to items and stacks, then push items into stacks.
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        TID itemId = idMap.item.toConsecutiveId(i->id);
        aux.items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
        if (itemId != i->id) {
            Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
        }

        TID stackId = idMap.stack.toConsecutiveId(i->stack);
        if (aux.stacks.size() <= stackId) { aux.stacks.push_back(List<TID>()); } // create a new stack.
        List<TID> &stack(aux.stacks[stackId]);
        // OPTIMIZE[szx][6]: what if the sequence number could be negative or very large?
        if (stack.size() <= i->seq) { stack.resize(i->seq + 1, InvalidItemId); }
        stack[i->seq] = itemId;
        aux.item2stack[itemId] = stackId;
    }
    // clear invalid items in stacks.
    for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
    }

    aux.defects.reserve(input.defects.size());
    // EXTEND[szx][9]: make sure that the plate IDs are already zero-base consecutive numbers.
    aux.plates_x.resize(Problem::MaxPlateNum);
    aux.plates_y.resize(Problem::MaxPlateNum);
    for (TID p = 0; p < Problem::MaxPlateNum; ++p) { idMap.plate.toConsecutiveId(p); }

    // map defects to plates.
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        TID defectId = idMap.defect.toConsecutiveId(d->id);
        aux.defects.push_back(RectArea(d->x, d->y, d->width, d->height));

        TID plateId = idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plates_x.size() <= plateId) { aux.plates_x.resize(plateId + 1); } // create new plates.
        aux.plates_x[plateId].push_back(defectId);
        if (aux.plates_y.size() <= plateId) { aux.plates_y.resize(plateId + 1); } // create new plates.
        aux.plates_y[plateId].push_back(defectId);
    }

    aux.item_area.reserve(aux.items.size());
    //calculate item areas.
    for (int i = 0; i < aux.items.size(); ++i) {
        aux.item_area.push_back(aux.items[i].h*aux.items[i].w);
        total_item_area += aux.items[i].h*aux.items[i].w;
    }
    // reverse item sequence convicent for pop_back.
    for (int i = 0; i < aux.stacks.size(); i++)
        reverse(aux.stacks[i].begin(), aux.stacks[i].end());
    // sort defects by it's x position.
    for (int p = 0; p < aux.plates_x.size(); ++p)
        for (int d = 0; d < aux.plates_x[p].size(); ++d)
            sort(aux.plates_x[p].begin(), aux.plates_x[p].end(), [&](TID &lhs, TID &rhs) {
            return aux.defects[lhs].x < aux.defects[rhs].x; });
    // sort defects by it's y position.
    for (int p = 0; p < aux.plates_y.size(); ++p)
        for (int d = 0; d < aux.plates_y[p].size(); ++d)
            sort(aux.plates_y[p].begin(), aux.plates_y[p].end(), [&](TID &lhs, TID &rhs) {
            return aux.defects[lhs].y < aux.defects[rhs].y; });
    // get hardware support thread numbers.
    support_thread = thread::hardware_concurrency();
    cfg.mbpn = support_thread;
}

/* fix one plate every turn. */
void TreeSearch::greedyBranchOptimize() {
    TID cur_plate = 0; // current optimizing plate identity.
    List<List<TID>> batch(aux.stacks);
    List<TreeNode> fixed_plate; // fixed plate soultion nodes.
    List<List<TreeNode>> par_sols;
    List<Length> scores;
    while (fixed_plate.size() < aux.items.size()) {
        par_sols.clear();
        par_sols.resize(cfg.mbpn);
        getSomePlateSolutions(cur_plate, batch, par_sols);
        if (timer.isTimeOut()) {
            break;
        }
        scores.clear();
        scores.resize(par_sols.size());
        // evaluate every plate, use thread pool.
        ThreadPool<> tp(support_thread);
        for (int i = 0; i < par_sols.size(); ++i) {
            tp.push([&, i]() {
                scores[i] = evaluateOnePlate(batch, fixed_plate, par_sols[i]);
            });
        }
        tp.pend();
        // get best index in par_sols.
        Length best_score = input.param.plateWidth*input.param.plateNum;
        int best_index = -1;
        for (int i = 0; i < scores.size(); ++i) {
            if (best_score > scores[i]) {
                best_score = scores[i];
                best_index = i;
            }
        }
        // add item into fixed_plate and pop item from batch.
        if (best_index >= 0) {
            for (int i = 0; i < par_sols[best_index].size(); ++i) {
                fixed_plate.push_back(par_sols[best_index][i]);
                batch[aux.item2stack[par_sols[best_index][i].item]].pop_back();
            }
            Log(Log::Debug) << "fix plate:" << cur_plate << endl;
            cur_plate++;
        }
    }
}

/* input: source batch to choose item, fixed plates solution, evaluating plate's solution.
   call getOptimalPlateSolution function to get a complete solution and return it's objective. */
Length TreeSearch::evaluateOnePlate(const List<List<TID>>& source_batch, const List<TreeNode>& fixed_sol, const List<TreeNode>& psol) {
    TID cur_plate = psol.back().plate + 1;
    List<TreeNode> comp_sol(fixed_sol);
    List<List<TID>> batch(source_batch);
    for (int i = 0; i < psol.size(); ++i) {
        comp_sol.push_back(psol[i]);
        batch[aux.item2stack[psol[i].item]].pop_back();
    }
    List<TreeNode> par_sol;
    // go on construct complete solution.
    while (comp_sol.size() < aux.items.size() && !timer.isTimeOut()) {
        par_sol.clear();
        getOptimalPlateSolution(cur_plate, batch, par_sol);
        for (int i = 0; i < par_sol.size(); ++i) {
            comp_sol.push_back(par_sol[i]);
            batch[aux.item2stack[par_sol[i].item]].pop_back();
        }
        cur_plate++;
    }
    if (comp_sol.size() == aux.items.size()) { // get one complete solution.
        const Length temp = comp_sol.back().plate*input.param.plateWidth + comp_sol.back().c1cpr;
        if (temp < best_objective) { // update best solution.
            best_objective = temp;
            best_solution = comp_sol;
            best_usage_rate = (double)total_item_area / (double)(input.param.plateHeight*best_objective);
            Log(Log::Debug) << "a better obj:" << best_objective << endl;
            info.generation_stamp = psol.back().plate;
        }
        return temp;
    } else {
        return input.param.plateWidth*input.param.plateNum;
    }
}

/* input:plate identity, source batch, conatiner list to put solutions(psol.size()>0).
   create different good solutions in one plate, use thread pool in this function. */
void TreeSearch::getSomePlateSolutions(const TID plateId, const List<List<TID>>& source_batch, List<List<TreeNode>>& psols) {
    TreeNode resume_point(-1, plateId, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // optimize some 1-cuts to creat different solution for this plate.
    List<List<List<TID>>> target_batchs; // batchs for branch.
    createItemBatchs(psols.size()*2, resume_point, source_batch, target_batchs);
    List<List<TreeNode>> cut1_sols(target_batchs.size()); // branch 1-cut solutions.
    List<List<List<TID>>> eval_batchs;
    ThreadPool<> tp1(support_thread);
    for (int i = 0; i < target_batchs.size(); ++i) {
        tp1.push([&, i]() {
            optimizeOneCut(resume_point, target_batchs[i], cut1_sols[i]); });
    }
    tp1.pend();
    if (!target_batchs.size() || timer.isTimeOut()) {
        psols.clear();
        return;
    }
    // evaluate 1-cuts and record the score.
    List<pair<int,Area>> scores(cut1_sols.size());
    eval_batchs.clear();
    eval_batchs.resize(cut1_sols.size(), source_batch);
    ThreadPool<> tp2(support_thread);
    for (int i = 0; i < cut1_sols.size(); ++i) {
        for (int j = 0; j < cut1_sols[i].size(); ++j) {
            eval_batchs[i][aux.item2stack[cut1_sols[i][j].item]].pop_back();
        }
        tp2.push([&, i]() {
            scores[i] = make_pair(i, evaluateOneCut(eval_batchs[i], cut1_sols[i]));
        });
    }
    tp2.pend();
    sort(scores.begin(), scores.end(), [](pair<int, Area>& lhs, pair<int, Area>& rhs) {
        return lhs.second > rhs.second; });
    // save some good solution for this plate.
    if (psols.size() > cut1_sols.size()) {
        psols.resize(cut1_sols.size());
    }
    for (int i = 0; i < psols.size(); ++i) {
        psols[i] = cut1_sols[scores[i].first];
    }
}

/* input:plate identity to optimize, source batch to choose item from, container to put solution. */
void TreeSearch::getOptimalPlateSolution(const TID plateId, const List<List<TID>>& source_batch, List<TreeNode>& psol) {
    List<TreeNode> fixed_1cut; // fixed 1-cuts, size keep growing when search.
    List<List<TID>> batch(source_batch);
    TreeNode resume_point(-1, plateId, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    Area max_item_area = 0;
    List<List<List<TID>>> target_batchs; // batchs for branch.
    List<List<TreeNode>> cut1_sols; // branch 1-cut solutions.
    List<List<List<TID>>> eval_batchs; // reference for evaluateOneCut.
    List<List<TreeNode>> eval_sols;
    TID cur_cut1 = 0;
    while (1) {
        target_batchs.clear();
        // create item batchs and make batchs size be a multiple of supported threads
        if (!createItemBatchs(cfg.mhcn, resume_point, batch, target_batchs)) { // no item to place.
            break;
        }
        // optimize some 1-cuts.
        cut1_sols.clear();
        cut1_sols.resize(target_batchs.size());
        bool no_branch_node = true;
        for (int i = 0; i < target_batchs.size(); ++i) {
            optimizeOneCut(resume_point, target_batchs[i], cut1_sols[i]);
            if (cut1_sols[i].size()) {
                no_branch_node = false;
            }
            if (timer.isTimeOut()) {
                break;
            }
        }
        if (no_branch_node || timer.isTimeOut()) {
            break;
        }
        // evaluate the partial solution's quality.
        int best_index = -1; // best index in hopeful sols.
        Area best_score = 0; // the used items area except fixed items area in this plate.
        eval_batchs.clear();
        eval_batchs.resize(cut1_sols.size(), batch);
        eval_sols.clear();
        eval_sols.resize(cut1_sols.size(), fixed_1cut);
        for (int i = 0; i < cut1_sols.size(); ++i) {
            if (cut1_sols[i].size()) {
                for (int j = 0; j < cut1_sols[i].size(); ++j) {
                    eval_batchs[i][aux.item2stack[cut1_sols[i][j].item]].pop_back();
                    eval_sols[i].push_back(cut1_sols[i][j]);
                }
                const Area temp = evaluateOneCut(eval_batchs[i], eval_sols[i]);
                if (best_score < temp) {
                    best_score = temp;
                    best_index = i;
                }
            }
            if (timer.isTimeOut()) {
                break;
            }
        }
        if (best_index >= 0) {
            // pop_back fixed item from batch.
            for (int i = 0; i < cut1_sols[best_index].size(); ++i) {
                fixed_1cut.push_back(cut1_sols[best_index][i]);
                batch[aux.item2stack[cut1_sols[best_index][i].item]].pop_back();
            }
            cur_cut1++;
            // update resume point.
            resume_point = fixed_1cut.back();
            resume_point.c1cpl = resume_point.c1cpr;
            resume_point.c2cpb = resume_point.c2cpu = 0;
            resume_point.cut1++;
            resume_point.depth = -1;
            if (max_item_area < best_score) {
                max_item_area = best_score;
                psol = eval_sols[best_index];
            }
        }
    }
}

/* input:item batch to choose item from, hopeful branch solution.
   continue optimize 1-cut until reach the tail of the plate, and look back to optimze tail.
   output:placed item area in hopeful_sol. */
Area TreeSearch::evaluateOneCut(List<List<TID>>& batch, List<TreeNode>& psol) {
    TreeNode resume_point = psol.back();
    resume_point.c1cpl = resume_point.c1cpr;
    resume_point.c2cpb = resume_point.c2cpu = 0;
    resume_point.cut1++;
    resume_point.depth = -1;
    List<List<List<TID>>> partial_batchs;
    List<TreeNode> best_par_sol;
    List<TreeNode> par_sol;
    while (!timer.isTimeOut()) {
        partial_batchs.clear();
        // create item batchs and make batchs size be a multiple of supported threads
        if (createItemBatchs(cfg.rcin, resume_point, batch, partial_batchs) == 0) {
            break;
        }
        // optimize some 1-cuts and collected the best.
        best_par_sol.clear();
        double best_par_obj = 0.0;
        for (int i = 0; i < partial_batchs.size(); ++i) {
            if (!timer.isTimeOut()) {
                par_sol.clear();
                const double rate = optimizeOneCut(resume_point, partial_batchs[i], par_sol);
                if (best_par_obj < rate) {
                    best_par_sol = par_sol;
                    best_par_obj = rate;
                }
            } else {
                break;
            }
        }
        if (best_par_sol.size()) {
            for (int i = 0; i < best_par_sol.size(); ++i) {
                batch[aux.item2stack[best_par_sol[i].item]].pop_back();
                psol.push_back(best_par_sol[i]);
            }
            resume_point = psol.back();
            resume_point.c1cpl = resume_point.c1cpr;
            resume_point.c2cpb = resume_point.c2cpu = 0;
            resume_point.cut1++;
            resume_point.depth = -1;
        } else { // no place to put item in this plate.
            break;
        }
    }
    // look back the last 1-cut.
    TID last_cut1_id = psol.back().cut1;
    Area tail_item_area = 0;
    double tail_usage_rate = 0.0;
    if (last_cut1_id > 0) {
        int last_cut1_index = 0;
        for (int i = psol.size() - 1; i >= 0; --i) {
            if (psol[i].cut1 == last_cut1_id) { // push last 1-cut's item into batch.
                batch[aux.item2stack[psol[i].item]].push_back(psol[i].item);
                tail_item_area += aux.item_area[psol[i].item];
            } else {
                resume_point = psol[i];
                resume_point.c1cpl = resume_point.c1cpr;
                resume_point.c2cpb = resume_point.c2cpu = 0;
                resume_point.cut1++;
                resume_point.depth = -1;
                tail_usage_rate = (double)tail_item_area /
                    (double)((input.param.plateWidth - psol[i].c1cpr)*input.param.plateHeight);
                last_cut1_index = i;
                break;
            }
        }
        partial_batchs.clear();
        createItemBatchs(cfg.rcin, resume_point, batch, partial_batchs);
        best_par_sol.clear();
        double best_par_obj = 0.0;
        for (int i = 0; i < partial_batchs.size(); ++i) {
            if (!timer.isTimeOut()) {
                par_sol.clear();
                const double rate = optimizePlateTail(resume_point, partial_batchs[i], par_sol);
                if (best_par_obj < rate) {
                    best_par_sol = par_sol;
                    best_par_obj = rate;
                }
            } else {
                break;
            }
        }
        if (best_par_obj > tail_usage_rate) {
            psol.erase(psol.begin() + last_cut1_index + 1, psol.end());
            for (int i = 0; i < best_par_sol.size(); ++i) {
                psol.push_back(best_par_sol[i]);
            }
        }
    }
    // look back last two 1-cut.
    if (last_cut1_id > 1) {
        int last_cut2_index = 0;
        tail_item_area = 0;
        for (int i = psol.size() - 1; i >= 0; --i) {
            if (psol[i].cut1 == last_cut1_id) { // push last 1-cut's item into batch.
                tail_item_area += aux.item_area[psol[i].item];
            } else if (psol[i].cut1 == last_cut1_id - 1) {
                batch[aux.item2stack[psol[i].item]].push_back(psol[i].item);
                tail_item_area += aux.item_area[psol[i].item];
            } else {
                resume_point = psol[i];
                resume_point.c1cpl = resume_point.c1cpr;
                resume_point.c2cpb = resume_point.c2cpu = 0;
                resume_point.cut1++;
                resume_point.depth = -1;
                tail_usage_rate = (double)tail_item_area /
                    (double)((input.param.plateWidth - psol[i].c1cpr)*input.param.plateHeight);
                last_cut2_index = i;
                break;
            }
        }
        partial_batchs.clear();
        createItemBatchs(cfg.rcin, resume_point, batch, partial_batchs);
        best_par_sol.clear();
        double best_par_obj = 0.0;
        for (int i = 0; i < partial_batchs.size(); ++i) {
            if (!timer.isTimeOut()) {
                par_sol.clear();
                const double rate = optimizePlateTail(resume_point, partial_batchs[i], par_sol);
                if (best_par_obj < rate) {
                    best_par_sol = par_sol;
                    best_par_obj = rate;
                }
            } else {
                break;
            }
        }
        if (best_par_obj > tail_usage_rate) {
            psol.erase(psol.begin() + last_cut2_index + 1, psol.end());
            for (int i = 0; i < best_par_sol.size(); ++i) {
                psol.push_back(best_par_sol[i]);
            }
        }
    }
    Area used_item_area = 0; // total item areas in hopeful_sol.
    for (int i = 0; i < psol.size(); ++i) {
        used_item_area += aux.item_area[psol[i].item];
    }
    return used_item_area;
}

/* Iterative optimization every 1-cut. */
void TreeSearch::iteratorImproveWorstPlate() {
    const TID total_item_num = input.batch.size();
    TreeNode resume_point(-1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    List<TreeNode> cmp_sol; // complete solution.
    List<List<TID>> batch = aux.stacks;
    TID left_items = total_item_num;
    TID cur_plate = 0;
    List<int> tabu_restart_plate;
    while (!timer.isTimeOut()) {
        List<List<List<TID>>> partial_batchs;
        while (left_items) {
            List<TreeNode> best_par_sol;
            double best_par_obj = 0.0;
            partial_batchs.clear();
            if (createItemBatchs(cfg.rcin,resume_point, batch, partial_batchs) == 0) {
                // plate left space cant's place any item, 
                resume_point = TreeNode(-1, ++cur_plate, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                createItemBatchs(cfg.rcin, resume_point, batch, partial_batchs);
            }
            for (int i = 0; i < partial_batchs.size(); ++i) {
                List<TreeNode> par_sol;
                if (!timer.isTimeOut()) {
                    const double rate = optimizeOneCut(resume_point, partial_batchs[i], par_sol);
                    if (best_par_obj < rate) {
                        best_par_sol = par_sol;
                        best_par_obj = rate;
                    }
                }
            }
            if (timer.isTimeOut()) {
                break;
            }
            if (best_par_sol.size()) {
                for (int i = 0; i < best_par_sol.size(); ++i) {
                    batch[aux.item2stack[best_par_sol[i].item]].pop_back();
                    cmp_sol.push_back(best_par_sol[i]);
                }
                left_items -= best_par_sol.size();
                resume_point = cmp_sol.back();
                resume_point.c1cpl = resume_point.c1cpr;
                resume_point.c2cpb = resume_point.c2cpu = 0;
                resume_point.cut1++;
                resume_point.depth = -1;
            } else { // no place to put item in this plate.
                resume_point = TreeNode(-1, ++cur_plate, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
        }
        if (cmp_sol.size() == total_item_num) { // get a complete solution.
            const Length cur_obj = 
                cmp_sol.back().plate*input.param.plateWidth + cmp_sol.back().c1cpr;
            if (best_objective > cur_obj) {
                best_objective = cur_obj;
                best_solution = cmp_sol;
                Log(Log::Debug) << "a better obj: " << cur_obj
                                 << " on generation " << generation << endl;
            }
            List<double> plate_usage_rate;
            getPlatesUsageRate(cmp_sol, plate_usage_rate);
            if (tabu_restart_plate.size() < plate_usage_rate.size()) {
                tabu_restart_plate.resize(plate_usage_rate.size(), generation);
            }
            TID wrost_plate_id = 0;
            double wrost_usage_rate = 1.0;
            // find the wrost no tabu plate.
            for (int i = 0; i < plate_usage_rate.size(); ++i) {
                if (generation >= tabu_restart_plate[i] &&
                    plate_usage_rate[i] < wrost_usage_rate) {
                    wrost_plate_id = i;
                    wrost_usage_rate = plate_usage_rate[i];
                }
            }
            tabu_restart_plate[wrost_plate_id] = generation + rand.pick(tabu_restart_plate.size()*0.5);
            // restart optimize from wrost plate's previous plate.
            cur_plate = wrost_plate_id ? wrost_plate_id - 1 : 0;
            int erase_index = 0;
            for (; erase_index < cmp_sol.size(); ++erase_index) {
                if (cmp_sol[erase_index].plate == cur_plate) {
                    break;
                }
            }
            cmp_sol.erase(cmp_sol.begin() + erase_index, cmp_sol.end());
            batch = aux.stacks;
            for (int i = 0; i < cmp_sol.size(); ++i) {
                batch[aux.item2stack[cmp_sol[i].item]].pop_back();
            }
            left_items = total_item_num - cmp_sol.size();
            resume_point = TreeNode(-1, cur_plate, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        } else { // restart optimize from the frist plate.
            cmp_sol.clear();
            batch = aux.stacks;
            cur_plate = 0;
            left_items = total_item_num;
            resume_point = TreeNode(-1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
        //cout << "restart plate:" << cur_plate << endl;
        generation++;
    }
}

/*  input:complete solution, list to store every plate's usage rate.
    calculate every plate's usage rate. */
void TreeSearch::getPlatesUsageRate(const List<TreeNode>& solution, List<double>& usage_rate) {
    TID cur_plate = 0;
    Area plate_item_area = 0;
    usage_rate.clear();
    for (int i = 0; i < solution.size(); ++i) {
        if (solution[i].plate > cur_plate || i == solution.size() - 1) {
            double plate_usage_rate = 0.0;
            if (solution[i].plate > cur_plate) { // calculate normal plate's usage rate.
                plate_usage_rate = (double)plate_item_area / (double)(input.param.plateWidth*input.param.plateHeight);
            } else { // calculate last plate's usage rate.
                plate_item_area += aux.item_area[solution[i].item];
                plate_usage_rate = (double)plate_item_area / (double)(solution[i].c1cpr*input.param.plateHeight);
            }
            usage_rate.push_back(plate_usage_rate);
            cur_plate = solution[i].plate; // update current plate identity.
            plate_item_area = 0; // clear last plate's item area.
        }
        plate_item_area += aux.item_area[solution[i].item];
    }
}

/* input:resume point, current batch to choose item from.
   use item head items to estimate next 1-cut width and calculate
   the 1-cut contain defects number.
   output:1-cut contain defects number. */
const int TreeSearch::estimateDefectNumber(const TreeNode & resume_point, const List<List<TID>>& source_bacth) {
    Area item_areas = 0;
    int item_num = 0;
    // statistic the batch head items areas and item number.
    for (int i = 0; i < source_bacth.size(); ++i) {
        if (source_bacth[i].size() > 1) {
            item_areas += aux.item_area[source_bacth[i].back()];
            item_areas += aux.item_area[source_bacth[i][source_bacth[i].size() - 2]];
            item_num += 2;
        } else if (source_bacth[i].size() == 1) {
            item_areas += aux.item_area[source_bacth[i].back()];
            item_num += 1;
        }
    }
    // calculate next 1-cut width and contain defects number.
    Length width = ((double)item_areas * (double)cfg.mcin) /
        ((double)input.param.plateHeight * best_usage_rate * (double)item_num);
    int res = 0;
    for (auto d : aux.plates_x[resume_point.plate]) {
        if (aux.defects[d].x > resume_point.c1cpr &&
            aux.defects[d].x < resume_point.c1cpr + width) {
            res++;
        }
    }
    return res;
}

/*input:create batch numbers, resume point, source batch lists, target batch lists.
  choose no repeate items by precise method.*/
const int TreeSearch::createItemBatchs(int nums, const TreeNode &resume_point, const List<List<TID>> &source_batch, List<List<List<TID>>> &target_batch) {
    const TLength left_plate_width = input.param.plateWidth - resume_point.c1cpr;
    int target_batchs_num = 0;
    int try_num = 0;
    CombinationCache comb_cache;
    // auto adjust nums according to defect number.
    if (estimateDefectNumber(resume_point, source_batch) > 1) {
        nums *= 2;
    }
    vector<bool> items_set(aux.items.size(), false);
    while (target_batchs_num < nums && try_num < nums) {
        List<List<TID>> temp_batch(source_batch.size());
        TID batch_items = 0; // current temp_batch size contain items.
        List<TID> candidate_items; // candidate items to choose by it's score.
        items_set.assign(aux.items.size(), false);
        for (int i = 0; i < source_batch.size(); ++i) {
            if (source_batch[i].size() && // the choosed item must fit the rest space of the plate.
                aux.items[source_batch[i].back()].h < left_plate_width) {
                candidate_items.push_back(source_batch[i].back());
            }
        }
        while (batch_items < cfg.mcin) {
            if (candidate_items.size() == 0) { // no fit item to choose.
                break;
            }
            int choose_index = rand.pick(candidate_items.size());
            const TID choosed_item = candidate_items[choose_index];
            temp_batch[aux.item2stack[choosed_item]].push_back(choosed_item); // collect choosed item.
            items_set[choosed_item] = true;
            const int index_div = source_batch[aux.item2stack[choosed_item]].size() -
                temp_batch[aux.item2stack[choosed_item]].size();
            if (index_div && aux.items[source_batch[aux.item2stack[choosed_item]][index_div - 1]].h < left_plate_width) {
                // source batch still have item, replace candidate_item[choose_index].
                candidate_items[choose_index] = source_batch[aux.item2stack[choosed_item]][index_div - 1];
            } else {
                candidate_items.erase(candidate_items.begin() + choose_index);
            }
            batch_items++;
        }
        if (!comb_cache.get(items_set)) { // choosed items no repeate.
            target_batch.resize(target_batch.size() + 1);
            // because fetch item from stack back, so reverse it.
            for (int i = 0; i < temp_batch.size(); ++i) {
                if (temp_batch[i].size()) {
                    target_batch[target_batchs_num].resize(target_batch[target_batchs_num].size() + 1);
                    for (int j = temp_batch[i].size() - 1; j >= 0; --j) {
                        target_batch[target_batchs_num].back().push_back(temp_batch[i][j]);
                    }
                }
            }
            comb_cache.set(items_set);
            target_batchs_num++;
            try_num = 0;
        } else {
            try_num++;
        }
    }
    return target_batchs_num;
}

/* input:plate id, start 1-cut position, maximum used width, the batch to be used, solution vector.
   use depth first search to optimize partial solution. */
double TreeSearch::optimizeOneCut(const TreeNode &resume_point, List<List<TID>> &batch, List<TreeNode> &solution) {
    Area batch_item_area = 0;
    List<TID> item2batch(input.batch.size());
    for (int i = 0; i < batch.size(); ++i) {
        for (auto item : batch[i]) {
            batch_item_area += aux.item_area[item];
            item2batch[item] = i;
        }
    }
    double best_obj = -0.1;
    Stack<TreeNode> live_nodes; // the tree nodes to be branched.
    List<TreeNode> cur_parsol, best_parsol; // current partial solution, best partial solution.
    size_t explored_nodes = 0, cut_nodes = 0;
    Depth pre_depth = -1; // last node depth.
    List<TreeNode> branch_nodes;
    partialBranch(resume_point, batch, cur_parsol, branch_nodes);
    for (int i = 0; i < branch_nodes.size(); ++i)
        live_nodes.push(branch_nodes[i]);
    while (live_nodes.size()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop();
        explored_nodes++;
        const double usage_rate_lb =
            (double)batch_item_area / (double)((node.c1cpr - resume_point.c1cpl)*input.param.plateHeight);
        if (usage_rate_lb < best_obj) {
            cut_nodes++;
            continue;
        }
        if (node.depth - pre_depth == 1) { // search forward.
            cur_parsol.push_back(node);
            batch[item2batch[node.item]].pop_back();
        } else if (node.depth - pre_depth < 1) { // search back.
            for (int i = cur_parsol.size() - 1; i >= node.depth; --i) { // resume stack.
                batch[item2batch[cur_parsol[i].item]].push_back(
                    cur_parsol[i].item);
            }
            cur_parsol.erase(cur_parsol.begin() + node.depth, cur_parsol.end()); // erase extend nodes.
            cur_parsol.push_back(node); // push current node into cur_parsol.
            batch[item2batch[node.item]].pop_back();
        }
        branch_nodes.clear();
        partialBranch(node, batch, cur_parsol, branch_nodes);
        for (int i = 0; i < branch_nodes.size(); ++i)
            live_nodes.push(branch_nodes[i]);
        pre_depth = node.depth;
        if (branch_nodes.size() == 0) { // fill a complate 1-cut.
            Area cut1_item_area = 0; // current 1-cut total item area.
            for (int i = 0; i < cur_parsol.size(); ++i)
                cut1_item_area += aux.item_area[cur_parsol[i].item];
            const double cur_obj =  // current 1-cut usage rate.
                (double)cut1_item_area / (double)((cur_parsol.back().c1cpr - resume_point.c1cpl)*input.param.plateHeight);
            if (best_obj < cur_obj) {
                best_obj = cur_obj;
                best_parsol = cur_parsol;
            }
        }
        if (explored_nodes % 100000 == 0 && timer.isTimeOut()) {
            break;
        }
    }
    for (int i = 0; i < best_parsol.size(); ++i) { // record this turn's best partial solution.
        solution.push_back(best_parsol[i]);
    }
    return best_obj;
}

/* input:plate id, start 1-cut position, maximum used width, the batch to be used, solution vector.
   consider the tail of the plate as a total area, return the best usage rate of the area. */
double TreeSearch::optimizePlateTail(const TreeNode & resume_point, List<List<TID>>& batch, List<TreeNode>& solution) {
    const Area tail_area = (input.param.plateWidth - resume_point.c1cpl)*input.param.plateHeight;
    Area used_item_area = 0; // current used items area.
    List<TID> item2batch(input.batch.size());
    for (int i = 0; i < batch.size(); ++i) {
        for (auto item : batch[i]) {
            item2batch[item] = i;
        }
    }
    Area min_waste = tail_area;
    Stack<TreeNode> live_nodes; // the tree nodes to be branched.
    List<TreeNode> cur_parsol, best_parsol; // current partial solution, best partial solution.
    size_t explored_nodes = 0;
    Depth pre_depth = -1; // last node depth.
    List<TreeNode> branch_nodes;
    totalBranch(resume_point, batch, cur_parsol, branch_nodes);
    for (int i = 0; i < branch_nodes.size(); ++i)
        live_nodes.push(branch_nodes[i]);
    while (live_nodes.size()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop();
        explored_nodes++;
        const Area cur_waste_area = (node.c1cpl - resume_point.c1cpl)*input.param.plateHeight
            + node.c2cpb*(node.c1cpr - node.c1cpl) + (node.c3cp - node.c1cpl)*(node.c2cpu - node.c2cpb) - used_item_area;
        if (cur_waste_area > min_waste) {
            continue;
        }
        if (node.depth - pre_depth == 1) { // search forward.
            cur_parsol.push_back(node);
            batch[item2batch[node.item]].pop_back();
            used_item_area += aux.item_area[node.item];
        } else if (node.depth - pre_depth < 1) { // search back.
            for (int i = cur_parsol.size() - 1; i >= node.depth; --i) { // resume stack.
                batch[item2batch[cur_parsol[i].item]].push_back(
                    cur_parsol[i].item);
                used_item_area -= aux.item_area[cur_parsol[i].item];
            }
            cur_parsol.erase(cur_parsol.begin() + node.depth, cur_parsol.end()); // erase extend nodes.
            cur_parsol.push_back(node); // push current node into cur_parsol.
            batch[item2batch[node.item]].pop_back();
            used_item_area += aux.item_area[node.item];
        }
        branch_nodes.clear();
        totalBranch(node, batch, cur_parsol, branch_nodes);
        for (int i = 0; i < branch_nodes.size(); ++i)
            live_nodes.push(branch_nodes[i]);
        pre_depth = node.depth;
        if (branch_nodes.size() == 0) { // fill the tail area.
            Area tail_item_area = 0; // current tail's total items area
            for (int i = 0; i < cur_parsol.size(); ++i)
                tail_item_area += aux.item_area[cur_parsol[i].item];
            if (tail_area - tail_item_area < min_waste) {
                best_parsol = cur_parsol;
                min_waste = tail_area - tail_item_area;
            }
        }
        if (explored_nodes % 100000 == 0 && timer.isTimeOut()) {
            break;
        }
    }
    for (int i = 0; i < best_parsol.size(); ++i) { // record this turn's best partial solution.
        solution.push_back(best_parsol[i]);
    }
    return 1 - (double)min_waste / (double)tail_area;
}

void TreeSearch::optimizeTotalProblem() {
    TID left_item_num = aux.items.size(); // left item number in the batch.
    Area left_item_area = total_item_area;
    List<List<TID>> batch(aux.stacks);
    Stack<TreeNode> live_nodes; // the tree nodes to be branched.
    List<TreeNode> cur_sol, branch_nodes;
    Depth pre_depth = -1; // last node depth.
    totalBranch(TreeNode(-1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        batch, cur_sol, branch_nodes); // first branch.
    for (int i = 0; i < branch_nodes.size(); ++i)
        live_nodes.push(branch_nodes[i]);
    while (live_nodes.size()) {
        TreeNode node = live_nodes.back();
        live_nodes.pop();
        if (getLowBound(node, left_item_area) > best_objective) {
            continue;
        }
        if (node.depth - pre_depth == 1) { // search forward.
            cur_sol.push_back(node);
            batch[aux.item2stack[node.item]].pop_back();
            left_item_num--;
            left_item_area -= aux.item_area[node.item];
            if (left_item_num > 0) {
                branch_nodes.clear();
                totalBranch(node, batch, cur_sol, branch_nodes);
                if (!branch_nodes.size()) { // no place to put item in this plate create a new plate.
                    totalBranch(TreeNode(node.depth, node.plate + 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0),
                        batch, cur_sol, branch_nodes);
                }
                for (int i = 0; i < branch_nodes.size(); ++i)
                    live_nodes.push(branch_nodes[i]);
            }
        } else if (node.depth - pre_depth < 1) { // search back.
            for (int i = cur_sol.size() - 1; i >= node.depth; --i) { // resume stack.
                batch[aux.item2stack[cur_sol[i].item]].push_back(
                    cur_sol[i].item);
                left_item_num++;
                left_item_area += aux.item_area[cur_sol[i].item];
            }
            cur_sol.erase(cur_sol.begin() + node.depth, cur_sol.end()); // erase extend nodes.
            cur_sol.push_back(node); // push current node into cur_parsol.
            batch[aux.item2stack[node.item]].pop_back();
            left_item_num--;
            left_item_area -= aux.item_area[node.item];
            if (left_item_num > 0) {
                branch_nodes.clear();
                totalBranch(node, batch, cur_sol, branch_nodes);
                if (!branch_nodes.size()) { // no place to put item in this plate create a new plate.
                    totalBranch(TreeNode(node.depth, node.plate + 1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0),
                        batch, cur_sol, branch_nodes);
                }
                for (int i = 0; i < branch_nodes.size(); ++i)
                    live_nodes.push(branch_nodes[i]);
            }
        }
        pre_depth = node.depth;
        if (left_item_num == 0) { // find a complate solution.
            int cur_obj = 0;
            for (TreeNode solnode : cur_sol) {
                if (cur_obj < solnode.plate*input.param.plateWidth + solnode.c1cpr) {
                    cur_obj = solnode.plate*input.param.plateWidth + solnode.c1cpr;
                }
            }
            if (best_objective > cur_obj) {
                best_objective = cur_obj;
                best_solution = cur_sol;
                best_usage_rate = (double)total_item_area /
                    (double)(best_objective*input.param.plateHeight);
            }
            if (timer.isTimeOut()) {
                break;
            }
        }
    }
}

/* input:last tree node(branch point).
   function:branch tree node by exact method,
   all the items need to be placed in one L1.
*/// output:push branched nodes into branch_nodes.
void TreeSearch::partialBranch(const TreeNode &old, const List<List<TID>> &batch, const List<TreeNode> &cur_parsol, List<TreeNode> &branch_nodes) {
    const bool c2cpu_locked = old.getFlagBit(FlagBit::LOCKC2); // status: current c2cpu was locked.
    // case A, place item in a new L1.
    if (old.c2cpu == 0) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                if (old.c1cpr + item.w > input.param.plateWidth)continue;
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // TODO: precheck if item exceed plate bound to speed up branch.
                // check if item conflict with defect and try to fix it.
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    TreeNode node_a1(old, itemId, rotate); // place in defect right(if no defect conflict, slip_r is 0).
                    node_a1.c3cp = node_a1.c1cpr = slip_r + item.w;
                    node_a1.c4cp = node_a1.c2cpu = item.h;
                    node_a1.cut2 = 0;
                    if (slip_r > old.c1cpr)
                        node_a1.setFlagBit(FlagBit::DEFECT_R);
                    if (constraintCheck(old, cur_parsol, node_a1)) {
                        branch_nodes.push_back(node_a1);
                    }
                }
                if (slip_r > old.c1cpr) { // item conflict with defect(two choice).
                    slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                    TreeNode node_a2(old, itemId, rotate); // place in defect up.
                    node_a2.c3cp = node_a2.c1cpr = old.c1cpr + item.w;
                    node_a2.c4cp = node_a2.c2cpu = slip_u + item.h;
                    node_a2.cut2 = 0;
                    node_a2.setFlagBit(FlagBit::DEFECT_U);
                    if (constraintCheck(old, cur_parsol, node_a2)) {
                        branch_nodes.push_back(node_a2);
                    }
                }
            }
        }
    }
    // case B, extend c1cpr or place item in a new L2.
    else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place in the right of old.c1cpr.
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    TreeNode node_b1(old, itemId, rotate);
                    node_b1.c3cp = node_b1.c1cpr = slip_r + item.w;
                    node_b1.c4cp = item.h;
                    if (slip_r > old.c1cpr)
                        node_b1.setFlagBit(FlagBit::DEFECT_R);
                    if (constraintCheck(old, cur_parsol, node_b1)) {
                        branch_nodes.push_back(node_b1);
                    }
                }
                if (slip_r > old.c1cpr) {
                    slip_u = sliptoDefectUp(RectArea(old.c1cpr, old.c2cpu - item.h > input.param.minWasteHeight ? 
                        old.c2cpu - item.h : 0, item.w, item.h), old.plate);
                    TreeNode node_b2(old, itemId, rotate);
                    node_b2.c3cp = node_b2.c1cpr = old.c1cpr + item.w;
                    node_b2.c4cp = slip_u + item.h;
                    node_b2.setFlagBit(FlagBit::DEFECT_U);
                    if (constraintCheck(old, cur_parsol, node_b2)) {
                        branch_nodes.push_back(node_b2);
                    }
                }
                // place in the upper of old.c2cpu.
                slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                {
                    TreeNode node_b3(old, itemId, rotate);
                    node_b3.c2cpb = old.c2cpu;
                    node_b3.c4cp = node_b3.c2cpu = old.c2cpu + item.h;
                    node_b3.c3cp = slip_r + item.w;
                    if (node_b3.c1cpr < node_b3.c3cp)
                        node_b3.c1cpr = node_b3.c3cp;
                    ++node_b3.cut2;
                    if (slip_r > old.c1cpl)
                        node_b3.setFlagBit(FlagBit::DEFECT_R);
                    if (constraintCheck(old, cur_parsol, node_b3)) {
                        branch_nodes.push_back(node_b3);
                    }
                }
                if (slip_r > old.c1cpl) {
                    slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    TreeNode node_b4(old, itemId, rotate);
                    node_b4.c2cpb = old.c2cpu;
                    node_b4.c4cp = node_b4.c2cpu = slip_u + item.h;
                    node_b4.c3cp = old.c1cpl + item.w;
                    if (node_b4.c1cpr < node_b4.c3cp)
                        node_b4.c1cpr = node_b4.c3cp;
                    ++node_b4.cut2;
                    node_b4.setFlagBit(FlagBit::DEFECT_U);
                    if (constraintCheck(old, cur_parsol, node_b4)) {
                        branch_nodes.push_back(node_b4);
                    }
                }
            }
        }
    }
    // case C, place item in the old L2 or a new L2 when item extend c1cpr too much.
    else {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place item in the up of current c4cp.
                Length last_item_w = old.getFlagBit(FlagBit::ROTATE) ? aux.items[old.item].h : aux.items[old.item].w;
                if (old.c2cpu > old.c4cp && last_item_w == item.w && !old.getFlagBit(FlagBit::DEFECT_U) &&
                    ((c2cpu_locked && old.c4cp + item.h == old.c2cpb) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpb)) &&
                    !sliptoDefectRight(RectArea(old.c3cp - item.w, old.c4cp, item.w, item.h), old.plate)) {
                    TreeNode node_c1(old, itemId, rotate); // place item in L4 bin.
                    node_c1.c2cpu = node_c1.c4cp = old.c4cp + item.h;
                    node_c1.setFlagBit(FlagBit::BIN4);
                    node_c1.setFlagBit(FlagBit::LOCKC2);
                    if (constraintCheck(old, cur_parsol, node_c1)) {
                        branch_nodes.push_back(node_c1);
                    }
                    continue;
                }
                if (old.c1cpr > old.c3cp) { // place item in the right of current c3cp.
                    slip_r = sliptoDefectRight(RectArea(old.c3cp, old.c2cpb, item.w, item.h), old.plate);
                    // when c2cpu is locked, old.c2cpb + item.h <= old.c2cpu constraint must meet.
                    if (!c2cpu_locked || (c2cpu_locked && old.c2cpb + item.h <= old.c2cpu)) {
                        TreeNode node_c2(old, itemId, rotate);
                        node_c2.c3cp = slip_r + item.w;
                        if (node_c2.c1cpr < node_c2.c3cp)
                            node_c2.c1cpr = node_c2.c3cp;
                        node_c2.c4cp = old.c2cpb + item.h;
                        if (slip_r > old.c3cp)
                            node_c2.setFlagBit(FlagBit::DEFECT_R);
                        if (c2cpu_locked)
                            node_c2.setFlagBit(FlagBit::LOCKC2);
                        if (constraintCheck(old, cur_parsol, node_c2)) {
                            branch_nodes.push_back(node_c2);
                        }
                    }
                    if (slip_r > old.c3cp) {
                        slip_u = sliptoDefectUp(RectArea(old.c3cp, old.c2cpu - item.h > old.c2cpb + input.param.minWasteHeight ? 
                            old.c2cpu - item.h : old.c2cpb, item.w, item.h), old.plate);
                        if (!c2cpu_locked || (c2cpu_locked && slip_u + item.h <= old.c2cpu)) {
                            TreeNode node_c3(old, itemId, rotate);
                            node_c3.c3cp = old.c3cp + item.w;
                            if (node_c3.c1cpr < node_c3.c3cp)
                                node_c3.c1cpr = node_c3.c3cp;
                            node_c3.c4cp = item.h + slip_u;
                            node_c3.setFlagBit(FlagBit::DEFECT_U);
                            if (c2cpu_locked)
                                node_c3.setFlagBit(FlagBit::LOCKC2);
                            if (constraintCheck(old, cur_parsol, node_c3)) {
                                branch_nodes.push_back(node_c3);
                            }
                        }
                    }
                }
                if (old.c3cp + item.w > old.c1cpr) {
                    // creat a new L2 and place item in it.
                    slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    {
                       TreeNode node_c4(old, itemId, rotate);
                        node_c4.c3cp = slip_r + item.w;
                        if (node_c4.c1cpr < node_c4.c3cp)
                            node_c4.c1cpr = node_c4.c3cp;
                        node_c4.c2cpb = old.c2cpu;
                        node_c4.c4cp = node_c4.c2cpu = old.c2cpu + item.h;
                        ++node_c4.cut2; // update new L2 id.
                        if (slip_r > old.c1cpl)
                            node_c4.setFlagBit(FlagBit::DEFECT_R);
                        if (constraintCheck(old, cur_parsol, node_c4)) {
                            branch_nodes.push_back(node_c4);
                        }
                    }
                    if (slip_r > old.c1cpl) {
                        slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                        TreeNode node_c5(old, itemId, rotate);
                        node_c5.c3cp = old.c1cpl + item.w;
                        if (node_c5.c1cpr < node_c5.c3cp)
                            node_c5.c1cpr = node_c5.c3cp;
                        node_c5.c2cpb = old.c2cpu;
                        node_c5.c4cp = node_c5.c2cpu = slip_u + item.h;
                        ++node_c5.cut2;
                        node_c5.setFlagBit(FlagBit::DEFECT_U);
                        if (constraintCheck(old, cur_parsol, node_c5)) {
                            branch_nodes.push_back(node_c5);
                        }
                    }
                }
            }
        }
    }
}

/* input:last tree node(branch point).
   function:branch tree node by exact method,
   can create new L1 when this L1 has no place to put item.
*/// output:push branched nodes into branch_nodes.
void TreeSearch::totalBranch(const TreeNode & old, const List<List<TID>>& batch, const List<TreeNode>& cur_parsol, List<TreeNode>& branch_nodes) {
    const bool c2cpu_locked = old.getFlagBit(FlagBit::LOCKC2); // status: current c2cpu was locked.
    // case A, place item in a new L1.
    if (old.c2cpu == 0) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                if (old.c1cpr + item.w > input.param.plateWidth)continue;
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // TODO: precheck if item exceed plate bound to speed up branch.
                // check if item conflict with defect and try to fix it.
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    TreeNode node_a1(old, itemId, rotate); // place in defect right(if no defect conflict, slip_r is 0).
                    node_a1.c3cp = node_a1.c1cpr = slip_r + item.w;
                    node_a1.c4cp = node_a1.c2cpu = item.h;
                    node_a1.cut2 = 0;
                    if (slip_r > old.c1cpr)
                        node_a1.setFlagBit(FlagBit::DEFECT_R);
                    if (constraintCheck(old, cur_parsol, node_a1)) {
                        branch_nodes.push_back(node_a1);
                    }
                }
                if (slip_r > old.c1cpr) { // item conflict with defect(two choice).
                    slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                    TreeNode node_a2(old, itemId, rotate); // place in defect up.
                    node_a2.c3cp = node_a2.c1cpr = old.c1cpr + item.w;
                    node_a2.c4cp = node_a2.c2cpu = slip_u + item.h;
                    node_a2.cut2 = 0;
                    node_a2.setFlagBit(FlagBit::DEFECT_U);
                    if (constraintCheck(old, cur_parsol, node_a2)) {
                        branch_nodes.push_back(node_a2);
                    }
                }
            }
        }
    }
    // case B, extend c1cpr or place item in a new L2.
    else if (old.c2cpb == 0 && old.c1cpr == old.c3cp) {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place in the right of old.c1cpr.
                slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                {
                    TreeNode node_b1(old, itemId, rotate);
                    node_b1.c3cp = node_b1.c1cpr = slip_r + item.w;
                    node_b1.c4cp = item.h;
                    if (slip_r > old.c1cpr)
                        node_b1.setFlagBit(FlagBit::DEFECT_R);
                    if (constraintCheck(old, cur_parsol, node_b1)) {
                        branch_nodes.push_back(node_b1);
                    }
                }
                if (slip_r > old.c1cpr) {
                    slip_u = sliptoDefectUp(RectArea(old.c1cpr, old.c2cpu - item.h > input.param.minWasteHeight ?
                        old.c2cpu - item.h : 0, item.w, item.h), old.plate);
                    TreeNode node_b2(old, itemId, rotate);
                    node_b2.c3cp = node_b2.c1cpr = old.c1cpr + item.w;
                    node_b2.c4cp = slip_u + item.h;
                    node_b2.setFlagBit(FlagBit::DEFECT_U);
                    if (constraintCheck(old, cur_parsol, node_b2)) {
                        branch_nodes.push_back(node_b2);
                    }
                }
                // place in the upper of old.c2cpu.
                slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                {
                    TreeNode node_b3(old, itemId, rotate);
                    node_b3.c2cpb = old.c2cpu;
                    node_b3.c4cp = node_b3.c2cpu = old.c2cpu + item.h;
                    node_b3.c3cp = slip_r + item.w;
                    if (node_b3.c1cpr < node_b3.c3cp)
                        node_b3.c1cpr = node_b3.c3cp;
                    ++node_b3.cut2;
                    if (slip_r > old.c1cpl)
                        node_b3.setFlagBit(FlagBit::DEFECT_R);
                    if (constraintCheck(old, cur_parsol, node_b3)) {
                        branch_nodes.push_back(node_b3);
                    }
                }
                if (slip_r > old.c1cpl) {
                    slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    TreeNode node_b4(old, itemId, rotate);
                    node_b4.c2cpb = old.c2cpu;
                    node_b4.c4cp = node_b4.c2cpu = slip_u + item.h;
                    node_b4.c3cp = old.c1cpl + item.w;
                    if (node_b4.c1cpr < node_b4.c3cp)
                        node_b4.c1cpr = node_b4.c3cp;
                    ++node_b4.cut2;
                    node_b4.setFlagBit(FlagBit::DEFECT_U);
                    if (constraintCheck(old, cur_parsol, node_b4)) {
                        branch_nodes.push_back(node_b4);
                    }
                }
            }
        }
    }
    // case C, place item in the old L2 or a new L2 when item extend c1cpr too much.
    else {
        for (int rotate = 0; rotate <= 1; ++rotate) {
            for (int i = 0; i < batch.size(); ++i) {
                // pretreatment.
                if (!batch[i].size())continue; // skip empty stack.
                TID itemId = batch[i].back();
                Rect item = aux.items[itemId];
                if (item.w == item.h && rotate)continue; // square item no need to rotate.
                if (rotate) { // rotate item.
                    item.w = aux.items[itemId].h;
                    item.h = aux.items[itemId].w;
                }
                // if item confilct, slip_r to place item in the defect right, slip_u to place item in the defect up.
                TLength slip_r = 0, slip_u = 0;
                // place item in the up of current c4cp.
                Length last_item_w = old.getFlagBit(FlagBit::ROTATE) ? aux.items[old.item].h : aux.items[old.item].w;
                if (old.c2cpu > old.c4cp && last_item_w == item.w && !old.getFlagBit(FlagBit::DEFECT_U) &&
                    ((c2cpu_locked && old.c4cp + item.h == old.c2cpb) || (!c2cpu_locked && old.c4cp + item.h >= old.c2cpb)) &&
                    !sliptoDefectRight(RectArea(old.c3cp - item.w, old.c4cp, item.w, item.h), old.plate)) {
                    TreeNode node_c1(old, itemId, rotate); // place item in L4 bin.
                    node_c1.c2cpu = node_c1.c4cp = old.c4cp + item.h;
                    node_c1.setFlagBit(FlagBit::BIN4);
                    node_c1.setFlagBit(FlagBit::LOCKC2);
                    if (constraintCheck(old, cur_parsol, node_c1)) {
                        branch_nodes.push_back(node_c1);
                    }
                    continue;
                }
                if (old.c1cpr > old.c3cp) { // place item in the right of current c3cp.
                    slip_r = sliptoDefectRight(RectArea(old.c3cp, old.c2cpb, item.w, item.h), old.plate);
                    // when c2cpu is locked, old.c2cpb + item.h <= old.c2cpu constraint must meet.
                    if (!c2cpu_locked || (c2cpu_locked && old.c2cpb + item.h <= old.c2cpu)) {
                        TreeNode node_c2(old, itemId, rotate);
                        node_c2.c3cp = slip_r + item.w;
                        if (node_c2.c1cpr < node_c2.c3cp)
                            node_c2.c1cpr = node_c2.c3cp;
                        node_c2.c4cp = old.c2cpb + item.h;
                        if (slip_r > old.c3cp)
                            node_c2.setFlagBit(FlagBit::DEFECT_R);
                        if (c2cpu_locked)
                            node_c2.setFlagBit(FlagBit::LOCKC2);
                        if (constraintCheck(old, cur_parsol, node_c2)) {
                            branch_nodes.push_back(node_c2);
                        }
                    }
                    if (slip_r > old.c3cp) {
                        slip_u = sliptoDefectUp(RectArea(old.c3cp, old.c2cpu - item.h > old.c2cpb + input.param.minWasteHeight ?
                            old.c2cpu - item.h : old.c2cpb, item.w, item.h), old.plate);
                        if (!c2cpu_locked || (c2cpu_locked && slip_u + item.h <= old.c2cpu)) {
                            TreeNode node_c3(old, itemId, rotate);
                            node_c3.c3cp = old.c3cp + item.w;
                            if (node_c3.c1cpr < node_c3.c3cp)
                                node_c3.c1cpr = node_c3.c3cp;
                            node_c3.c4cp = item.h + slip_u;
                            node_c3.setFlagBit(FlagBit::DEFECT_U);
                            if (c2cpu_locked)
                                node_c3.setFlagBit(FlagBit::LOCKC2);
                            if (constraintCheck(old, cur_parsol, node_c3)) {
                                branch_nodes.push_back(node_c3);
                            }
                        }
                    }
                }
                if (old.c3cp + item.w > old.c1cpr) {
                    bool flag = false;
                    // creat a new L2 and place item in it.
                    slip_r = sliptoDefectRight(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                    {
                        TreeNode node_c4(old, itemId, rotate);
                        node_c4.c3cp = slip_r + item.w;
                        if (node_c4.c1cpr < node_c4.c3cp)
                            node_c4.c1cpr = node_c4.c3cp;
                        node_c4.c2cpb = old.c2cpu;
                        node_c4.c4cp = node_c4.c2cpu = old.c2cpu + item.h;
                        ++node_c4.cut2; // update new L2 id.
                        if (slip_r > old.c1cpl)
                            node_c4.setFlagBit(FlagBit::DEFECT_R);
                        if (constraintCheck(old, cur_parsol, node_c4)) {
                            branch_nodes.push_back(node_c4);
                            flag = true;
                        }
                    }
                    if (slip_r > old.c1cpl) {
                        slip_u = sliptoDefectUp(RectArea(old.c1cpl, old.c2cpu, item.w, item.h), old.plate);
                        TreeNode node_c5(old, itemId, rotate);
                        node_c5.c3cp = old.c1cpl + item.w;
                        if (node_c5.c1cpr < node_c5.c3cp)
                            node_c5.c1cpr = node_c5.c3cp;
                        node_c5.c2cpb = old.c2cpu;
                        node_c5.c4cp = node_c5.c2cpu = slip_u + item.h;
                        ++node_c5.cut2;
                        node_c5.setFlagBit(FlagBit::DEFECT_U);
                        if (constraintCheck(old, cur_parsol, node_c5)) {
                            branch_nodes.push_back(node_c5);
                            flag = true;
                        }
                    }
                    if (flag)continue; // if item can placed in a new L2, no need to place it in a new L1.
                    // creat a new L1 and place item in it.
                    slip_r = sliptoDefectRight(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                    {
                        TreeNode node_c6(old, itemId, rotate);
                        node_c6.c1cpl = old.c1cpr;
                        node_c6.c3cp = node_c6.c1cpr = item.w + slip_r;
                        node_c6.c2cpb = 0;
                        node_c6.c4cp = node_c6.c2cpu = item.h;
                        if (slip_r > old.c1cpr)
                            node_c6.setFlagBit(FlagBit::DEFECT_R);
                        ++node_c6.cut1;
                        node_c6.cut2 = 0;
                        if (constraintCheck(old, cur_parsol, node_c6)) {
                            branch_nodes.push_back(node_c6);
                        }
                    }
                    if (slip_r > old.c1cpr) {
                        slip_u = sliptoDefectUp(RectArea(old.c1cpr, 0, item.w, item.h), old.plate);
                        TreeNode node_c7(old, itemId, rotate);
                        node_c7.c1cpl = old.c1cpr;
                        node_c7.c3cp = node_c7.c1cpr = old.c1cpr + item.w;
                        node_c7.c2cpb = 0;
                        node_c7.c4cp = node_c7.c2cpu = slip_u + item.h;
                        node_c7.setFlagBit(FlagBit::DEFECT_U);
                        ++node_c7.cut1;
                        node_c7.cut2 = 0;
                        if (constraintCheck(old, cur_parsol, node_c7)) {
                            branch_nodes.push_back(node_c7);
                        }
                    }
                }
            }
        }
    }
}

/* input: old branch node, current partial solution, new branch node.
   function: check if new branch satisfy all the constraints.
   if some constraints not staisfy, try to fix is by move item. */
const bool TreeSearch::constraintCheck(const TreeNode &old, const List<TreeNode> &cur_parsol, TreeNode &node) {
    bool moved_cut1 = false, moved_cut2 = false;
    // if c2cpu less than c4cp, move it up.
    if (node.c2cpu < node.c4cp) {
        node.c2cpu = node.c4cp;
        moved_cut2 = true;
    }
    // check if item right side and c1cpr interval less than minWasteWidth.
    if (node.c3cp != node.c1cpr && node.c1cpr - node.c3cp < input.param.minWasteWidth) {
        node.c1cpr += input.param.minWasteWidth; // move c1cpr right to staisfy constraint.
        moved_cut1 = true;
    }
    // check if new c1cpr and old c1cpr interval less than minWasteWidth.
    if (node.c1cpr != old.c1cpr && node.c1cpr - old.c1cpr < input.param.minWasteWidth) {
        node.c1cpr += input.param.minWasteWidth;
        moved_cut1 = true;
    }
    // check if item up side and c2cpu interval less than minWasteHeight.
    if (node.c4cp != node.c2cpu && node.c2cpu - node.c4cp < input.param.minWasteHeight) {
        if (node.getFlagBit(FlagBit::LOCKC2)) { // c2cpu cant's move in this case.
            return false;
        } else {
            node.c2cpu += input.param.minWasteHeight; // move c2cpu up to satisfy constraint.
            if (node.getFlagBit(FlagBit::DEFECT_U))
                node.c4cp = node.c2cpu;
            moved_cut2 = true;
        }
    }
    // check if new c2cpu and old c2cpu interval less than minWasteHeight
    if (node.cut1 == old.cut1 && node.cut2 == old.cut2 && node.c2cpu != old.c2cpu 
        && node.c2cpu - old.c2cpu < input.param.minWasteHeight) {
        if (node.getFlagBit(FlagBit::LOCKC2)) {
            return false;
        } else {
            node.c2cpu += input.param.minWasteHeight;
            if (node.getFlagBit(FlagBit::DEFECT_U))
                node.c4cp = node.c2cpu;
            moved_cut2 = true;
        }
    }
    // check c3cp not cut through defect if item placed in defect up.
    if (node.getFlagBit(FlagBit::DEFECT_U)) {
        for (int i = 0; i < aux.plates_x[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_x[node.plate][i]];
            if (defect.x > node.c3cp) {
                break;
            }
            if (node.c3cp < defect.x + defect.w
                && node.c2cpb < defect.y + defect.h && node.c2cpu > defect.y) {
                return false;
            }
        }
    }
    // check not cut through defect constraint and try to fix it.
    if (moved_cut1 || old.c1cpr != node.c1cpr) {
        for (int i = 0; i < aux.plates_x[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_x[node.plate][i]];
            if (defect.x <= node.c1cpr) {
                if (defect.x + defect.w > node.c1cpr) { // c1cpr cut through defect.
                    if (defect.x + defect.w - node.c1cpr < input.param.minWasteWidth) {
                        if (defect.x + defect.w - node.c1cpr < input.param.minWasteWidth && !moved_cut1)
                            node.c1cpr += input.param.minWasteWidth;
                        else
                            node.c1cpr = defect.x + defect.w; // move c1cpr -> defect right side to satisfy the constraint.
                    } else {
                        return false;
                    }
                }
            } else { // the leftover defects is in the right of c1cpr.
                break;
            }
        }
    }
    if (moved_cut2 || old.c2cpu != node.c2cpu || old.c1cpr != node.c1cpr) {
        for (int i = 0; i < aux.plates_y[node.plate].size(); ++i) {
            RectArea defect = aux.defects[aux.plates_y[node.plate][i]];
            if (defect.y < node.c2cpu) {
                if (defect.y + defect.h > node.c2cpu
                    && node.c1cpl < defect.x + defect.w && node.c1cpr > defect.x) { // c2cpu cut through defect.
                    if (node.getFlagBit(FlagBit::LOCKC2) || // can't move c2cpu to satisfy the constraint.
                        defect.y + defect.h - node.c2cpu < input.param.minWasteHeight) {
                        return false;
                    } else {
                        if (defect.y + defect.h - node.c2cpu < input.param.minWasteHeight && !moved_cut2)
                            node.c2cpu += input.param.minWasteHeight;
                        else
                            node.c2cpu = defect.y + defect.h; // move c2cpu -> defect up side to satisfy the constraint.
                        if (node.getFlagBit(FlagBit::DEFECT_U))
                            node.c4cp = node.c2cpu;
                        moved_cut2 = true;
                    }
                }
            } else { // the leftover defects is in the upper of c2cpu.
                break;
            }
        }
    }
    // search back the current solution to check if item(placed in defect upper) conflict with defect after slip up.
    if (moved_cut2) {
        // check if current consider node conflict with defect after move.
        if (node.getFlagBit(FlagBit::DEFECT_U)) {
            const TLength item_w = node.getFlagBit(FlagBit::ROTATE) ? 
                aux.items[node.item].h : aux.items[node.item].w;
            const TLength item_h = node.getFlagBit(FlagBit::ROTATE) ? 
                aux.items[node.item].w : aux.items[node.item].h;
            if (defectConflictArea(RectArea(
                node.c3cp - item_w, node.c2cpu - item_h, item_w, item_h), node.plate))
                return false;
        }
        // check current partial solution
        for (int i = cur_parsol.size() - 1; i >= 0; --i) {
            if (cur_parsol[i].cut2 == node.cut2 && // the checked item should be in the same cut2.
                cur_parsol[i].cut1 == node.cut1 &&
                cur_parsol[i].plate == node.plate) {
                if (cur_parsol[i].getFlagBit(FlagBit::DEFECT_U)) {
                    const TLength item_w = cur_parsol[i].getFlagBit(FlagBit::ROTATE) ?
                        aux.items[cur_parsol[i].item].h : aux.items[cur_parsol[i].item].w;
                    const TLength item_h = cur_parsol[i].getFlagBit(FlagBit::ROTATE) ?
                        aux.items[cur_parsol[i].item].w : aux.items[cur_parsol[i].item].h;
                    if (defectConflictArea(RectArea(
                        cur_parsol[i].c3cp - item_w, node.c2cpu - item_h, item_w, item_h), cur_parsol[i].plate))
                        return false;
                }
                // check if (old.c3cp,old.c2cpb,0,node.c2cpu-old.c2cpb) conflict with defect.
                if (defectConflictArea(RectArea(
                    cur_parsol[i].c3cp, cur_parsol[i].c2cpb, 0, node.c2cpu - cur_parsol[i].c2cpb), node.plate)) {
                    return false;
                }
            } else {
                break;
            }
        }
    }
    // check if cut1 exceed stock right side.
    if (node.c1cpr > input.param.plateWidth) {
        return false;
    }
    // check if cut2 exceed stock up side.
    if (node.c2cpu > input.param.plateHeight) {
        return false;
    }
    // check if maximum cut1 width not staisfy.
    if (node.c1cpr - node.c1cpl > input.param.maxL1Width) {
        return false;
    }
    // check if minimum cut1 width not staisfy.
    if (node.cut1 > old.cut1 && old.cut1 > -1 && old.c1cpr - old.c1cpl < input.param.minL1Width) {
        return false;
    }
    // check if minimum cut2 height not staisfy.
    if (node.cut2 > old.cut2 && old.c2cpu - old.c2cpb < input.param.minL2Height) {
        return false;
    }
    // check if cut1 and stock right side interval less than minimum waste width.
    if (input.param.plateWidth - node.c1cpr != 0 &&
        input.param.plateWidth - node.c1cpr < input.param.minWasteWidth) {
        return false;
    }
    // check if cut2 and stock up side interval less than minimum waste height.
    if (input.param.plateHeight - node.c2cpu != 0 &&
        input.param.plateHeight - node.c2cpu < input.param.minWasteHeight) {
        return false;
    }
    return true; // every constraints satisfied.
}

/* input:rectangle area to place item, plate id.
   function: if rectangle area conflict with one or more defects,
   slip the area right to the rightmost defects right side.
*/// output: slip position to avoid conflict with defect.
const TCoord TreeSearch::sliptoDefectRight(const RectArea &area, const TID plate) const {
    TCoord slip = area.x;
    for (auto d : aux.plates_x[plate]) {
        if (aux.defects[d].x >= slip + area.w) {
            break; // the defects is sorted by x position, so in this case just break.
        }
        if (!(slip - aux.defects[d].x >= aux.defects[d].w // defect in item left.
            || aux.defects[d].y - area.y >= area.h // defect in item up.
            || area.y - aux.defects[d].y >= aux.defects[d].h)) { // defect in item bottom.
            slip = aux.defects[d].x + aux.defects[d].w; // defect[d] conflict with area, slip to defect right side.
            if (slip - area.x != 0 && slip - area.x < input.param.minWasteWidth)
                slip = area.x + input.param.minWasteWidth;
        }
    }
    return slip;
}

/* input:rectangle area to place item, plate id.
   function: if rectangle area conflict with one or more defects.
   slip the area up to the upmost defects up side.
*/// output: slip position to avoid conflict with defect.
const TCoord TreeSearch::sliptoDefectUp(const RectArea &area, const TID plate) const {
    TCoord slip = area.y;
    for (auto d : aux.plates_y[plate]) {
        if (aux.defects[d].y >= slip + area.h) {
            break; // the defects is sorted by y position, so in this case just break.
        }
        if (!(aux.defects[d].x - area.x >= area.w // defect in item right.
            || area.x - aux.defects[d].x >= aux.defects[d].w // defect in item left.
            || slip - aux.defects[d].y >= aux.defects[d].h)) { // defect in item bottom.
            slip = aux.defects[d].y + aux.defects[d].h; // defect[d] conflict with area, slip to defect up side.
            if (slip - area.y != 0 && slip - area.y < input.param.minWasteHeight)
                slip = area.y + input.param.minWasteHeight;
        }
    }
    return slip;
}

// if one or more defct conflict with area, return true.
const bool TreeSearch::defectConflictArea(const RectArea & area, const TID plate) const {
    for (auto d : aux.plates_y[plate]) {
        if (aux.defects[d].y >= area.y + area.h) {
            break;
        }
        if (!(aux.defects[d].x - area.x >= area.w // defect in item right.
            || area.x - aux.defects[d].x >= aux.defects[d].w  // defect in item left.
            || area.y - aux.defects[d].y >= aux.defects[d].h)) {  // defect in item bottom.
            return true;
        }
    }
    return false;
}

/* input: current branch node, the left item area to calculate low bound.
   calculate a little relaxed low bound */
const Length TreeSearch::getLowBound(const TreeNode & cur_node, Area left_item_area) const {
    left_item_area = left_item_area - (cur_node.c2cpu - cur_node.c2cpb)*(cur_node.c1cpr - cur_node.c3cp) -
        (input.param.plateHeight - cur_node.c2cpu)*(cur_node.c1cpr - cur_node.c1cpl) - aux.item_area[cur_node.item];
    return input.param.plateWidth*cur_node.plate + cur_node.c1cpr + max(
        0, static_cast<Length>(left_item_area / (input.param.plateHeight*best_usage_rate)));
}

// this function convert TreeNode type solution into output type .
void TreeSearch::toOutput(List<TreeNode> &sol) {
    using SpecialType = Problem::Output::Node::SpecialType;
    const TLength PW = input.param.plateWidth, PH = input.param.plateHeight;
    // define current plate/cut1/cut2 identity in solution node.
    TID cur_plate = -1, cur_cut1 = 0, cur_cut2 = 0;
    // define current node/parentL0.. identity in output solution tree node.
    TID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    // define current c1cpl/c1cpr... when constructing solution tree.
    TCoord c1cpl = 0, c1cpr = 0, c2cpb=0, c2cpu = 0, c3cp = 0;
    // construct solution tree by visit each sol TreeNode.
    for (int n = 0; n < sol.size(); ++n) {
        if (cur_plate < sol[n].plate) { // a new plate.
            // add waste for last plate
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (cur_plate != -1 && c2cpu < input.param.plateHeight) { // add waste between c2cpu and plate up bound.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            if (cur_plate != -1 && c1cpr < input.param.plateWidth) { // add waste between c1cpr and plate right bound.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Waste, 1, parent_L0));
            }
            // update cut position and cut identity information.
            cur_plate = sol[n].plate;
            cur_cut1 = 0;
            cur_cut2 = 0;
            c1cpl = 0;
            c1cpr = getC1cpr(sol, n, cur_plate, cur_cut1);
            c2cpb = 0;
            c2cpu = getC2cpu(sol, n, cur_cut1, cur_cut2);
            c3cp = 0;
            // creat new nodes.
            parent_L0 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new plate.
                cur_plate, nodeId++, 0, 0, PW, PH, SpecialType::Branch, 0, -1));
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1.
                cur_plate, nodeId++, 0, 0, c1cpr, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, 0, 0, c1cpr, c2cpu, SpecialType::Branch, 2, parent_L1));
        }
        else if (cur_cut1 < sol[n].cut1) { // a new cut1.
            // add waste for last L1.
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (c2cpu < input.param.plateHeight) { // add waste between c2cpu and plate up bound.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            // update cut position and cut identity information.
            cur_cut1 = sol[n].cut1;
            cur_cut2 = 0;
            c1cpl = c1cpr;
            c1cpr = getC1cpr(sol, n, cur_plate, cur_cut1);
            c2cpb = 0;
            c2cpu = getC2cpu(sol, n, cur_cut1, cur_cut2);
            c3cp = c1cpl;
            // creat new nodes.
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1.
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        }
        else if (cur_cut2 < sol[n].cut2) { // a new cut2.
            // add waste for last L2.
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            // update cut position and cut identity information.
            cur_cut2 = sol[n].cut2;
            c2cpb = c2cpu;
            c2cpu = getC2cpu(sol, n, cur_cut1, cur_cut2);
            c3cp = c1cpl;
            // creat new nodes.
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        const TLength item_w = sol[n].getFlagBit(FlagBit::ROTATE) ? aux.items[sol[n].item].h : aux.items[sol[n].item].w;
        const TLength item_h = sol[n].getFlagBit(FlagBit::ROTATE) ? aux.items[sol[n].item].w : aux.items[sol[n].item].h;
        // if item placed in defect right bound, creat waste between c3cp and item left bound.
        if (sol[n].getFlagBit(FlagBit::DEFECT_R)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // if has bin4, place this node into last L3, else creat a new L3.
        if (sol[n].getFlagBit(FlagBit::BIN4)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
            continue;
        } else {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        // if item placed in defect up bound, creat waste between item bottom and c2cpb.
        if (sol[n].getFlagBit(FlagBit::DEFECT_U)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
        } else {
            // creat a item in L3.
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, item_h, sol[n].item, 4, parent_L3));
            // add waste between c4cp and c2cpu.
            if (c2cpu > sol[n].c4cp && n + 1 < sol.size() && !sol[n + 1].getFlagBit(FlagBit::BIN4)) {
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, sol[n].c3cp - item_w, sol[n].c4cp, item_w, c2cpu - sol[n].c4cp, SpecialType::Waste, 4, parent_L3));
            }
        }
        c3cp = sol[n].c3cp;
    }
    const TreeNode last_item = sol.back();
    const TLength last_item_w = last_item.getFlagBit(FlagBit::ROTATE) ? 
        aux.items[last_item.item].h : aux.items[last_item.item].w;
    if (last_item.c4cp < c2cpu && !last_item.getFlagBit(FlagBit::DEFECT_U)) { // add last waste between c4cp and c2cpu.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, last_item.c3cp - last_item_w, 
            last_item.c4cp, last_item_w, c2cpu - last_item.c4cp, SpecialType::Waste, 4, parent_L3));
    }
    if (c3cp < c1cpr) { // add last waste between c3cp and c1cpr.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
    }
    if (c2cpu < input.param.plateHeight) { // add last waste between c2cpu and plate up bound.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
    }
    if (c1cpr < input.param.plateWidth) { // add last residual between c1cpr and plate right bound.
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Residual, 1, parent_L0));
    }
    // calculate total width of output.
    output.totalWidth = 0;
    for (int i = 0; i < sol.size(); ++i) {
        if (output.totalWidth < sol[i].plate*input.param.plateWidth + sol[i].c1cpr) {
            output.totalWidth = sol[i].plate*input.param.plateWidth + sol[i].c1cpr;
        }
    }
}

/* input: solution TreeNode type, solution start index, current cut1 id.
*/// ouput: current cut1's c1cpr.
const TCoord TreeSearch::getC1cpr(const List<TreeNode> &sol, const int index, const TID cur_plate, const TID cur_cut1) const {
    TCoord res = 0;
    for (int i = index; i < sol.size(); ++i) {
        if (cur_plate == sol[i].plate && cur_cut1 == sol[i].cut1) {
            if (res < sol[i].c1cpr)
                res = sol[i].c1cpr;
        } else {
            break;
        }
    }
    return res;
}

/* input: solution TreeNode type, solution start index, current cut2 id.
*/// ouput: current cut2's c2cpu.
const TCoord TreeSearch::getC2cpu(const List<TreeNode> &sol, const int index, const TID cur_cut1, const TID cur_cut2) const {
    TCoord res = 0;
    for (int i = index; i < sol.size(); ++i) {
        if (cur_cut1 == sol[i].cut1 && cur_cut2 == sol[i].cut2) {
            if (res < sol[i].c2cpu)
                res = sol[i].c2cpu;
        } else {
            break;
        }
    }
    return res;
}

bool TreeSearch::check(Length & checkerObj) const {
    #if SZX_DEBUG
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        ItemProductionError = 0x2,
        DefectSuperposingError = 0x4,
        ItemDimensionError = 0x8,
        IdentityError = 0x10,
        SequenceError = 0x20
    };

    checkerObj = System::exec("Checker.exe " + env.instName + " " + env.solutionPath());
    if (checkerObj > 0) { return true; }
    checkerObj = ~checkerObj;
    if (checkerObj == CheckerFlag::IoError) { Log(LogSwitch::Checker) << "IoError." << endl; }
    if (checkerObj & CheckerFlag::FormatError) { Log(LogSwitch::Checker) << "FormatError." << endl; }
    if (checkerObj & CheckerFlag::ItemProductionError) { Log(LogSwitch::Checker) << "ItemProductionError." << endl; }
    if (checkerObj & CheckerFlag::DefectSuperposingError) { Log(LogSwitch::Checker) << "DefectSuperposingError." << endl; }
    if (checkerObj & CheckerFlag::ItemDimensionError) { Log(LogSwitch::Checker) << "ItemDimensionError." << endl; }
    if (checkerObj & CheckerFlag::IdentityError) { Log(LogSwitch::Checker) << "IdentityError." << endl; }
    if (checkerObj & CheckerFlag::SequenceError) { Log(LogSwitch::Checker) << "SequenceError." << endl; }
    return false;
    #else
    checkerObj = 0;
    return true;
    #endif // SZX_DEBUG
}

/* return the rate of total waste L1 in every plate. */
const double TreeSearch::getScrapWasteRate(List<TreeNode>& sol) const {
    if (sol.size() == 0)
        return 0.0;
    Length scrap_length = 0;
    TID cur_plate = 0;
    for (int i = 1; i < sol.size(); ++i) {
        if (sol[i].plate > cur_plate) {
            scrap_length += (input.param.plateWidth - sol[i - 1].c1cpr);
            cur_plate++;
        }
    }
    return (double)scrap_length / (double)(sol.back().plate*input.param.plateWidth + sol.back().c1cpr);
}

#pragma endregion Achievement

}