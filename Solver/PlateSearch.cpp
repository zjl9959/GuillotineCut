#include "PlateSearch.h"
#include "ThreadPool.h"

using namespace std;
using namespace szx;

// 蒙特卡洛树搜索算法，max_iter代表树节点最大扩展数目
Score PlateSearch::mcts(List<List<TID>>& batch, List<SolutionNode>& sol, int max_iter) {
    // [zjl][TODO]: add code.
    return Score();
}

// 从batch中随机产生一个可行物品序列，并保证该序列和node子节点中的物品序列不同。
bool PlateSearch::getItemSequence(int seq_len, MCTNode *node, List<List<TID>>& batch, List<TID>& seq) {
    int left_item = 0;
    for (int i = 0; i < batch.size(); ++i)
        left_item += batch[i].size();
    seq_len = min(seq_len, left_item);  // seq_len不大于batch中剩余物品数
    for (int i = 0; i < seq_len; ++i) {
        List<bool> item_hash(1000, false);
        // 构建哈希，记录node所有子节点中位于seq位置的物品id
        for (MCTNode *child : node->get_childrens()) {
            item_hash[child->get_solution_nodes()[i].item] = true;
        }
        // 遍历栈首，随机选择一个非重复的物品，放入seq中
        int samp_count = 1;     // 水塘抽样计数
        int choose_batch = -1;  // 选择的batch索引
        for (int i = 0; i < batch.size(); ++i) {
            if (!batch[i].empty() && item_hash[batch[i].back()] == false &&
                rand_.pick(samp_count) == 0) {
                choose_batch = i;
                samp_count++;
            }
        }
        if (choose_batch >= 0) {    // 将选择的物品添加进seq
            seq.push_back(batch[choose_batch].back());
            batch[choose_batch].pop_back();
        } else {
            break;
        }
    }
    return seq_len == seq.size();
}

int PlateSearch::createItemBatchs(int nums, const SolutionNode & resume_point, const List<MyStack>& source_batch, List<List<MyStack>>& target_batchs) {
	/*
	* 创建分支
	* @Input: nums（尝试分支数）, resume_point（分支起点）, source_batch（候选 item 集合）
	* @Return: 实际分支数目；target_batchs[b][s]：分支 b，栈 s
	*/
	
	// 符合要求的 item 存入候选表
	TLength left_plate_width = GV::param.plateWidth - resume_point.c1cpr;
	List<TID> init_candidate_items; init_candidate_items.reserve(GV::stack_num); // 初始候选表
	for (int i = 0; i < GV::stack_num; ++i) {
		if (source_batch[i].empty()) { continue; }
		TID top_item = GV::stacks[i][source_batch[i].top()];
		if (GV::items[top_item].h < left_plate_width) { init_candidate_items.push_back(top_item); }
	}
	if (init_candidate_items.empty()) { return 0; }

	CombinationCache comb_cache;
	target_batchs.clear(); target_batchs.reserve(nums);
	int try_num = 0;
	while (target_batchs.size() < nums && try_num < nums) {
		List<MyStack> temp_batch(GV::stack_num); // 空栈，从 top 向 bottom 扩展
		for (int i = 0; i < GV::stack_num; ++i) { temp_batch[i] = MyStack(source_batch[i].end, source_batch[i].end); }
		vector<bool> items_set(GV::item_num, false);  // 标记所有 item 未选中
		List<TID> candidate_items(init_candidate_items);
		int chosen_items_num = 0;
		while (chosen_items_num < cfg_.mcin) { // cfg.mcin: 配置最多可选择的 item 数目
			if (candidate_items.empty()) { break; }
			int chosen_index = rand_.pick(candidate_items.size());
			TID chosen_item  = candidate_items[chosen_index]; items_set[chosen_item] = true;
			TID chosen_stack = GV::item2stack[chosen_item]; 
			int temp_bottom  = --temp_batch[chosen_stack].begin;
			if (temp_bottom < source_batch[chosen_stack].bottom()) { // ① 栈取空，擦除位置
				candidate_items.erase(candidate_items.begin() + chosen_index);
			}
			else {
				TID bottom_item = GV::stacks[chosen_stack][temp_bottom];
				if (GV::items[bottom_item].h < left_plate_width) { // 将 bottom_item 填入 chosen_index 位置作为新的候选
					candidate_items[chosen_index] = bottom_item;
				}
				else { // ② 不能继续弹栈，擦除位置
					candidate_items.erase(candidate_items.begin() + chosen_index);
				}
			}
			chosen_items_num++;
		}

		if (!comb_cache.get(items_set)) {
			comb_cache.set(items_set);
			target_batchs.push_back(temp_batch);
			try_num = 0;
		}
		else { try_num++; } // 重复
	}
	return int(target_batchs.size());
}

