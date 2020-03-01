#include "Solver/algorithm/header/Solver.h"

#include <iostream>
#include <fstream>
#include <windows.h>

#include "Solver/utility/LogSwitch.h"
#include "Solver/data/header/Global.h"
#include "Solver/algorithm/header/TopSearch.h"
#include "Solver/unit_test/test.h"


using namespace std;

namespace szx {

#pragma region Cli
int Cli::run(int argc, char *argv[]) {
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

    Log(LogSwitch::Szx::Input) << "load instance " << env.instName << " (seed=" << env.randSeed << ")." << endl;
    
    Problem::Input input;
    if (!input.load(env.batchPath(), env.defectsPath())) { return -1; }

    init_global_variables(input, env);
    
    Log(Log::Debug) << "Configure: " << gv::cfg.toBriefStr() << "\n"  << endl;
    
    #if TEST_MODE == true
    UnitTest test;
    test.run();
    #else
    Solver solver(input, env);
    solver.run();
    #endif

    return 0;
}
#pragma endregion Cli

#pragma region Solver

Problem::Output createOutput(const Solution &sol);

void Solver::run() {
    // 调用topSearch进行求解
    TopSearch solver;
    solver.run();
    if (solver.best_obj() != Problem::Output::MaxWidth) {
        Solution best_sol;
        solver.get_best_sol(best_sol);
        cout << gv::info.to_str() << endl;
        output = createOutput(best_sol);
        // 输出解
        output.save(env.solutionPath());
        #if SZX_DEBUG
        output.save(env.solutionPathWithTime());
        gv::info.save(env.statisticPath());
        record();
        #endif
    } else {
        Log(Log::Error) << "[ERROR]Can't find a complete solution." << endl;
    }
}

void Solver::record() const {
    #if SZX_DEBUG

    ostringstream log;

    Area total_item_area = 0;
    for (int i = 0; i < gv::item_area.size(); ++i) {
        total_item_area += gv::item_area[i];
    }

    Length obj = output.totalWidth * input.param.plateHeight - total_item_area;
    Length checkerObj = -1;
    bool feasible = 0;
    if (output.nodes.size()) {
        feasible = check(checkerObj);
    }

    // record basic information.
    log << env.friendlyLocalTime() << ";"
        << env.instName << ";"
        << env.randSeed << ";"
        << gv::timer.elapsedSeconds() << ";"
        << (feasible ? checkerObj : -1) << ";"
        << gv::cfg.toBriefStr();

    // record solution vector.
    // EXTEND[szx][2]: save solution in log.
    log << endl;

    // append all text atomically.
    static mutex logFileMutex;
    lock_guard<mutex> logFileGuard(logFileMutex);

    ofstream logFile(env.logPath, ios::app);
    logFile.seekp(0, ios::end);
    if (logFile.tellp() <= 0) {
        logFile << "Time,Instance,RandSeed,Duration,Waste,Configure" << endl;
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
#pragma endregion Solver

#pragma region ToOutput
/*
* 从sol[index]开始，获取下一个1-cut的位置。
*/
TCoord get_next_1cut(int index, const Solution &sol) {
    TCoord res = sol[index].c1cpr;
    ++index;
    while (index < sol.size()) {
        if (sol[index].getFlagBit(Placement::NEW_L1))
            break;
        if (res < sol[index].c1cpr)
            res = sol[index].c1cpr;
        ++index;
    }
    if (index == sol.size())
        return sol.back().c1cpr;
    return res;
}

/*
* 从sol[index]开始，获取下一个2-cut的位置。
*/
TCoord get_next_2cut(int index, const Solution &sol) {
    TCoord res = sol[index].c2cpu;
    ++index;
    while (index < sol.size()) {
        if (sol[index].getFlagBit(Placement::NEW_L2))
            break;
        if (res < sol[index].c2cpu)
            res = sol[index].c2cpu;
        ++index;
    }
    if (index == sol.size())
        return sol.back().c2cpu;
    return res;
}

/*
* 将Solution格式的解转换成Problem::Output格式的解。
*/
Problem::Output createOutput(const Solution &sol) {
    /*      _________________________________
           |_________________|<-waste_2    | |
     4-cut_|______|<-waste_4 |             | |
           |      |          |             | |
     2-cut_|______|__________|_____________| |<-waste_1
           |               | |             | |
           |               | |<-waste_3    | |
           |_______________|_|_____________|_|
                           ^               ^
                          3-cut           1-cut
    */
    Problem::Output output;
    if (sol.size() == 0)return output;
    using SpecialType = Problem::Output::Node::SpecialType;
    const Length PW = gv::param.plateWidth, PH = gv::param.plateHeight;
    ID cur_plate = -1;
    // parent_L0/1/2/3代表当前结点所在的1/2/3层桶的节点id
    ID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    Coord c1cpl = 0, c1cpr = 0, c2cpb = 0, c2cpu = 0, c3cp = 0;
    // 模拟物品放置切割过程
    for (int n = 0; n < sol.size(); ++n) {
        if (sol[n].getFlagBit(Placement::NEW_PLATE)) { // 使用一块新的原料
            if (c3cp < c1cpr) { // + waste_3
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (cur_plate != -1 && c2cpu < gv::param.plateHeight) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            if (cur_plate != -1 && c1cpr < gv::param.plateWidth) { // + waste_1
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Waste, 1, parent_L0));
            }
            // 更新当前各个cut的位置信息
            cur_plate += 1;
            c1cpl = 0;
            c1cpr = get_next_1cut(n, sol);
            c2cpb = 0;
            c2cpu = get_next_2cut(n, sol);
            c3cp = 0;
            // 添加节点
            parent_L0 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 原料节点
                cur_plate, nodeId++, 0, 0, PW, PH, SpecialType::Branch, 0, -1));
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 一层桶节点
                cur_plate, nodeId++, 0, 0, c1cpr, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 二层桶节点
                cur_plate, nodeId++, 0, 0, c1cpr, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (sol[n].getFlagBit(Placement::NEW_L1)) { // 使用新的一层桶
            if (c3cp < c1cpr) { // + waste_3
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (c2cpu < gv::param.plateHeight) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            c1cpl = c1cpr;
            c1cpr = get_next_1cut(n, sol);
            c2cpb = 0;
            c2cpu = get_next_2cut(n, sol);
            c3cp = c1cpl;
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 一层桶
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 二层桶
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (sol[n].getFlagBit(Placement::NEW_L2)) {  // 使用新的二层桶
            if (c3cp < c1cpr) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            c2cpb = c2cpu;
            c2cpu = get_next_2cut(n, sol);
            c3cp = c1cpl;
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 二层桶
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        // 获取物品宽高
        const Length item_w = sol[n].getFlagBit(Placement::ROTATE) ?
            gv::items[sol[n].item].h : gv::items[sol[n].item].w;
        const Length item_h = sol[n].getFlagBit(Placement::ROTATE) ?
            gv::items[sol[n].item].w : gv::items[sol[n].item].h;
        /*  ________________
           |   | |<-waste_5 |
           |   | |          |
           |   |d|  item    |
           |___|_|__________|
        */// 添加物品平移到瑕疵右侧所产生waste
        if (sol[n].getFlagBit(Placement::DEFECT_R)) {
            output.nodes.push_back(Problem::Output::Node( // + waste
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // 如果该物品属于四层桶，直接把它放到上一个3层桶中，否则新建一个3层桶
        if (sol[n].getFlagBit(Placement::BIN4)) {
            output.nodes.push_back(Problem::Output::Node( // + 物品
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
            continue;
        } else {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 三层桶
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        /*  ___________________
           |        |          |
           |  item  |          |
           |________|          |
           |   d    |<-waste_6 |
           |________|__________|
        */// 添加物品平移到瑕疵上侧所产生的waste
        if (sol[n].getFlagBit(Placement::DEFECT_U)) {
            output.nodes.push_back(Problem::Output::Node( // + waste
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
            output.nodes.push_back(Problem::Output::Node( // + 物品
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
        } else {
            output.nodes.push_back(Problem::Output::Node( // + 物品
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, item_h, sol[n].item, 4, parent_L3));
            if (c2cpu > sol[n].c4cp && n + 1 < sol.size() && !sol[n + 1].getFlagBit(Placement::BIN4)) {
                output.nodes.push_back(Problem::Output::Node( // + waste_4
                    cur_plate, nodeId++, sol[n].c3cp - item_w, sol[n].c4cp, item_w, c2cpu - sol[n].c4cp, SpecialType::Waste, 4, parent_L3));
            }
        }
        c3cp = sol[n].c3cp;
    }
    const Placement last_item = sol.back();
    const Length last_item_w = last_item.getFlagBit(Placement::ROTATE) ?
        gv::items[last_item.item].h : gv::items[last_item.item].w;
    if (last_item.c4cp < c2cpu && !last_item.getFlagBit(Placement::DEFECT_U)) {
        output.nodes.push_back(Problem::Output::Node( // + waste_4
            cur_plate, nodeId++, last_item.c3cp - last_item_w,
            last_item.c4cp, last_item_w, c2cpu - last_item.c4cp, SpecialType::Waste, 4, parent_L3));
    }
    if (c3cp < c1cpr) {
        output.nodes.push_back(Problem::Output::Node( // + 最后一个waste_3
            cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
    }
    if (c2cpu < gv::param.plateHeight) { // + 最后一个waste_2
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
    }
    if (c1cpr < gv::param.plateWidth) { // + 余料
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Residual, 1, parent_L0));
    }
    // 计算已使用原料的宽度
    output.totalWidth = cur_plate * gv::param.plateWidth + sol.back().c1cpr;
    return output;
}

/*
* 功能：更新中间解文件。
*/
void update_middle_solution(TID pid, const Solution &sol) {
    if (sol.empty())return;
    static const string middle_path = "middle_sol.csv";
    static constexpr int sleep_milliseconds = 1000;
    int count = 0;
    Placement plate_item;
    plate_item.c1cpl = 0;
    plate_item.c1cpr = gv::param.plateWidth;
    plate_item.c2cpb = 0;
    plate_item.c2cpu = gv::param.plateHeight;
    plate_item.c3cp = gv::param.plateWidth;
    plate_item.c4cp = gv::param.plateHeight;
    plate_item.item = static_cast<TID>(gv::items.size());
    gv::items.push_back(Rect(gv::param.plateWidth, gv::param.plateHeight));
    ++count;
    plate_item.flag = Placement::NEW_PLATE | Placement::NEW_L1 | Placement::NEW_L2;
    Solution new_sol;
    while (pid--)new_sol.push_back(plate_item);
    if (!sol[0].getFlagBit(Placement::NEW_PLATE)) {
        plate_item.c1cpr = sol[0].c1cpl;
        plate_item.c3cp = sol[0].c1cpl;
        plate_item.item = static_cast<TID>(gv::items.size());
        gv::items.push_back(Rect(sol[0].c1cpl, gv::param.plateHeight));
        ++count;
        new_sol.push_back(plate_item);
    }
    new_sol += sol;
    Problem::Output out = createOutput(new_sol);
    while (count--)gv::items.pop_back();
    for (Problem::Output::Node& node : out.nodes) {
        if (node.type >= int(gv::items.size())) {
            node.type = -1;
        }
    }
    out.save("middle_sol.csv");
    Sleep(sleep_milliseconds);
}
#pragma endregion

}
