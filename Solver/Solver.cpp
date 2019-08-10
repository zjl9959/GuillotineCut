#include "Solver.h"

#include <iostream>
#include <fstream>
#include "LogSwitch.h"
#include "ThreadPool.h"

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

namespace szx {

#pragma region cli
int Cli::run(int argc, char * argv[]) {
    Log(LogSwitch::Szx::Cli) << "parse command line arguments." << endl;
    Set<String> switchSet;
    Map<String, char*> optionMap({ // use string as key to compare string contents instead of pointers.
        { InstancePathOption(), nullptr },
        { SolutionPathOption(), nullptr },
        { RandSeedOption(), nullptr },
        { TimeoutOption(), nullptr },
        { MaxIterOption(), nullptr },
        { JobNumOption(), nullptr },
        { RunIdOption(), nullptr },
        { EnvironmentPathOption(), nullptr },
        { ConfigPathOption(), nullptr },
        { LogPathOption(), nullptr }
        });

    for (int i = 1; i < argc; ++i) {    // skip executable name.
        auto mapIter = optionMap.find(argv[i]);
        if (mapIter != optionMap.end()) { // option argument.
            mapIter->second = argv[++i];
        } else { // switch argument.
            switchSet.insert(argv[i]);
        }
    }

    Log(LogSwitch::Szx::Cli) << "execute commands." << endl;

    if (switchSet.find(AuthorNameSwitch()) != switchSet.end()) {
        cout << AuthorName() << endl;
    }

    Environment env;
    env.load(optionMap);
    if (env.instName.empty() || env.slnPath.empty()) { return -1; }

    Configuration cfg;

    Log(LogSwitch::Szx::Input) << "load instance " << env.instName << " (seed=" << env.randSeed << ")." << endl;
    Problem::Input input;
    if (!input.load(env.batchPath(), env.defectsPath())) { return -1; }

    Solver solver(input, env, cfg);
    solver.solve();
    solver.output.save(env.solutionPath());
    #if SZX_DEBUG
    solver.output.save(env.solutionPathWithTime());
    solver.record();
    #endif

    return 0;
}
#pragma endregion cli


#pragma region environment
void Environment::load(const Map<String, char*> &optionMap) {
    char *str;

    str = optionMap.at(Cli::InstancePathOption());
    if (str != nullptr) { instName = str; }

    str = optionMap.at(Cli::SolutionPathOption());
    if (str != nullptr) { slnPath = str; }

    str = optionMap.at(Cli::RandSeedOption());
    if (str != nullptr) { randSeed = atoi(str); }

    str = optionMap.at(Cli::TimeoutOption());
    if (str != nullptr) { msTimeout = static_cast<Duration>(atof(str) * Timer::MillisecondsPerSecond); }

    str = optionMap.at(Cli::MaxIterOption());
    if (str != nullptr) { maxIter = atoi(str); }

    str = optionMap.at(Cli::JobNumOption());
    if (str != nullptr) { jobNum = atoi(str); }

    str = optionMap.at(Cli::RunIdOption());
    if (str != nullptr) { rid = str; }

    str = optionMap.at(Cli::ConfigPathOption());
    if (str != nullptr) { cfgPath = str; }

    str = optionMap.at(Cli::LogPathOption());
    if (str != nullptr) { logPath = str; }

    calibrate();
}

void Environment::calibrate() {
    // adjust thread number.
    int threadNum = thread::hardware_concurrency();
    if ((jobNum <= 0) || (jobNum > threadNum)) { jobNum = threadNum; }

    // adjust timeout.
    msTimeout -= Environment::SaveSolutionTimeInMillisecond;
}
#pragma endregion environment


#pragma region solver
void Solver::solve() {
	this->init();
	this->run();
	this->toOutput();
}

void Solver::init() {
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

void Solver::run() {
	MySolution fixed_plate_sol; fixed_plate_sol.reserve(GV::item_num);
	List<MySolution> target_sols;
	List<List<MyStack>> eval_batchs;
	List<MySolution> eval_sols;
	List<LengthPair> eval_length;

	// 循环每次固定一块 Plate
	TID cur_plate = 0;
	while (fixed_plate_sol.size() < GV::item_num) {
		if (timer.isTimeOut()) { break; }

		PlateSearch plate_search(this->cfg, this->rand, this->timer, cur_plate);
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

Length Solver::evaluateOnePlate(TID cur_plate, List<MyStack>& batch, MySolution &comp_sol) {
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

void Solver::record() const {
    #if SZX_DEBUG

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Length obj = output.totalWidth * input.param.plateHeight - total_item_area;
    Length checkerObj = -1;
    bool feasible = 0;
    if (output.nodes.size()) {
        feasible = check(checkerObj);
    }

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

bool Solver::check(Length & checkerObj) const {
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

void Solver::toOutput() {
    if (best_solution.size() == 0)return;
    using SpecialType = Problem::Output::Node::SpecialType;
    const TLength PW = input.param.plateWidth, PH = input.param.plateHeight;
    // define current plate/cut1/cut2 identity in solution node.
    TID cur_plate = -1;
    // define current node/parentL0.. identity in output solution tree node.
    TID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    // define current c1cpl/c1cpr... when constructing solution tree.
    TCoord c1cpl = 0, c1cpr = 0, c2cpb = 0, c2cpu = 0, c3cp = 0;
    // construct solution tree by visit each sol TreeNode.
    for (int n = 0; n < best_solution.size(); ++n) {
        if (best_solution[n].getFlagBit(NEW_PLATE)) { // a new plate.
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
            cur_plate += 1;
            c1cpl = 0;
            c1cpr = get_next_1cut(n);
            c2cpb = 0;
            c2cpu = get_next_2cut(n);
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
        } else if (best_solution[n].getFlagBit(NEW_L1)) { // a new cut1.
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
            c1cpl = c1cpr;
            c1cpr = get_next_1cut(n);
            c2cpb = 0;
            c2cpu = get_next_2cut(n);
            c3cp = c1cpl;
            // creat new nodes.
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L1.
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (best_solution[n].getFlagBit(NEW_L2)) { // a new cut2.
            // add waste for last L2.
            if (c3cp < c1cpr) { // add waste between c3cp and c1cpr.
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            // update cut position and cut identity information.
            c2cpb = c2cpu;
            c2cpu = get_next_2cut(n);
            c3cp = c1cpl;
            // creat new nodes.
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // creat a new L2.
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        const TLength item_w = best_solution[n].getFlagBit(ROTATE) ?
            GV::items[best_solution[n].item].h : GV::items[best_solution[n].item].w;
        const TLength item_h = best_solution[n].getFlagBit(ROTATE) ?
            GV::items[best_solution[n].item].w : GV::items[best_solution[n].item].h;
        // if item placed in defect right bound, creat waste between c3cp and item left bound.
        if (best_solution[n].getFlagBit(FlagBit::DEFECT_R)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, c3cp, c2cpb, best_solution[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // if has bin4, place this node into last L3, else creat a new L3.
        if (best_solution[n].getFlagBit(FlagBit::BIN4)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, best_solution[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, best_solution[n].item, 4, parent_L3));
            continue;
        } else {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, best_solution[n].c3cp - item_w, c2cpb, item_w, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        // if item placed in defect up bound, creat waste between item bottom and c2cpb.
        if (best_solution[n].getFlagBit(FlagBit::DEFECT_U)) {
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, best_solution[n].c3cp - item_w, c2cpb, item_w, c2cpu - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, best_solution[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, best_solution[n].item, 4, parent_L3));
        } else {
            // creat a item in L3.
            output.nodes.push_back(Problem::Output::Node(
                cur_plate, nodeId++, best_solution[n].c3cp - item_w, c2cpb, item_w, item_h, best_solution[n].item, 4, parent_L3));
            // add waste between c4cp and c2cpu.
            if (c2cpu > best_solution[n].c4cp && n + 1 < best_solution.size() && !best_solution[n + 1].getFlagBit(FlagBit::BIN4)) {
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, best_solution[n].c3cp - item_w, best_solution[n].c4cp, item_w, c2cpu - best_solution[n].c4cp, SpecialType::Waste, 4, parent_L3));
            }
        }
        c3cp = best_solution[n].c3cp;
    }
    const SolutionNode last_item = best_solution.back();
    const TLength last_item_w = last_item.getFlagBit(FlagBit::ROTATE) ?
        GV::items[last_item.item].h : GV::items[last_item.item].w;
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
    for (int i = 0; i < best_solution.size(); ++i) {
        if (output.totalWidth < cur_plate*input.param.plateWidth + best_solution[i].c1cpr) {
            output.totalWidth = cur_plate*input.param.plateWidth + best_solution[i].c1cpr;
        }
    }
}

// get c1cpr for this index's item.
const TCoord Solver::get_next_1cut(int index) const {
    TCoord res = 0;
    while (index < best_solution.size()) {
        if (best_solution[index].getFlagBit(NEW_L1))
            break;
        if (res < best_solution[index].c1cpr)
            res = best_solution[index].c1cpr;
    }
    return res;
}

// get c2cpu for this index's item.
const TCoord Solver::get_next_2cut(int index) const {
    TCoord res = 0;
    while (index < best_solution.size()) {
        if (best_solution[index].getFlagBit(NEW_L2))
            break;
        if (res < best_solution[index].c2cpu)
            res = best_solution[index].c2cpu;
    }
    return res;
}
#pragma endregion solver

}