ScorePair PlateSearch::getBestTargetSol(int nums, const SolutionNode & resume_point, const List<MyStack>& source_batch, List<List<MyStack>>& target_batchs, List<MySolution>& target_sols, bool tail){
	/*
	* 创建分支，并求各分支的部分解，返回最大利用率解的下标
	* @Input: nums（尝试分支数）, resume_point（分支起点）, source_batch（候选 item 集合）
	* @Return: target_batchs[b][s]（分支），target_sols[b]（对应的部分解），best_index（最大利用率解的下标）
	*/
	ThreadPool tp(GV::support_thread);

	target_batchs.clear(); target_batchs.reserve(nums);
	int branch_num = createItemBatchs(nums, resume_point, source_batch, target_batchs);
	if (branch_num == 0) { return make_pair(-1, 0.0); } // ①：无法继续分支（余料宽度限制）
	
	target_sols.clear(); target_sols.resize(branch_num);
	List<ScorePair> scores(branch_num);
	CutSearch cut_search(plate_, resume_point.c1cpr);
	for (int i = 0; i < branch_num; ++i) {
		tp.push([&, i]() {
			List<List<TID>> recover_stacks(GV::stack_num);
			for (int j = 0; j < GV::stack_num; ++j) { recover_stacks[j] = target_batchs[i][j].recoverStack(j); }
			scores[i] = make_pair(i, cut_search.dfs(recover_stacks, target_sols[i], tail));
		});
	}
	tp.pend();
	auto max_score = max_element(scores.begin(), scores.end(), [](ScorePair &lhs, ScorePair &rhs) { return lhs.second < rhs.second; });
	if (target_sols[max_score->first].empty()) { return make_pair(-1, 0.0); }  // ②：CutSearch 在所有分支都无法放下任一 item（宽度、瑕疵限制）
	return *max_score;
}

void PlateSearch::optimizeOnePlate(const List<MyStack>& source_batch, MySolution & psol) {
	/*
	* 将一块 Plate 算到最优
	* @Input: source_batch（候选 item 集合）
	* @Return: psol（当前 Plate 完整最优解）
	*/
	
	// target_batchs[b][s]：分支 b，栈 s
	// target_sols[b]：分支 b 的 1-cut 部分解
	List<List<MyStack>> target_batchs;
	List<MySolution> target_sols; 
	// eval_areas[b]：分支 b 的评分：每个部分解补全为完整解的总面积
	// eval_batchs[b][s]：分支 b，栈 s
	// eval_sols[b]：分支 b 基于 target_sols[b] 的完整解
	List<AreaPair> eval_areas;
	List<List<MyStack>> eval_batchs;
	List<MySolution> eval_sols;
	
	List<MyStack> batch(source_batch);
	MySolution fixed_sol;         // 已固定的 item 
	SolutionNode resume_point(0); // 搜索起点
	TID cur_cut1 = 0;             // 当前 1-cut id
	Area max_item_area = 0;       // 最大放置面积，下界
	ThreadPool tp(GV::support_thread);
	
	/*
	*		+-----+--+--+-----------+
	*		|     |  |  |           |
	*		+--+--+--+--+           |
	*		|  |  |     |           |
	*		+--+--+-----+-----------+
	*       |1-cut|2-cut|
	*       | fixed_sol |
	*
	*/
	while (1) {
		if (timer_.isTimeOut()) { break; }

		// 得到多个部分解并返回最好的
		ScorePair best_pair = getBestTargetSol(cfg_.mhcn, resume_point, batch, target_batchs, target_sols);
		if (best_pair.first == -1) { break; }
		int branch_num = int(target_batchs.size());
		// 评估多个部分解，evaluateOneCut()补全成当前 Plate 的完整解，固定最好的部分解
		eval_batchs.clear(); eval_batchs.resize(branch_num, batch);
		eval_sols.clear(); eval_sols.resize(branch_num, fixed_sol);
		eval_areas.clear(); eval_areas.resize(branch_num);
		for (int i = 0; i < branch_num; ++i) {
			tp.push([&, i]() {
				for (auto &node : target_sols[i]) {
					eval_sols[i].push_back(node);
					eval_batchs[i][GV::item2stack[node.item]].pop();
				}
				eval_areas[i] = make_pair(i, evaluateOneCut(eval_batchs[i], eval_sols[i]));
			});
		}
		tp.pend();
		auto max_area = max_element(eval_areas.begin(), eval_areas.end(), [](AreaPair &lhs, AreaPair &rhs) { return lhs.second < rhs.second; });
		int best_index = max_area->first;
		Area best_area = max_area->second;
		
		// 已固定 item 更新，batch 弹栈
		for (int i = 0; i < target_sols[best_index].size(); ++i) {
			fixed_sol.push_back(target_sols[best_index][i]);
			batch[GV::item2stack[target_sols[best_index][i].item]].pop();
		}
		// 更新搜索起点
		resume_point = fixed_sol.back();
		resume_point.c1cpl = resume_point.c1cpr;
		resume_point.c2cpb = resume_point.c2cpu = 0;
		// 更新最大放置面积、最优解
		if (max_item_area < best_area) {
			max_item_area = best_area;
			psol = eval_sols[best_index];
		}
		cur_cut1++; // next 1-cut
	}
}

