#include <iostream>
#include "TopSearch.h"
#include "ThreadPool.h"
#include "LogSwitch.h"

using namespace std;

namespace GV {  // global variables
	List<Rect> items;
	List<Area> item_area;
	List<List<TID>> stacks; // stacks[s][i] is the `i`_th item in stack `s`.
	List<TID> item2stack;   // item2stack[i] is the stack of item `i`.
	List<List<Defect>> plate_defect_x; // plate_defect_x[p][i] is the `i`_th defect on plate p, sorted by defect x position.
	List<List<Defect>> plate_defect_y; // plate_defect_y[p][i] is the `i`_th defect on plate p, sorted by defect y position.
	IdMap idMap;
	Problem::Input::Param param;
	int item_num;
	int stack_num;
	int support_thread;
}


void TopSearch::solve() {
	this->init();
	this->run();
	this->toOutput();
}


void TopSearch::init() {
	constexpr TID InvalidItemId = Problem::InvalidItemId;

	GV::items.reserve(input.batch.size());
	GV::stacks.reserve(input.batch.size());
	GV::item2stack.resize(input.batch.size());
	// assign internal id to items and stacks, then push items into stacks.
	for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
		TID itemId = GV::idMap.item.toConsecutiveId(i->id);
		GV::items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
		if (itemId != i->id) {
			Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
		}

		TID stackId = GV::idMap.stack.toConsecutiveId(i->stack);
		if (GV::stacks.size() <= stackId) { GV::stacks.push_back(List<TID>()); } // create a new stack.
		// OPTIMIZE[szx][6]: what if the sequence number could be negative or very large?
		List<TID> &stack(GV::stacks[stackId]);
		if (stack.size() <= i->seq) { stack.resize(i->seq + 1, InvalidItemId); }
		stack[i->seq] = itemId;
		GV::item2stack[itemId] = stackId;
	}
	// clear invalid items in stacks.
	for (auto s = GV::stacks.begin(); s != GV::stacks.end(); ++s) {
		s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
	}

	// EXTEND[szx][9]: make sure that the plate IDs are already zero-base consecutive numbers.
	GV::plate_defect_x.resize(Problem::MaxPlateNum);
	GV::plate_defect_y.resize(Problem::MaxPlateNum);
	for (TID p = 0; p < Problem::MaxPlateNum; ++p) { GV::idMap.plate.toConsecutiveId(p); }

	// map defects to plates.
	for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
		TID defectId = GV::idMap.defect.toConsecutiveId(d->id);
		TID plateId = GV::idMap.plate.toConsecutiveId(d->plateId);
		if (GV::plate_defect_x.size() <= plateId) { GV::plate_defect_x.resize(plateId + 1); } // create new plates.
		GV::plate_defect_x[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
		if (GV::plate_defect_y.size() <= plateId) { GV::plate_defect_y.resize(plateId + 1); } // create new plates.
		GV::plate_defect_y[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
	}

	GV::item_area.reserve(GV::items.size());
	//calculate item areas.
	for (int i = 0; i < GV::items.size(); ++i) {
		GV::item_area.push_back(GV::items[i].h*GV::items[i].w);
		total_item_area += GV::items[i].h*GV::items[i].w;
	}
	// reverse item sequence convicent for pop_back.
	for (int i = 0; i < GV::stacks.size(); i++)
		reverse(GV::stacks[i].begin(), GV::stacks[i].end());
	// sort defects by it's x position.
	for (int p = 0; p < GV::plate_defect_x.size(); ++p)
		sort(GV::plate_defect_x[p].begin(), GV::plate_defect_x[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.x < rhs.x; });
	// sort defects by it's y position.
	for (int p = 0; p < GV::plate_defect_y.size(); ++p)
		sort(GV::plate_defect_y[p].begin(), GV::plate_defect_y[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.y < rhs.y; });
	
	// init GV param
	GV::param = input.param;
	GV::item_num = int(GV::items.size());
	GV::stack_num = int(GV::stacks.size());
	GV::support_thread = thread::hardware_concurrency();
	// init this
	this->best_objective = GV::param.plateNum * GV::param.plateWidth;
	this->source_batch.reserve(GV::stack_num);
	for (int i = 0; i < GV::stack_num; ++i) { this->source_batch.emplace_back(0, int(GV::stacks[i].size())); }
}


void TopSearch::run() {
	MySolution fixed_plate_sol; fixed_plate_sol.reserve(GV::item_num);
	List<MySolution> target_sols;
	List<List<MyStack>> eval_batchs;
	List<MySolution> eval_sols;
	List<LengthPair> eval_length;
	
	// 循环每次固定一块 Plate
	TID cur_plate = 0;
	while (fixed_plate_sol.size() < GV::item_num) { 
		if (timer.isTimeOut()) { break; }

		PlateSearch plate_search(this->cfg,this->rand, this->timer, cur_plate);
		plate_search.branchOnePlate(cfg.mbpn * 2, cfg.mbpn, this->source_batch, target_sols);
		if (target_sols.empty()) { continue; }
		int branch_num = int(target_sols.size());

		eval_batchs.clear(); eval_batchs.resize(branch_num, this->source_batch);
		eval_sols.clear(); eval_sols.resize(branch_num, fixed_plate_sol);
		eval_length.clear(); eval_length.resize(branch_num);
		ThreadPool tp(GV::support_thread);
		for (int i = 0; i < branch_num; ++i) {
			tp.push([&, i]() { 
				for (auto &node : target_sols[i]) {
					eval_sols[i].push_back(node);
					eval_batchs[i][GV::item2stack[node.item]].pop();
				}
				eval_length[i] = make_pair(i, evaluateOnePlate(cur_plate + 1, eval_batchs[i], eval_sols[i]));
			});
		}
		tp.pend();
		auto min_length = min_element(eval_length.begin(), eval_length.end(), [](LengthPair &lhs, LengthPair &rhs) { return lhs.second < rhs.second; });
		int best_index = min_length->first;
		Length best_length = min_length->second;
		
		if (this->best_objective > best_length) {
			this->best_objective = best_length;
			this->best_solution = eval_sols[best_index];
			Log(Log::Debug) << "a better obj:" << best_objective << endl;

			for (auto &node : target_sols[best_index]) {
				fixed_plate_sol.push_back(node);
				this->source_batch[GV::item2stack[node.item]].pop();
			}
			Log(Log::Debug) << "fix plate:" << cur_plate << endl;
			++cur_plate;
		}
	}
}


Length TopSearch::evaluateOnePlate(TID cur_plate, List<MyStack>& batch, MySolution &comp_sol) {
	/*
	* 评价一个 Plate 的切割方案
	* @Input: fixed_sol（已固定 item），psol（新增 1-cut 解）
	* @Return: 使用的 Plate 总宽度
	*/

	// comp_sol 解不完整，继续构造一个完整的解
	MySolution cur_plate_sol;
	while (comp_sol.size() < GV::item_num) {
		if (timer.isTimeOut()) { break; }

		PlateSearch plate_search(this->cfg, this->rand, this->timer, cur_plate);
		plate_search.optimizeOnePlate(batch, cur_plate_sol);
		for (auto &node : cur_plate_sol) {
			comp_sol.push_back(node);
			batch[GV::item2stack[node.item]].pop();
		}
		++cur_plate;
	}
	if (comp_sol.size() != GV::item_num) { return GV::param.plateWidth * GV::param.plateNum; }

	return cur_plate * GV::param.plateWidth + comp_sol.back().c1cpr;
}
