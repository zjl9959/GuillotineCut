#include "PlateSearch.h"
#include "ThreadPool.h"

using namespace std;

namespace szx{

// ���ؿ����������㷨��max_iter�������ڵ������չ��Ŀ
Score PlateSearch::mcts(List<List<TID>>& batch, List<SolutionNode>& sol, int max_iter) {
    // [zjl][TODO]: add code.
    return Score();
}

// ��batch���������һ��������Ʒ���У�����֤�����к�node�ӽڵ��е���Ʒ���в�ͬ��
bool PlateSearch::getItemSequence(int seq_len, MCTNode *node, List<List<TID>>& batch, List<TID>& seq) {
    int left_item = 0;
    for (int i = 0; i < batch.size(); ++i)
        left_item += batch[i].size();
    seq_len = min(seq_len, left_item);  // seq_len������batch��ʣ����Ʒ��
    for (int i = 0; i < seq_len; ++i) {
        List<bool> item_hash(1000, false);
        // ������ϣ����¼node�����ӽڵ���λ��seqλ�õ���Ʒid
        for (MCTNode *child : node->get_childrens()) {
            item_hash[child->get_solution_nodes()[i].item] = true;
        }
        // ����ջ�ף����ѡ��һ�����ظ�����Ʒ������seq��
        int samp_count = 1;     // ˮ����������
        int choose_batch = -1;  // ѡ���batch����
        for (int i = 0; i < batch.size(); ++i) {
            if (!batch[i].empty() && item_hash[batch[i].back()] == false &&
                rand_.pick(samp_count) == 0) {
                choose_batch = i;
                samp_count++;
            }
        }
        if (choose_batch >= 0) {    // ��ѡ�����Ʒ��ӽ�seq
            seq.push_back(batch[choose_batch].back());
            batch[choose_batch].pop_back();
        } else {
            break;
        }
    }
    return seq_len == seq.size();
}

void PlateSearch::createItemBatchs(int nums, const SolutionNode & resume_point, const List<MyStack>& source_batch, List<List<MyStack>>& target_batchs) {
	/*
	* �����ѡ��Ʒ��������֧����ͬ����Ʒ��ϣ�
	* @Input: nums�����Է�֧����, resume_point����֧��㣩, source_batch����ѡ item ���ϣ�
	* @Return: target_batchs[b][s]����֧ b��ջ s
	*/
	// ����Ҫ�����Ʒ�����ѡ�� init_candidate_items
	TLength left_plate_width = GV::param.plateWidth - resume_point.c1cpr;
	List<TID> init_candidate_items; init_candidate_items.reserve(GV::stack_num);
	for (int i = 0; i < GV::stack_num; ++i) {
		if (source_batch[i].empty()) { continue; }
		TID top_item = GV::stacks[i][source_batch[i].top()];
		if (GV::items[top_item].h < left_plate_width) { init_candidate_items.push_back(top_item); }
	}
	if (init_candidate_items.empty()) { return; }

	CombinationCache comb_cache; int try_num = 0;
	while (target_batchs.size() < nums && try_num < nums) {
		List<MyStack> temp_batch; temp_batch.reserve(GV::stack_num); // ��ʼ����ջ
		for (int i = 0; i < GV::stack_num; ++i) { temp_batch.emplace_back(source_batch[i].end, source_batch[i].end); }
		vector<bool> items_set(GV::item_num, false);  // ���������Ʒδѡ��
		List<TID> candidate_items(init_candidate_items);
		int chosen_items_num = 0;
		while (candidate_items.size() && chosen_items_num < cfg_.mcin) {
			++chosen_items_num;
			int chosen_index = rand_.pick(candidate_items.size());
			TID chosen_item  = candidate_items[chosen_index]; items_set[chosen_item] = true;
			TID chosen_stack = GV::item2stack[chosen_item]; 
			int temp_bottom  = temp_batch[chosen_stack].begin - 1; // �� top �� bottom ��չһ����Ʒ
			if (temp_bottom < source_batch[chosen_stack].bottom()) { // �� ջȡ�գ�����λ��
				candidate_items.erase(candidate_items.begin() + chosen_index);
			}
			else {
				--temp_batch[chosen_stack].begin;
				TID bottom_item = GV::stacks[chosen_stack][temp_bottom];
				if (GV::items[bottom_item].h < left_plate_width) { // �� �� bottom_item ���� chosen_index λ����Ϊ�µĺ�ѡ
					candidate_items[chosen_index] = bottom_item;
				}
				else { // �� ���ܼ�����ջ������λ��
					candidate_items.erase(candidate_items.begin() + chosen_index);
				}
			}
		}
		if (!comb_cache.get(items_set)) {
			comb_cache.set(items_set);
			target_batchs.push_back(temp_batch);
			try_num = 0;
		}
		else { try_num++; } // �ظ�
	}
}

ScorePair PlateSearch::branchAndGetBestOneCutSol(int nums, const SolutionNode & resume_point, const List<MyStack>& source_batch, List<List<MyStack>>& target_batchs, List<MySolution>& target_sols, bool tail){
	/*
	* ������֧���������֧�� 1-cut �⣬������������ʽ���±�
	* @Input: nums�����Է�֧����, resume_point����֧��㣩, source_batch����ѡ item ���ϣ���tail���Ƿ�Ϊ��� 1-cut��
	* @Return: target_batchs[b][s]����֧����target_sols[b]����֧�⣩��best_index����������ʽ���±꣩
	*/
	ThreadPool tp(GV::support_thread);

	target_batchs.clear(); target_batchs.reserve(nums);
	createItemBatchs(nums, resume_point, source_batch, target_batchs);
	if (target_batchs.empty()) { return InvalidPair; } // �� �޷�������֧�����Ͽ�����ƣ�
	
	int branch_num = int(target_batchs.size());
	target_sols.clear(); target_sols.resize(branch_num);
	List<ScorePair> scores(branch_num);
	CutSearch cut_search(plate_, resume_point.c1cpr);
	for (int i = 0; i < branch_num; ++i) {
		List<List<TID>> recover_stacks; recover_stacks.reserve(GV::stack_num);
		for (int j = 0; j < GV::stack_num; ++j) { 
			 List<TID> recover_stack = target_batchs[i][j].recoverStack(j); 
			 if (recover_stack.size()) { recover_stacks.push_back(recover_stack); }
		}
		scores[i] = make_pair(i, cut_search.dfs(recover_stacks, target_sols[i], tail));
	}
	//for (int i = 0; i < branch_num; ++i) {
	//	tp.push([&, i]() {
	//		List<List<TID>> recover_stacks(GV::stack_num);
	//		for (int j = 0; j < GV::stack_num; ++j) { recover_stacks[j] = target_batchs[i][j].recoverStack(j); }
	//		scores[i] = make_pair(i, cut_search.dfs(recover_stacks, target_sols[i], tail));
	//	});
	//}
	//tp.pend();
	auto max_score = max_element(scores.begin(), scores.end(), [](ScorePair &lhs, ScorePair &rhs) { return lhs.second < rhs.second; });
	if (target_sols[max_score->first].empty()) { return InvalidPair; }  // �� ���з�֧���޷�������һ��Ʒ��覴����ƣ�
	return *max_score;
}

void PlateSearch::optimizeOnePlate(const List<MyStack>& source_batch, MySolution & plate_sol) {
	/*
	* ��һ�� Plate �㵽����
	* @Input: source_batch����ѡ item ���ϣ�
	* @Return: plate_sol����ǰ Plate �������Ž⣩
	*
	*		+-----+--+--+-----------+
	*		|     |  |  |           |
	*		+--+--+--+--+           |
	*		|  |  |     |           |
	*		+--+--+-----+-----------+
	*       |1-cut|2-cut|
	*       | fixed_sol |
	*
	*/
	
	// target_batchs[b][s]����֧ b��ջ s
	// target_sols[b]����֧ b �� 1-cut ��
	List<List<MyStack>> target_batchs;
	List<MySolution> target_sols; 
	// eval_batchs[b][s]����֧ b��ջ s
	// eval_sols[b]����֧ b ���� target_sols[b] �� Plate ��
	// eval_areas[b]����֧ b �����֣�ÿ�����ֽⲹȫΪ������������
	List<List<MyStack>> eval_batchs;
	List<MySolution> eval_sols;
	List<AreaPair> eval_areas;
	
	List<MyStack> batch(source_batch);
	MySolution fixed_sol;         // �ѹ̶��� item 
	SolutionNode resume_point(0); // �������
	TID cur_cut1 = 0;             // ��ǰ 1-cut id
	Area max_item_area = 0;       // ������������½�
	ThreadPool tp(GV::support_thread);
	
	while (1) {
		if (timer_.isTimeOut()) { break; }

		if (InvalidPair == branchAndGetBestOneCutSol(cfg_.mbcn, resume_point, batch, target_batchs, target_sols)) { break; }
		int branch_num = int(target_batchs.size());
		// ����������ֽ⣬evaluateOneCut()��ȫ�ɵ�ǰ Plate �������⣬�̶���õĲ��ֽ�
		eval_batchs.clear(); eval_batchs.resize(branch_num, batch);
		eval_sols.clear(); eval_sols.resize(branch_num, fixed_sol);
		eval_areas.clear(); eval_areas.resize(branch_num);
		for (int i = 0; i < branch_num; ++i) {
			tp.push([&, i]() {
				for (auto &node : target_sols[i]) {
					eval_batchs[i][GV::item2stack[node.item]].pop();
					eval_sols[i].push_back(node);
				}
				eval_areas[i] = make_pair(i, evaluateOneCut(eval_batchs[i], eval_sols[i]));
			});
		}
		tp.pend();
		auto max_area = max_element(eval_areas.begin(), eval_areas.end(), [](AreaPair &lhs, AreaPair &rhs) { return lhs.second < rhs.second; });
		int best_index = max_area->first;
		Area best_area = max_area->second;
		
		// �ѹ̶� item ���£�batch ��ջ
		for (int i = 0; i < target_sols[best_index].size(); ++i) {
			fixed_sol.push_back(target_sols[best_index][i]);
			batch[GV::item2stack[target_sols[best_index][i].item]].pop();
		}
		// �����������
		resume_point = fixed_sol.back();
		resume_point.c1cpl = resume_point.c1cpr;
		resume_point.c2cpb = resume_point.c2cpu = 0;
		// ������������������Ž�
		if (max_item_area < best_area) {
			max_item_area = best_area;
			plate_sol = eval_sols[best_index];
		}
		cur_cut1++; // next 1-cut
	}
}

void PlateSearch::branchAndGetSomePlateSols(int try_num, int real_num, const List<MyStack>& source_batch, List<MySolution>& plate_sols) {
	/*
	@���ɵ�ǰ Plate �Ķ���Ϻõ�ƽ���
	@Input: try_nums(���Է�֧��Ŀ)��real_nums�������Ϻõķ�֧������source_batch����ѡ item ���ϣ�
	@Return: plate_sols[]
	*/

	// target_batchs[b][s]����֧ b��ջ s
	// target_sols[b]��
	List<List<MyStack>> target_batchs;
	List<MySolution> target_sols;
	SolutionNode resume_point(0);
	ThreadPool tp(GV::support_thread);

	if (InvalidPair == branchAndGetBestOneCutSol(try_num, resume_point, source_batch, target_batchs, target_sols)) { return; }
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
	plate_sols.clear(); plate_sols.resize(real_num);
	for (int i = 0; i < real_num; ++i) { plate_sols[i] = target_sols[eval_areas[i].first]; }
}

Area PlateSearch::evaluateOneCut(List<MyStack>& batch, MySolution & psol) {
	/*
	* �����ѷ��õĲ��ֽ⣬��ȫ�� Plate �⣬���ط�����Ʒ�������
	* @Input: batch����ѡ item ���ϣ���psol���ѷ��ò��ֽ⣩
	* @Return: psol����ǰ Plate �����⣩
	*/
	if (psol.empty()) { return 0; }

	// target_batchs[b][s]����֧ b��ջ s
	// target_sols[b]��
	List<List<MyStack>> target_batchs;
	List<MySolution> target_sols;
	TID cur_cut1 = 0;        // ��ǰ 1-cut id
	MySolution last_par_sol; // ��� 1-cut ���ֽ�
	Score last_par_score;    // ��� 1-cut ������
	// �����������
	SolutionNode resume_point = psol.back();
	resume_point.c1cpl = resume_point.c1cpr;
	resume_point.c2cpb = resume_point.c2cpu = 0;
	
	while (1) {
		if (timer_.isTimeOut()) { break; }

		ScorePair best_pair = branchAndGetBestOneCutSol(cfg_.mbcn, resume_point, batch, target_batchs, target_sols);
		if (best_pair == InvalidPair) { break; }
		last_par_sol   = target_sols[best_pair.first];
		last_par_score = best_pair.second;
		// ���� psol������ӳ�������
		for (auto &node : last_par_sol) {
			psol.push_back(node);
			batch[GV::item2stack[node.item]].pop();
		}
		// �����������
		resume_point = psol.back();
		resume_point.c1cpl = resume_point.c1cpr;
		resume_point.c2cpb = resume_point.c2cpu = 0;
		++cur_cut1;
	}

	Area placed_item_area = 0;
	for (auto &node : psol) { placed_item_area += GV::item_area[node.item]; }
	if (cur_cut1 < 1) { return placed_item_area; }
	
	// �����������Ϊ last_par_sol ǰһ�� item.
	int last_par_sol_begin_index = int(psol.size()) - int(last_par_sol.size());
	resume_point = psol[last_par_sol_begin_index - 1];
	resume_point.c1cpl = resume_point.c1cpr;
	resume_point.c2cpb = resume_point.c2cpu = 0;
	// �����Ż���� 1-cut.
	for (auto &node : last_par_sol) { batch[GV::item2stack[node.item]].push(); }
	ScorePair best_pair = branchAndGetBestOneCutSol(cfg_.mbcn, resume_point, batch, target_batchs, target_sols, true);
	if (best_pair != InvalidPair && (best_pair.second - last_par_score) > 0.001) {
		last_par_sol = target_sols[best_pair.first];
		psol.erase(psol.begin() + last_par_sol_begin_index, psol.end());
		for (auto &node : last_par_sol) { psol.push_back(node); }
		placed_item_area = 0;
		for (auto &node : psol) { placed_item_area += GV::item_area[node.item]; }
	}

	return placed_item_area;
}

}