void PlateSearch::branchOnePlate(int try_num, int real_num, const List<MyStack>& source_batch, List<MySolution>& psols) {
	/*
	@生成当前平面的多个完整解
	@Input: try_nums(尝试分支数目)，real_nums（实际分支数），source_batch（候选 item 集合）
	@Return: psols[]（当前平面多个完整解）
	*/

	// target_batchs[b][s]：分支 b，栈 s
	// target_sols[b]：分支 b 的 1-cut 部分解
	List<List<MyStack>> target_batchs;
	List<MySolution> target_sols;
	SolutionNode resume_point(0); // 搜索起点
	ThreadPool tp(GV::support_thread);

	ScorePair best_pair = getBestTargetSol(try_num, resume_point, source_batch, target_batchs, target_sols);
	if (best_pair.first == -1) { return; }
	int branch_num = int(target_batchs.size());

	List<AreaPair> eval_areas(branch_num);
	List<List<MyStack>> eval_batchs(branch_num, source_batch);
	for (int i = 0; i < branch_num; ++i) {
		tp.push([&, i]() {
			for (auto &node : target_sols[i]) { eval_batchs[i][GV::item2stack[node.item]].pop(); }
			eval_areas[i] = make_pair(i, evaluateOneCut(eval_batchs[i], target_sols[i]));
		});
	}
	tp.pend();
	sort(eval_areas.begin(), eval_areas.end(), [](AreaPair &lhs, AreaPair &rhs) { return lhs.second > rhs.second; });
	real_num = real_num > branch_num ? branch_num : real_num;
	psols.clear(); psols.resize(real_num);
	for (int i = 0; i < real_num; ++i) { psols[i] = target_sols[eval_areas[i].first]; }
}

Area PlateSearch::evaluateOneCut(List<MyStack>& batch, MySolution & psol) {
	/*
	* 评估部分解，补全成当前 Plate 的完整解，返回放置物品的总面积
	* @Input: batch（候选 item 集合），psol（当前 Plate 部分解 + fixed 部分）
	* @Return: psol（当前 Plate 完整解）
	*/
	if (psol.empty()) { return 0; }

	// target_batchs[i][j]：分支 i，栈 j
	// target_sols[i]：分支 i 的 1-cut 部分解
	List<List<MyStack>> target_batchs;
	List<MySolution> target_sols;
	
	// 更新搜索起点
	SolutionNode resume_point = psol.back();
	resume_point.c1cpl = resume_point.c1cpr;
	resume_point.c2cpb = resume_point.c2cpu = 0;
	TID cur_cut1 = 0;        // 当前 1-cut id
	MySolution last_par_sol; // 最后 1-cut 部分解
	Score last_par_score;    // 最后 1-cut 利用率
	
	while (1) {
		if (timer_.isTimeOut()) { break; }

		ScorePair best_pair = getBestTargetSol(cfg_.mhcn, resume_point, batch, target_batchs, target_sols);
		if (best_pair.first == -1) { break; }
		last_par_sol   = target_sols[best_pair.first];
		last_par_score = best_pair.second;
		
		// 更新 psol，逐渐添加成完整解
		for (auto &node : last_par_sol) {
			psol.push_back(node);
			batch[GV::item2stack[node.item]].pop();
		}
		resume_point = psol.back();
		resume_point.c1cpl = resume_point.c1cpr;
		resume_point.c2cpb = resume_point.c2cpu = 0;
		cur_cut1++;
	}

	Area placed_item_area = 0;
	for (auto &node : psol) { placed_item_area += GV::item_area[node.item]; }
	if (cur_cut1 < 1) { return placed_item_area; }
	
	// 更新搜索起点为 last_par_sol 前一块 item.
	int last_par_sol_begin_index = int(psol.size()) - int(last_par_sol.size());
	resume_point = psol[last_par_sol_begin_index - 1];
	resume_point.c1cpl = resume_point.c1cpr;
	resume_point.c2cpb = resume_point.c2cpu = 0;
	// 重新优化最后 1-cut.
	for (auto &node : last_par_sol) { batch[GV::item2stack[node.item]].push(); }
	ScorePair best_pair = getBestTargetSol(cfg_.mhcn, resume_point, batch, target_batchs, target_sols, true);
	if (best_pair.first != -1 && (best_pair.second - last_par_score) > 0.001) {
		last_par_sol = target_sols[best_pair.first];
		psol.erase(psol.begin() + last_par_sol_begin_index, psol.end());
		for (auto &node : last_par_sol) { psol.push_back(node); }
		placed_item_area = 0;
		for (auto &node : psol) { placed_item_area += GV::item_area[node.item]; }
	}

	return placed_item_area;
}
