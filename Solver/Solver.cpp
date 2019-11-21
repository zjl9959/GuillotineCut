#include "Solver.h"

#include <iostream>
#include <fstream>

#include "data/Configuration.h"
#include "utility/LogSwitch.h"
#include "utility/ThreadPool.h"
#include "algorithm/Factory.h"
#include "algorithm/TopSearch.h"
#include "unit_test/test.h"

using namespace std;

namespace szx {

#pragma region Cli
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
    solver.run();
    

    /*
    // 测试cutSearch
    Auxiliary aux = createAuxiliary(input);
    Solution sol = unit_test::test_CutSearch(aux);
    Problem::Output out = createOutput(sol, aux);
    out.save(env.solutionPathWithTime());
    */

    /*
    // 测试plateSearch
    Auxiliary aux = createAuxiliary(input);
    Timer timer(chrono::milliseconds(env.msTimeout));
    Random rand(env.randSeed);
    Solution sol = unit_test::test_PlateSearch(timer, rand, cfg, aux);
    Problem::Output out = createOutput(sol, aux);
    out.save(env.solutionPathWithTime());
    */

    return 0;
}
#pragma endregion Cli


#pragma region Environment
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
#pragma endregion Environment


#pragma region Solver
void Solver::run() {
    //unit_test::test_copy_speed_of_placement(100000, 10000);
    aux = createAuxiliary(input);
    // 调用topSearch进行求解
    TopSearch solver(rand, timer, cfg, aux);
    Solution best_sol;
    solver.beam_search();
    solver.get_best_sol(best_sol);
    output = createOutput(best_sol, aux);
    // 输出解
    output.save(env.solutionPath());
    #if SZX_DEBUG
    save_solution(best_sol, "tmp/solution.csv");
    output.save(env.solutionPathWithTime());
    record();
    #endif
}

void Solver::record() const {
    #if SZX_DEBUG

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Area total_item_area = 0;
    for (int i = 0; i < aux.item_area.size(); ++i) {
        total_item_area += aux.item_area[i];
    }

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
        logFile << "Time,ID,Instance,Feasible,ObjMatch,Width,Duration,PhysMem,VirtMem,RandSeed,Config,UtilRatio,CheckerObj" << endl;
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

}
