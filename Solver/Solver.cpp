#include "Solver.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

#include <cmath>

#include "MpSolver.h"
#include "Visualizer.h"


using namespace std;


namespace szx {

#pragma region Solver::Cli
int Solver::Cli::run(int argc, char * argv[]) {
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
    if (switchSet.find(HelpSwitch()) != switchSet.end()) {
        cout << HelpInfo << endl;
    }

    if (switchSet.find(AuthorNameSwitch()) != switchSet.end()) {
        cout << AuthorName << endl;
    }

    Solver::Environment env;
    env.load(optionMap);
    if (env.instName.empty() || env.slnPath.empty()) { return -1; }

    Solver::Configuration cfg;
    cfg.load(env.cfgPath);

    Log(LogSwitch::Szx::Cli) << "load instance " << env.instName << " (seed=" << env.randSeed << ")." << endl;
    Problem::Input input;
    if (!input.load(env.batchPath(), env.defectsPath())) { return -1; }

    Solver solver(input, env, cfg);
    solver.solve();
    solver.output.save(env.solutionPath());
    // OPTIMIZE[szx][9]: confirm arg format for solution file.
    //solver.output.save(env.solutionPathWithSuffix());
    solver.output.save(env.solutionPathWithTime());
    solver.record();

    return 0;
}
#pragma endregion Solver::Cli

#pragma region Solver::Environment
void Solver::Environment::load(const Map<String, char*> &optionMap) {
    char *str;

    str = optionMap.at(Cli::EnvironmentPathOption());
    if (str != nullptr) { loadWithoutCalibrate(str); }

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

void Solver::Environment::load(const String &filePath) {
    loadWithoutCalibrate(filePath);
    calibrate();
}

void Solver::Environment::loadWithoutCalibrate(const String &filePath) {
    // EXTEND[szx][8]: load environment from file.
    // EXTEND[szx][8]: check file existence first.
}

void Solver::Environment::save(const String &filePath) const {
    // EXTEND[szx][8]: save environment to file.
}
void Solver::Environment::calibrate() {
    // adjust thread number.
    int threadNum = thread::hardware_concurrency();
    if ((jobNum <= 0) || (jobNum > threadNum)) { jobNum = threadNum; }

    // adjust timeout.
    msTimeout -= Environment::SaveSolutionTimeInMillisecond;
}
#pragma endregion Solver::Environment

#pragma region Solver::Configuration
void Solver::Configuration::load(const String &filePath) {
    // EXTEND[szx][5]: load configuration from file.
    // EXTEND[szx][8]: check file existence first.
}

void Solver::Configuration::save(const String &filePath) const {
    // EXTEND[szx][5]: save configuration to file.
}
#pragma endregion Solver::Configuration

#pragma region Solver
void Solver::solve() {
    init();

    List<Solution> solutions(env.jobNum, Solution(this));

    Log(LogSwitch::Szx::Framework) << "launch " << env.jobNum << " workers." << endl;
    List<thread> threadList;
    threadList.reserve(env.jobNum);
    for (int i = 0; i < env.jobNum; ++i) {
        // TODO[szx][0]: as *this is captured by ref, the solver should support concurrency itself, i.e., data members should be read-only or independent for each worker.
        // OPTIMIZE[szx][3]: add a list to specify a series of algorithm to be used by each threads in sequence.
        threadList.emplace_back([&, i]() { optimize(solutions[i], i); });
    }
    for (int i = 0; i < env.jobNum; ++i) { threadList.at(i).join(); }

    Log(LogSwitch::Szx::Framework) << "collect best result among all workers." << endl;
    output = *min_element(solutions.begin(), solutions.end(),
        [](Solution &l, Solution &r) { return (l.totalWidth < r.totalWidth); });
}

void Solver::record() const {
    #if SZX_DEBUG
    int generation = 0, iteration = 0;

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Length obj = output.totalWidth * input.param.plateHeight - totalItemArea();
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
        << generation << "," << iteration << ","
        << totalItemArea() / (output.totalWidth * input.param.plateHeight / 100.0) << "%," // util ratio.
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

bool Solver::check(Length &checkerObj) const {
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

Length Solver::totalItemArea() const {
    Length area = 0;
    for (auto i = aux.items.begin(); i != aux.items.end(); ++i) { area += (i->w * i->h); }
    return area;
}

void Solver::init() {
    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());

    // assign internal id to items and stacks, then push items into stacks.
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        ID itemId = idMap.item.toConsecutiveId(i->id);
        aux.items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
        if (itemId != i->id) {
            Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
        }

        ID stackId = idMap.stack.toConsecutiveId(i->stack);
        if (aux.stacks.size() <= stackId) { aux.stacks.push_back(List<ID>()); } // create a new stack.
        List<ID> &stack(aux.stacks[stackId]);
        // OPTIMIZE[szx][6]: what if the sequence number could be negative or very large?
        if (stack.size() <= i->seq) { stack.resize(i->seq + 1, Problem::InvalidItemId); }
        stack[i->seq] = itemId;
    }
    // clear invalid items in stacks.
    for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), Problem::InvalidItemId), s->end());
    }

    aux.defects.reserve(input.defects.size());
    // EXTEND[szx][9]: make sure that the plate IDs are already zero-base consecutive numbers.
    aux.plates.resize(Problem::MaxPlateNum);
    for (ID p = 0; p < Problem::MaxPlateNum; ++p) { idMap.plate.toConsecutiveId(p); }

    // map defects to plates.
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        ID defectId = idMap.defect.toConsecutiveId(d->id);
        aux.defects.push_back(RectArea(d->x, d->y, d->width, d->height));

        ID plateId = idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plates.size() <= plateId) { aux.plates.resize(plateId + 1); } // create new plates.
        aux.plates[plateId].push_back(defectId);
    }
}

void Solver::optimize(Solution &sln, ID workerId) {
    Log(LogSwitch::Szx::Framework) << "worker " << workerId << " starts." << endl;

    cfg.alg = (aux.items.size() > cfg.itemNumThresholdForCompleteModel) ? Configuration::Algorithm::IteratedMp : Configuration::Algorithm::CompleteMp;
    switch (cfg.alg) {
    case Configuration::Algorithm::CompleteMp:
        cfg.cm.maxCoveredArea = true;
        optimizeCompleteModel(sln, cfg.cm); break;
    case Configuration::Algorithm::IteratedMp:
        //cfg.im.minWastedArea = false;
        optimizeIteratedModel(sln, cfg.im); break;
    }

    Log(LogSwitch::Szx::Framework) << "worker " << workerId << " ends." << endl;
}

//  y ^
//    |           w1
//    |  +-----+-----+
//    |  |     |     |
//    |  |     +--+--+
//    |  |     |  |  |h2
//    |  +-----+--+--+
//    |            w3
// ---+------------------>
//  O |                  x
void Solver::optimizeCompleteModel(Solution &sln, Configuration::CompleteModel cfg) {
    using Dvar = MpSolver::DecisionVar;
    using Expr = MpSolver::LinearExpr;
    using VarType = MpSolver::VariableType;

    enum Layer { L0, L1, L2, L3 };
    constexpr ID maxBinNum[] = { 2, 8, 8, 6 }; // EXTEND[szx][5]: parameterize the constant!
    constexpr ID binNum = maxBinNum[L0] * maxBinNum[L1] * maxBinNum[L2] * maxBinNum[L3];
    constexpr bool flawless = false; // there are defects on the plate.
    constexpr bool ordered = true; // the items should be produced in given order.

    ID stackNum = static_cast<ID>(aux.stacks.size());
    ID itemNum = static_cast<ID>(aux.items.size());

    MpSolver::Configuration mpcfg(MpSolver::Configuration::DefaultSolver, timer.restSeconds(), true, false);
    MpSolver mp(mpcfg);

    Arr<Dvar> d(itemNum); // direction (if i should rotate 90 degree).

    Arr<Dvar> p(maxBinNum[L0]); // item placed in plate.
    Arr2D<Dvar> p1(maxBinNum[L0], maxBinNum[L1]); // there are some items placed in L1 virtual bin.
    Arr2D<Arr<Dvar>> p2(maxBinNum[L0], maxBinNum[L1], Arr<Dvar>(maxBinNum[L2])); // there are some items placed in L2 virtual bin.
    Arr2D<Arr2D<Dvar>> p3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // there are some items placed in L3 virtual bin.
    Arr<Arr2D<Arr2D<Dvar>>> pi3(itemNum, Arr2D<Arr2D<Dvar>>(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3]))); // item placed in L3 virtual bin.

    Arr2D<Dvar> w1(maxBinNum[L0], maxBinNum[L1]); // width of L1 virtual bin.
    Arr2D<Arr<Dvar>> h2(maxBinNum[L0], maxBinNum[L1], Arr<Dvar>(maxBinNum[L2])); // height of L2 virtual bin.
    Arr2D<Arr2D<Dvar>> w3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // width of L3 virtual bin.
    Arr2D<Arr2D<Dvar>> h4u(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // height of upper waste in L3 virtual bin.
    Arr2D<Arr2D<Dvar>> h4l(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // height of lower waste in L3 virtual bin.

    Arr2D<Arr<Dvar>> t2(maxBinNum[L0], maxBinNum[L1], Arr<Dvar>(maxBinNum[L2])); // L2 virtual bin is non-trivial.
    Arr2D<Arr2D<Dvar>> t3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is non-trivial.
    Arr2D<Arr2D<Dvar>> t4u(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // upper waste in L3 virtual bin is non-trivial.
    Arr2D<Arr2D<Dvar>> t4l(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // lower waste in L3 virtual bin is non-trivial.

    Arr<Dvar> e(maxBinNum[L0]); // there is residual on plate.
    Dvar r; // width of the used area in the last plate.

    Arr2D<Arr2D<Arr<Dvar>>> c(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is contains flaw.
    Arr2D<Arr2D<Arr<Dvar>>> cr(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is not on the right side of flaw.
    Arr2D<Arr2D<Arr<Dvar>>> cl(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is not on the left side of flaw.
    Arr2D<Arr2D<Arr<Dvar>>> cu(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is not on the up side of flaw.
    Arr2D<Arr2D<Arr<Dvar>>> cd(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is not on the down side of flaw.

    //Arr<Dvar> o(itemNum); // the sequence number of item to be produced.
    //o[i] = mp.addVar(VarType::Real, 0, binNum);

    Expr coveredArea; // the sum of placed items' area.
    Expr coveredWidth;
    Expr placedItem;
    Arr<Expr> pi(itemNum);

    Log(LogSwitch::Szx::Model) << "add decisions variables." << endl;
    for (ID i = 0; i < itemNum; ++i) {
        d[i] = mp.addVar(VarType::Bool, 0, 1);
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        pi3[i][g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                    }
                }
            }
        }
    }
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        p[g] = mp.addVar(VarType::Bool, 0, 1);
        e[g] = mp.addVar(VarType::Bool, 0, 1);
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            p1[g][l] = mp.addVar(VarType::Bool, 0, 1);
            w1[g][l] = mp.addVar(VarType::Real, 0, input.param.plateWidth); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                p2[g][l][m] = mp.addVar(VarType::Bool, 0, 1);
                h2[g][l][m] = mp.addVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                t2[g][l][m] = mp.addVar(VarType::Bool, 0, 1);
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    p3[g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                    w3[g][l][m][n] = mp.addVar(VarType::Real, 0, input.param.plateWidth); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    h4u[g][l][m][n] = mp.addVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    h4l[g][l][m][n] = mp.addVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    t3[g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                    t4u[g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                    t4l[g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                    ID defectNum = static_cast<ID>(aux.plates[g].size());
                    c[g][l][m][n].init(defectNum);
                    cr[g][l][m][n].init(defectNum);
                    cl[g][l][m][n].init(defectNum);
                    cu[g][l][m][n].init(defectNum);
                    cd[g][l][m][n].init(defectNum);
                    for (ID f = 0; f < defectNum; ++f) {
                        c[g][l][m][n][f] = mp.addVar(VarType::Bool, 0, 1);
                        cr[g][l][m][n][f] = mp.addVar(VarType::Bool, 0, 1);
                        cl[g][l][m][n][f] = mp.addVar(VarType::Bool, 0, 1);
                        cu[g][l][m][n][f] = mp.addVar(VarType::Bool, 0, 1);
                        cd[g][l][m][n][f] = mp.addVar(VarType::Bool, 0, 1);
                    }
                }
            }
        }
    }
    r = mp.addVar(VarType::Real, 0, input.param.plateWidth);

    Log(LogSwitch::Szx::Model) << "add constraints." << endl;
    Log(LogSwitch::Szx::Model) << "add placement constraints." << endl;
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        Expr plateItems;
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            Expr l1BinItems;
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                Expr l2BinItems;
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    Expr l3BinItems;
                    for (ID i = 0; i < itemNum; ++i) { l3BinItems += pi3[i][g][l][m][n]; }
                    l2BinItems += l3BinItems;
                    // exclusive placement.
                    if (aux.plates[g].empty()) { mp.addConstraint(l3BinItems <= 1); } // in case there is no defect on this plate and the defect free constraint is not added.
                    // L3 item placement.
                    mp.addConstraint(l3BinItems == p3[g][l][m][n]); // OPTIMIZE[szx][5]: can be <= if the obj is not maximizing the area of placed items.
                }
                l1BinItems += l2BinItems;
                // L2 item placement.
                mp.addConstraint(p2[g][l][m] <= l2BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
                mp.addConstraint(l2BinItems <= itemNum * p2[g][l][m]);
            }
            plateItems += l1BinItems;
            // L1 item placement.
            mp.addConstraint(p1[g][l] <= l1BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
            mp.addConstraint(l1BinItems <= itemNum * p1[g][l]);
        }
        // plate used.
        mp.addConstraint(p[g] <= plateItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
        mp.addConstraint(plateItems <= itemNum * p[g]);

        coveredWidth += input.param.plateWidth * p[g];
    }
    coveredWidth -= (input.param.plateWidth - r);

    Log(LogSwitch::Szx::Model) << "add fitting constraints." << endl;
    auto width = [&](ID itemId) {
        return (aux.items[itemId].w * (1 - d[itemId]) + aux.items[itemId].h * d[itemId]);
    };
    auto height = [&](ID itemId) {
        return (aux.items[itemId].h * (1 - d[itemId]) + aux.items[itemId].w * d[itemId]);
    };
    for (ID i = 0; i < itemNum; ++i) {
        Length itemLen = aux.items[i].w;
        Expr putInBin;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        putInBin += pi3[i][g][l][m][n];
                        // horizontal fitting.
                        mp.addConstraint(width(i) - itemLen * (1 - pi3[i][g][l][m][n]) <= w3[g][l][m][n]);
                        mp.addConstraint(w3[g][l][m][n] <= width(i) + input.param.plateWidth * (1 - pi3[i][g][l][m][n]));
                        // vertical fitting.
                        mp.addConstraint(height(i) - itemLen * (1 - pi3[i][g][l][m][n]) <= h2[g][l][m] - h4u[g][l][m][n] - h4l[g][l][m][n]);
                        mp.addConstraint(h2[g][l][m] - h4u[g][l][m][n] - h4l[g][l][m][n] <= height(i) + input.param.plateHeight * (1 - pi3[i][g][l][m][n]));
                    }
                }
            }
        }
        // single placement.
        mp.addConstraint(cfg.placeAllItems ? (putInBin == 1) : (putInBin <= 1));
        // covered area.
        coveredArea += (aux.items[i].w * aux.items[i].h * putInBin);
        placedItem += putInBin;
        pi[i] += putInBin;
    }

    Log(LogSwitch::Szx::Model) << "add composition constraints." << endl;
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        Expr l1BinWidthSum;
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            l1BinWidthSum += w1[g][l];
            Expr l2BinHeightSum;
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                l2BinHeightSum += h2[g][l][m];
                Expr l3BinWidthSum;
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    l3BinWidthSum += w3[g][l][m][n];
                }
                // L3 total width.
                mp.addConstraint(l3BinWidthSum == w1[g][l]);
            }
            // L2 total height.
            mp.addConstraint(l2BinHeightSum == input.param.plateHeight);
        }
        // L1 total width.
        mp.addConstraint(input.param.plateWidth * (1 - e[g]) <= l1BinWidthSum);
        mp.addConstraint(l1BinWidthSum <= input.param.plateWidth - input.param.minWasteWidth * e[g]);

        // residual position.
        if (g + 1 < maxBinNum[L0]) {
            mp.addConstraint(l1BinWidthSum - input.param.plateWidth * (1 - p[g] + p[g + 1]) <= r);
            mp.addConstraint(r <= l1BinWidthSum + input.param.plateWidth * (1 - p[g] + p[g + 1]));
        } else {
            mp.addConstraint(l1BinWidthSum - input.param.plateWidth * (1 - p[g]) <= r);
            mp.addConstraint(r <= l1BinWidthSum + input.param.plateWidth * (1 - p[g]));
        }
    }

    Log(LogSwitch::Szx::Model) << "add bounding constraints." << endl;
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            // L1 width bound.
            mp.addConstraint(input.param.minL1Width * p1[g][l] <= w1[g][l]);
            mp.addConstraint(w1[g][l] <= input.param.maxL1Width);
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                // L2 min height.
                mp.addConstraint(input.param.minL2Height * p2[g][l][m] <= h2[g][l][m]);
                mp.addConstraint(input.param.minWasteHeight * t2[g][l][m] - input.param.plateHeight * p2[g][l][m] <= h2[g][l][m]);
                mp.addConstraint(h2[g][l][m] <= input.param.plateHeight * t2[g][l][m] + input.param.plateHeight * p2[g][l][m]);
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    // L3 min width.
                    mp.addConstraint(input.param.minWasteWidth * t3[g][l][m][n] - input.param.plateWidth * p3[g][l][m][n] <= w3[g][l][m][n]);
                    mp.addConstraint(w3[g][l][m][n] <= input.param.plateWidth * t3[g][l][m][n] + input.param.plateWidth * p3[g][l][m][n]);
                    // L4 min height.
                    mp.addConstraint(input.param.minWasteHeight * t4u[g][l][m][n] <= h4u[g][l][m][n]);
                    mp.addConstraint(h4u[g][l][m][n] <= input.param.plateHeight * t4u[g][l][m][n]);
                    mp.addConstraint(input.param.minWasteHeight * t4l[g][l][m][n] <= h4l[g][l][m][n]);
                    mp.addConstraint(h4l[g][l][m][n] <= input.param.plateHeight * t4l[g][l][m][n]);
                    // max waste.
                    mp.addConstraint(t4u[g][l][m][n] + t4l[g][l][m][n] <= 1);
                }
            }
        }
    }

    if (ordered) {
        Log(LogSwitch::Szx::Model) << "add ordering constraints." << endl;
        for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
            for (auto i = s->begin(), j = s->begin() + 1; j != s->end(); ++i, ++j) {
                Expr putInBin;
                for (ID g = 0; g < maxBinNum[L0]; ++g) {
                    for (ID l = 0; l < maxBinNum[L1]; ++l) {
                        for (ID m = 0; m < maxBinNum[L2]; ++m) {
                            for (ID n = 0; n < maxBinNum[L3]; ++n) {
                                mp.addConstraint(1 - pi3[*i][g][l][m][n] >= putInBin); // OPTIMIZE[szx][9]: skip the very first one where g=l=m=n=0.
                                putInBin += pi3[*j][g][l][m][n];
                            }
                        }
                    }
                }
            }
        }

        // an item can not be placed if its preceding item is not placed.
        if (!cfg.placeAllItems || cfg.addPlacementOrderCut) {
            for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
                Expr prevPutInBin;
                for (auto i = s->begin(); i != s->end(); ++i) {
                    Expr nextPutInBin;
                    for (ID g = 0; g < maxBinNum[L0]; ++g) {
                        for (ID l = 0; l < maxBinNum[L1]; ++l) {
                            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                                    nextPutInBin += pi3[*i][g][l][m][n];
                                }
                            }
                        }
                    }
                    if (i > s->begin()) { mp.addConstraint(prevPutInBin >= nextPutInBin); }
                    prevPutInBin = nextPutInBin;
                }
            }
        }
    }

    if (!flawless) {
        Log(LogSwitch::Szx::Model) << "add defect constraints." << endl;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        ID f = 0;
                        for (auto gf = aux.plates[g].begin(); gf != aux.plates[g].end(); ++gf, ++f) {
                            // defect free.
                            mp.addConstraint(p3[g][l][m][n] <= 1 - c[g][l][m][n][f]);
                            // defect containing.
                            mp.addConstraint(3 + c[g][l][m][n][f] >= cr[g][l][m][n][f] + cl[g][l][m][n][f] + cu[g][l][m][n][f] + cd[g][l][m][n][f]);
                            // defect direction.
                            Expr fx;
                            for (ID ll = 0; ll < l; ++ll) { fx += w1[g][ll]; }
                            for (ID nn = 0; nn < n; ++nn) { fx += w3[g][l][m][nn]; }
                            Expr fy;
                            for (ID mm = 0; mm < m; ++mm) { fy += h2[g][l][mm]; }
                            mp.addConstraint(fx + input.param.plateWidth * cr[g][l][m][n][f] >= aux.defects[*gf].x + aux.defects[*gf].w);
                            mp.addConstraint(fx + w3[g][l][m][n] - input.param.plateWidth * cl[g][l][m][n][f] <= aux.defects[*gf].x);
                            mp.addConstraint(fy + h4l[g][l][m][n] + input.param.plateHeight * cu[g][l][m][n][f] >= aux.defects[*gf].y + aux.defects[*gf].h);
                            mp.addConstraint(fy - h4u[g][l][m][n] + h2[g][l][m] - input.param.plateHeight * cd[g][l][m][n][f] <= aux.defects[*gf].y);
                        }
                    }
                }
            }
        }
    }

    if (cfg.addBinSizeOrderCut) {
        Log(LogSwitch::Szx::Model) << "add bin size order cuts." << endl;
        // if there is no defect and order, put larger bins to the left or bottom.
        // else put all non-trivial bins to the left or bottom.
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                if (l + 1 < maxBinNum[L1]) {
                    mp.addConstraint(t3[g][l][0][0] >= t3[g][l + 1][0][0]);
                    if (flawless && !ordered) { mp.addConstraint(w1[g][l] >= w1[g][l + 1]); }
                }
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    if (m + 1 < maxBinNum[L2]) {
                        mp.addConstraint(t2[g][l][m] >= t2[g][l][m + 1]);
                        if (flawless && !ordered) { mp.addConstraint(h2[g][l][m] >= h2[g][l][m + 1]); }
                    }
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        if (n + 1 < maxBinNum[L3]) {
                            mp.addConstraint(t3[g][l][m][n] >= t3[g][l][m][n + 1]);
                            if (flawless && !ordered) { mp.addConstraint(w3[g][l][m][n] >= w3[g][l][m][n + 1]); }
                        }
                    }
                }
            }
        }
    }

    if (cfg.addEmptyBinMergingCut) {
        Log(LogSwitch::Szx::Model) << "add empty bin merging cuts." << endl;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                if (l + 1 < maxBinNum[L1]) {
                    // OPTIMIZE[szx][0]: make sure adding this will not cut the optima!
                    //mp.addConstraint(p1[g][l] + p1[g][l + 1] >= t3[g][l + 1][0][0]);
                }
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    if (m + 1 < maxBinNum[L2]) {
                        mp.addConstraint(p2[g][l][m] + p2[g][l][m + 1] >= t2[g][l][m + 1]);
                    }
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        if (n + 1 < maxBinNum[L3]) {
                            mp.addConstraint(p3[g][l][m][n] + p3[g][l][m][n + 1] >= t3[g][l][m][n + 1]);
                        }
                    }
                }
            }
        }
    }

    if (cfg.addGlassOrderCut) { // user cut for glass order.
        Log(LogSwitch::Szx::Model) << "add glass order cuts." << endl;
        for (ID g = 1; g < maxBinNum[L0]; ++g) { mp.addConstraint(p[g - 1] >= p[g]); }
    }

    if (cfg.addAreaBoundCut) { // user cut for covered area should be less than the plate or bin.
        // TODO[szx][1]: fix the bug here.
        Log(LogSwitch::Szx::Model) << "add area bound cuts." << endl;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            Expr area;
            for (ID i = 0; i < itemNum; ++i) {
                Expr putInBin;
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            putInBin += pi3[i][g][l][m][n];
                        }
                    }
                }
                area += (aux.items[i].w * aux.items[i].h * putInBin);
            }
            mp.addConstraint(area <= input.param.plateWidth * input.param.plateHeight);
        }
    }

    if (cfg.addL1BinWidthSumCut) {
        // OPTIMIZE[szx][6]: tighter bound didn't give better performance?
        //Expr l1BinWidthSum;
        //for (ID g = 0; g < maxBinNum[L0]; ++g) {
        //    for (ID l = 0; l < maxBinNum[L1]; ++l) { l1BinWidthSum += w1[g][l]; }
        //}

        Log(LogSwitch::Szx::Model) << "add min L1 width sum cut." << endl;
        if (cfg.placeAllItems) {
            //mp.addConstraint(input.param.plateHeight * l1BinWidthSum >= totalItemArea);
            mp.addConstraint(input.param.plateHeight * coveredWidth >= totalItemArea());
        } else {
            //mp.addConstraint(input.param.plateHeight * l1BinWidthSum >= coveredArea);
            mp.addConstraint(input.param.plateHeight * coveredWidth >= coveredArea);
        }
    }

    Log(LogSwitch::Szx::Model) << "add objectives." << endl;
    // maximize the area of placed items.
    if (cfg.maxCoveredArea && !cfg.placeAllItems) { // EXTEND[szx][5]: parameterize the constant!
        mp.addObjective(coveredArea, MpSolver::OptimaOrientation::Maximize, 0, 0, 0, timer.restSeconds() * 0.75);
    }

    // maximize the area utilization.
    if (cfg.maxCoveredRatio) {
        // TODO[szx][4]: what if the optimal ratio is below optRatio? then the optima of this transformed obj will be 0 and nothing will be placed!
        // TODO[szx][0]: parameterize the constant.
        // OPTIMIZE[szx][2]: set it to greater value or make more iterations? this may not get better result since the construction itself is empirical.
        //mp.addConstraint(placedItem >= 1);
        double optRatio = 0.8; // OPTIMIZE[szx][0]: parameterize the constant.
        mp.addObjective(coveredArea - optRatio * input.param.plateHeight * coveredWidth, MpSolver::OptimaOrientation::Maximize, 0, 0, 0, timer.restSeconds() * 0.75);
    }

    // minimize the wasted area.
    if (cfg.minWastedArea) {
        mp.addConstraint(placedItem >= 1);
        mp.addObjective(input.param.plateHeight * coveredWidth - coveredArea, MpSolver::OptimaOrientation::Minimize, 1, 0, 0, MpSolver::Configuration::Forever);
    }

    // minimize the width of used plates (the original objective).
    mp.addObjective(coveredWidth, MpSolver::OptimaOrientation::Minimize, 2, 0, 0, MpSolver::Configuration::Forever);

    // solve.
    if (mp.optimize()) {
        // record obj and solution.
        sln.totalWidth = 0;
        using Node = Problem::Output::Node;
        for (ID g = 0; ; ) {
            sln.bins.push_back(Bin(0, 0, input.param.plateWidth, input.param.plateHeight, Node::SpecialType::Branch));
            Bin &plate(sln.bins.back());
            double x1 = 0;
            double margin1 = 0; // cumulative length loss due to truncation from double to int.
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                double l1BinWidth = mp.getValue(w1[g][l]);
                margin1 += (l1BinWidth - Math::floor(l1BinWidth));
                l1BinWidth = Math::floor(l1BinWidth);
                if (Math::weakLess(l1BinWidth, 0)) { continue; }
                if (Math::weakLess(1, margin1)) {
                    l1BinWidth += margin1;
                    margin1 -= Math::floor(margin1);
                }
                plate.children.push_back(Bin(x1, 0, l1BinWidth, input.param.plateHeight, Node::SpecialType::Waste));
                Bin &l1Bin(plate.children.back());
                double y2 = 0;
                double margin2 = 0; // cumulative length loss due to truncation from double to int.
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    double l2BinHeight = mp.getValue(h2[g][l][m]);
                    margin2 += (l2BinHeight - Math::floor(l2BinHeight));
                    l2BinHeight = Math::floor(l2BinHeight);
                    if (Math::weakLess(l2BinHeight, 0)) { continue; }
                    if (Math::weakLess(1, margin2)) {
                        l2BinHeight += margin2;
                        margin2 -= Math::floor(margin2);
                    }
                    l1Bin.children.push_back(Bin(x1, y2, l1BinWidth, l2BinHeight, Node::SpecialType::Waste));
                    Bin &l2Bin(l1Bin.children.back());
                    double x3 = x1;
                    double margin3 = 0; // cumulative length loss due to truncation from double to int.
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        double l3BinWidth = mp.getValue(w3[g][l][m][n]);
                        margin3 += (l3BinWidth - Math::floor(l3BinWidth));
                        l3BinWidth = Math::floor(l3BinWidth);
                        if (Math::weakLess(l3BinWidth, 0)) { continue; }
                        if (Math::weakLess(1, margin3)) {
                            l3BinWidth += margin3;
                            margin3 -= Math::floor(margin3);
                        }
                        l2Bin.children.push_back(Bin(x3, y2, l3BinWidth, l2BinHeight, Node::SpecialType::Waste));
                        Bin &l3Bin(l2Bin.children.back());
                        for (ID i = 0; i < itemNum; ++i) {
                            if (!mp.isTrue(pi3[i][g][l][m][n])) { continue; }
                            l1Bin.type = l2Bin.type = l3Bin.type = Node::SpecialType::Branch;

                            Length w = (mp.isTrue(d[i])) ? aux.items[i].h : aux.items[i].w;
                            Length h = (mp.isTrue(d[i])) ? aux.items[i].w : aux.items[i].h;
                            double wasteHeight = l2BinHeight - h;
                            bool lowerWaste = mp.isTrue(t4l[g][l][m][n]);
                            bool upperWaste = mp.isTrue(t4u[g][l][m][n]);
                            if (lowerWaste) {
                                l3Bin.children.push_back(Bin(x3, y2, w, wasteHeight, Node::SpecialType::Waste));
                                l3Bin.children.push_back(Bin(x3, y2 + wasteHeight, w, h, i));
                            } else if (upperWaste) {
                                l3Bin.children.push_back(Bin(x3, y2, w, h, i));
                                l3Bin.children.push_back(Bin(x3, y2 + h, w, wasteHeight, Node::SpecialType::Waste));
                            } else { // item only.
                                l3Bin.rect = RectArea(Math::lfloor(x3), Math::lfloor(y2), w, h);
                                l3Bin.type = i;
                            }
                            // OPTIMIZE[szx][7]: make it consistent that items always produced via 4-cut?
                            //if (lowerWaste) { l3Bin.children.push_back(Bin(x3, y2, w, wasteHeight, Node::SpecialType::Waste)); }
                            //l3Bin.children.push_back(Bin(x3, y2 + wasteHeight, w, h, i));
                            //if (upperWaste) { l3Bin.children.push_back(Bin(x3, y2 + h, w, wasteHeight, Node::SpecialType::Waste)); }
                            if (!Math::weakEqual(l3BinWidth, w)) { Log(Log::Error) << "the width of an item does not fit its containing L3 bin." << endl; }
                            if (lowerWaste && upperWaste) { Log(Log::Error) << "more than one 4-cut is required to produce an item." << endl; }
                            break;
                        }
                        x3 += l3BinWidth;
                    }
                    y2 += l2BinHeight;
                }
                x1 += l1BinWidth;
            }
            if ((++g < maxBinNum[L0]) && mp.isTrue(p[g])) {
                sln.totalWidth += input.param.plateWidth;
            } else {
                sln.totalWidth += Math::lfloor(x1);
                break;
            }
        }

        // statistics.
        ID usedPlateNum = 0;
        for (; (usedPlateNum < maxBinNum[L0]) && mp.isTrue(p[usedPlateNum]); ++usedPlateNum) {}
        ID placedItemNum = 0;
        for (ID i = 0; i < itemNum; ++i) { placedItemNum += mp.isTrue(pi[i]); }
        Log(LogSwitch::Szx::Postprocess) << usedPlateNum << " plates are used to place " << placedItemNum << "/" << itemNum << " items." << endl;

        // visualization.
        constexpr double PlateGap = 100;
        constexpr double DefectHintRadius = 32;
        const char FontColor[] = "000000";
        const char FillColor[] = "CCFFCC";
        Drawer draw;
        draw.begin(env.visualizPath(), input.param.plateWidth, input.param.plateHeight, usedPlateNum, PlateGap);

        double offset = 0;
        for (ID g = 0; g < maxBinNum[L0]; ++g, offset += (input.param.plateHeight + PlateGap)) {
            // draw plate.
            draw.rect(0, offset, input.param.plateWidth, input.param.plateHeight);
            // draw items.
            double x1 = 0;
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                double y2 = offset;
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    double x3 = x1;
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        for (ID i = 0; i < itemNum; ++i) {
                            if (!mp.isTrue(pi3[i][g][l][m][n])) { continue; }
                            draw.rect(x3, y2 + mp.getValue(h4l[g][l][m][n]), aux.items[i].w, aux.items[i].h, mp.isTrue(d[i]), to_string(i), FontColor, FillColor);
                            break;
                        }
                        x3 += mp.getValue(w3[g][l][m][n]);
                    }
                    y2 += mp.getValue(h2[g][l][m]);
                }
                x1 += mp.getValue(w1[g][l]);
            }
            // draw cuts.
            x1 = 0;
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                if (Math::weakLess(mp.getValue(w1[g][l]), 0)) { continue; }
                double oldX1 = x1;
                x1 += mp.getValue(w1[g][l]);
                double y2 = offset;
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    if (Math::weakLess(mp.getValue(h2[g][l][m]), 0)) { continue; }
                    double oldY2 = y2;
                    y2 += mp.getValue(h2[g][l][m]);
                    double x3 = oldX1;
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        if (Math::weakLess(mp.getValue(w3[g][l][m][n]), 0)) { continue; }
                        x3 += mp.getValue(w3[g][l][m][n]);
                        draw.line(x3, oldY2, x3, y2, L3);
                    }
                    draw.line(oldX1, y2, x1, y2, L2);
                }
                draw.line(x1, offset, x1, offset + input.param.plateHeight, L1);
            }
            // draw defects.
            for (auto f = aux.plates[g].begin(); f != aux.plates[g].end(); ++f) {
                if ((aux.defects[*f].w < (2 * DefectHintRadius)) || (aux.defects[*f].h < (2 * DefectHintRadius))) {
                    draw.circle(aux.defects[*f].x + aux.defects[*f].w * 0.5, offset + aux.defects[*f].y + aux.defects[*f].h * 0.5, DefectHintRadius);
                }
                draw.rect(aux.defects[*f].x, offset + aux.defects[*f].y, aux.defects[*f].w, aux.defects[*f].h, false, "", FontColor, FontColor);
            }
        }

        draw.end();
    } else {
        Log(Log::Error) << "unable to find feasible solution." << endl;
    }
}

void Solver::optimizeIteratedModel(Solution &sln, Configuration::IteratedModel cfg) {
    using Dvar = MpSolver::DecisionVar;
    using Expr = MpSolver::LinearExpr;
    using VarType = MpSolver::VariableType;

    constexpr bool flawless = false; // there are defects on the plate.
    constexpr bool ordered = true; // the items should be produced in given order.

    // step control.
    enum Layer { L0, L1, L2, L3, L4 };
    constexpr ID maxBinNum[] = { 1, 1, 6, 6, 2 }; // define how many bins will be considered in model. // EXTEND[szx][5]: parameterize the constant!
    constexpr ID binNum = maxBinNum[L0] * maxBinNum[L1] * maxBinNum[L2] * maxBinNum[L3] * maxBinNum[L4];
    constexpr ID PlateStep = 1; // define how many plates will be accepted/recorded in each iteration.
    constexpr ID L1BinStep = 1; // define how many bins will be accepted/recorded in each iteration.

    ID stackNum = static_cast<ID>(aux.stacks.size());
    ID itemNum = static_cast<ID>(aux.items.size());

    double approxIter = max(1.5, itemNum / cfg.approxPlacedItemNumPerIteration); // TODO[szx][5]: parameterize the constant!
    double timeoutPerIteration = max(cfg.minSecTimeoutPerIteration, timer.restSeconds() / approxIter);
    Log(LogSwitch::Szx::Model) << "timeout per iteration=" << timeoutPerIteration << "s." << endl;

    // for visualization.
    List<Drawer::Rect> plates;
    List<Drawer::Item> items;
    List<Drawer::Cut> cuts;
    List<Drawer::Rect> flaws;
    ID usedPlateNum = 1;

    ID plateId = 0; // current plate.
    Length xOffset = 0; // left of current L1 bin in current plate.
    ID placedItemNum = 0;
    List<bool> isItemPlaced(itemNum, false);
    for (;;) {
        List<bool> skipItem(isItemPlaced); // select items to be considered in model.
        if (placedItemNum + cfg.maxItemToConsiderPerIteration < itemNum) {
            // OPTIMIZE[szx][0]: add some randomness?
            List<List<ID>::iterator> sp(aux.stacks.size());
            for (ID s = 0; s < stackNum; ++s) { sp[s] = aux.stacks[s].begin(); }
            ID itemToSelectNum = cfg.maxItemToConsiderPerIteration;
            for (ID s = 0; ; (++s) %= stackNum) { // seek to the last selected item.
                while ((sp[s] != aux.stacks[s].end()) && skipItem[*sp[s]]) { ++sp[s]; }
                if (sp[s] == aux.stacks[s].end()) { continue; }
                if (--itemToSelectNum <= 0) { break; }
            }
            for (ID s = 0; s < stackNum; ++s) { // mark the following items as skipped.
                if (sp[s] == aux.stacks[s].end()) { continue; }
                while (++sp[s] != aux.stacks[s].end()) { skipItem[*sp[s]] = true; }
            }
        }

        MpSolver::Configuration mpcfg(MpSolver::Configuration::DefaultSolver, timer.restSeconds(), true, false);
        MpSolver mp(mpcfg);
        //mp.setMaxThread(2);

        Arr<Dvar> d(itemNum); // direction (if i should rotate 90 degree).

        Arr2D<Dvar> p1(maxBinNum[L0], maxBinNum[L1]); // there are some items placed in L1 virtual bin.
        Arr2D<Arr<Dvar>> p2(maxBinNum[L0], maxBinNum[L1], Arr<Dvar>(maxBinNum[L2])); // there are some items placed in L2 virtual bin.
        Arr2D<Arr2D<Dvar>> p3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // there are some items placed in L3 virtual bin.
        Arr2D<Arr2D<Arr<Dvar>>> p4(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3], Arr<Dvar>(maxBinNum[L4]))); // there are some items placed in L4 virtual bin.
        Arr2D<Arr2D<Arr2D<Dvar>>> pi4(itemNum, maxBinNum[L0], Arr2D<Arr2D<Dvar>>(maxBinNum[L1], maxBinNum[L2], Arr2D<Dvar>(maxBinNum[L3], maxBinNum[L4]))); // item placed in L3 virtual bin.

        Arr2D<Dvar> w1(maxBinNum[L0], maxBinNum[L1]); // width of L1 virtual bin.
        Arr2D<Arr<Dvar>> h2(maxBinNum[L0], maxBinNum[L1], Arr<Dvar>(maxBinNum[L2])); // height of L2 virtual bin.
        Arr2D<Arr2D<Dvar>> w3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // width of L3 virtual bin.
        Arr2D<Arr2D<Arr<Dvar>>> h4(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3], Arr<Dvar>(maxBinNum[L4]))); // height of L4 virtual bin.

        Arr2D<Arr2D<Dvar>> t3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // L3 virtual bin is non-trivial.
        Arr2D<Arr2D<Arr<Dvar>>> t4(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Dvar>>(maxBinNum[L2], maxBinNum[L3], Arr<Dvar>(maxBinNum[L4]))); // L4 virtual bin is non-trivial.

        Arr<Dvar> e(maxBinNum[L0]); // there is residual on plate.

        Arr2D<Arr2D<Arr<Arr<Dvar>>>> c(maxBinNum[L0], maxBinNum[L1], Arr2D<Arr<Arr<Dvar>>>(maxBinNum[L2], maxBinNum[L3], Arr<Arr<Dvar>>(maxBinNum[L4]))); // L4 virtual bin covers the flaw.
        Arr<Arr<Dvar>> cr(maxBinNum[L0]); // the residual covers the flaw.

        Expr coveredArea; // the sum of placed items' area.
        Expr coveredWidth;
        Expr placedItem;
        Arr<Expr> pi(itemNum);

        Log(LogSwitch::Szx::Model) << "add decisions variables." << endl;
        for (ID i = 0; i < itemNum; ++i) {
            if (skipItem[i]) { continue; }
            d[i] = mp.addVar(VarType::Bool, 0, 1);
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                pi4[i][g][l][m][n][k] = mp.addVar(VarType::Bool, 0, 1);
                            }
                        }
                    }
                }
            }
        }
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            ID defectNum = static_cast<ID>(aux.plates[plateId + g].size());
            cr[g].init(defectNum);
            for (ID f = 0; f < defectNum; ++f) { cr[g][f] = mp.addVar(VarType::Bool, 0, 1); }
            e[g] = mp.addVar(VarType::Bool, 0, 1);
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                p1[g][l] = mp.addVar(VarType::Bool, 0, 1);
                w1[g][l] = mp.addVar(VarType::Real, 0, input.param.plateWidth); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    p2[g][l][m] = mp.addVar(VarType::Bool, 0, 1);
                    h2[g][l][m] = mp.addVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        p3[g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                        w3[g][l][m][n] = mp.addVar(VarType::Real, 0, input.param.plateWidth); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                        t3[g][l][m][n] = mp.addVar(VarType::Bool, 0, 1);
                        for (ID k = 0; k < maxBinNum[L4]; ++k) {
                            p4[g][l][m][n][k] = mp.addVar(VarType::Bool, 0, 1);
                            h4[g][l][m][n][k] = mp.addVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                            t4[g][l][m][n][k] = mp.addVar(VarType::Bool, 0, 1);
                            c[g][l][m][n][k].init(defectNum);
                            for (ID f = 0; f < defectNum; ++f) {
                                c[g][l][m][n][k][f] = mp.addVar(VarType::Bool, 0, 1);
                            }
                        }
                    }
                }
            }
        }

        for (ID g = 1; g < maxBinNum[L0]; ++g) { coveredWidth += input.param.plateWidth; }
        for (ID l = 0; l < maxBinNum[L1]; ++l) { coveredWidth += w1[PlateStep - 1][l]; }

        Log(LogSwitch::Szx::Model) << "add constraints." << endl;
        Log(LogSwitch::Szx::Model) << "add placement constraints." << endl;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                Expr l1BinItems;
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    Expr l2BinItems;
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        Expr l3BinItems;
                        for (ID k = 0; k < maxBinNum[L4]; ++k) {
                            Expr l4BinItems;
                            for (ID i = 0; i < itemNum; ++i) {
                                if (skipItem[i]) { continue; }
                                l4BinItems += pi4[i][g][l][m][n][k];
                            }
                            l3BinItems += l4BinItems;
                            // exclusive placement.
                            if (aux.plates[plateId + g].empty()) { mp.addConstraint(l4BinItems <= 1); } // in case there is no defect on this plate and the defect free constraint is not added.
                            // L4 item placement.
                            mp.addConstraint(l4BinItems == p4[g][l][m][n][k]); // OPTIMIZE[szx][5]: can be <= if the obj is not maximizing the area of placed items.
                        }
                        l2BinItems += l3BinItems;
                        // L3 item placement.
                        mp.addConstraint(p3[g][l][m][n] <= l3BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
                        mp.addConstraint(l3BinItems <= itemNum * p3[g][l][m][n]);
                    }
                    l1BinItems += l2BinItems;
                    // L2 item placement.
                    mp.addConstraint(p2[g][l][m] <= l2BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
                    mp.addConstraint(l2BinItems <= itemNum * p2[g][l][m]);
                }
                // L1 item placement.
                mp.addConstraint(p1[g][l] <= l1BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
                mp.addConstraint(l1BinItems <= itemNum * p1[g][l]);
            }
        }

        Log(LogSwitch::Szx::Model) << "add fitting constraints." << endl;
        auto width = [&](ID itemId) {
            return (aux.items[itemId].w * (1 - d[itemId]) + aux.items[itemId].h * d[itemId]);
        };
        auto height = [&](ID itemId) {
            return (aux.items[itemId].h * (1 - d[itemId]) + aux.items[itemId].w * d[itemId]);
        };
        for (ID i = 0; i < itemNum; ++i) {
            if (skipItem[i]) { continue; }
            Length itemLen = aux.items[i].w;
            Expr putInBin;
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                putInBin += pi4[i][g][l][m][n][k];
                                // horizontal fitting.
                                mp.addConstraint(width(i) - itemLen * (1 - pi4[i][g][l][m][n][k]) <= w3[g][l][m][n]);
                                mp.addConstraint(w3[g][l][m][n] <= width(i) + input.param.plateWidth * (1 - pi4[i][g][l][m][n][k]));
                                // vertical fitting.
                                mp.addConstraint(height(i) - itemLen * (1 - pi4[i][g][l][m][n][k]) <= h4[g][l][m][n][k]);
                                mp.addConstraint(h4[g][l][m][n][k] <= height(i) + input.param.plateHeight * (1 - pi4[i][g][l][m][n][k]));
                            }
                        }
                    }
                }
            }
            // single placement.
            mp.addConstraint(putInBin <= 1);
            // covered area.
            coveredArea += (aux.items[i].w * aux.items[i].h * putInBin);
            placedItem += putInBin;
            pi[i] += putInBin;
        }

        Log(LogSwitch::Szx::Model) << "add composition constraints." << endl;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            Expr l1BinWidthSum;
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                l1BinWidthSum += w1[g][l];
                Expr l2BinHeightSum;
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    l2BinHeightSum += h2[g][l][m];
                    Expr l3BinWidthSum;
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        l3BinWidthSum += w3[g][l][m][n];
                        Expr l4BinHeightSum;
                        for (ID k = 0; k < maxBinNum[L4]; ++k) {
                            l4BinHeightSum += h4[g][l][m][n][k];
                        }
                        // L4 total height.
                        mp.addConstraint(l4BinHeightSum == h2[g][l][m]);
                    }
                    // L3 total width.
                    mp.addConstraint(l3BinWidthSum == w1[g][l]);
                }
                // L2 total height.
                mp.addConstraint(l2BinHeightSum == input.param.plateHeight);
            }
            // L1 total width.
            Length fixedWidth = ((g == 0) ? xOffset : 0);
            mp.addConstraint(input.param.plateWidth * (1 - e[g]) <= fixedWidth + l1BinWidthSum);
            mp.addConstraint(fixedWidth + l1BinWidthSum <= input.param.plateWidth - input.param.minWasteWidth * e[g]);
        }

        Log(LogSwitch::Szx::Model) << "add bounding constraints." << endl;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                // L1 width bound.
                mp.addConstraint(input.param.minL1Width * p1[g][l] <= w1[g][l]);
                mp.addConstraint(w1[g][l] <= input.param.maxL1Width); // OPTIMIZE[szx][9]: set the bound when adding the variable?
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    // L2 min height.
                    mp.addConstraint(input.param.minL2Height * p2[g][l][m] <= h2[g][l][m]);
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        // L3 min width.
                        mp.addConstraint(input.param.minWasteWidth * t3[g][l][m][n] - input.param.plateWidth * p3[g][l][m][n] <= w3[g][l][m][n]);
                        mp.addConstraint(w3[g][l][m][n] <= input.param.plateWidth * t3[g][l][m][n] + input.param.plateWidth * p3[g][l][m][n]);
                        for (ID k = 0; k < maxBinNum[L4]; ++k) {
                            // L4 min height.
                            mp.addConstraint(input.param.minWasteHeight * t4[g][l][m][n][k] - input.param.plateHeight * p4[g][l][m][n][k] <= h4[g][l][m][n][k]);
                            mp.addConstraint(h4[g][l][m][n][k] <= input.param.plateHeight * t4[g][l][m][n][k] + input.param.plateHeight * p4[g][l][m][n][k]);
                        }
                    }
                }
            }
        }

        if (ordered) {
            Log(LogSwitch::Szx::Model) << "add ordering constraints." << endl;
            // item order.
            for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
                for (auto i = s->begin(), j = s->begin() + 1; j != s->end(); ++i, ++j) {
                    if (skipItem[*i] || skipItem[*j]) { continue; }
                    Expr putInBin;
                    for (ID g = 0; g < maxBinNum[L0]; ++g) {
                        for (ID l = 0; l < maxBinNum[L1]; ++l) {
                            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                                    for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                        if (putInBin.size() > 0) { mp.addConstraint(1 - pi4[*i][g][l][m][n][k] >= putInBin); }
                                        putInBin += pi4[*j][g][l][m][n][k];
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // an item can not be placed if its preceding item is not placed.
            for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
                Expr prevPutInBin;
                for (auto i = s->begin(); i != s->end(); ++i) {
                    if (skipItem[*i]) { continue; }
                    Expr nextPutInBin;
                    for (ID g = 0; g < maxBinNum[L0]; ++g) {
                        for (ID l = 0; l < maxBinNum[L1]; ++l) {
                            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                                    for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                        nextPutInBin += pi4[*i][g][l][m][n][k];
                                    }
                                }
                            }
                        }
                    }
                    if (prevPutInBin.size() > 0) { mp.addConstraint(prevPutInBin >= nextPutInBin); }
                    prevPutInBin = nextPutInBin; // OPTIMIZE[szx][9]: use pi[i]?
                }
            }
        }

        if (!flawless) {
            Log(LogSwitch::Szx::Model) << "add defect constraints." << endl;
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                Expr l1BinWidthSum;
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    l1BinWidthSum += w1[g][l];
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                ID f = 0;
                                for (auto gf = aux.plates[plateId + g].begin(); gf != aux.plates[plateId + g].end(); ++gf, ++f) {
                                    if ((aux.defects[*gf].x + aux.defects[*gf].w) <= xOffset) { continue; } // skip passed flaws (it is impossible to cover them in any bins).
                                    // defect free.
                                    mp.addConstraint(p4[g][l][m][n][k] <= 1 - c[g][l][m][n][k][f]);
                                    // defect covering.
                                    Expr fx = ((g == 0) ? xOffset : 0);
                                    for (ID ll = 0; ll < l; ++ll) { fx += w1[g][ll]; }
                                    for (ID nn = 0; nn < n; ++nn) { fx += w3[g][l][m][nn]; }
                                    Expr fy;
                                    for (ID mm = 0; mm < m; ++mm) { fy += h2[g][l][mm]; }
                                    mp.addConstraint(fx - input.param.plateWidth * (1 - c[g][l][m][n][k][f]) <= aux.defects[*gf].x);
                                    mp.addConstraint(fx + w3[g][l][m][n] + input.param.plateWidth * (1 - c[g][l][m][n][k][f]) >= aux.defects[*gf].x + aux.defects[*gf].w);
                                    mp.addConstraint(fy - input.param.plateHeight * (1 - c[g][l][m][n][k][f]) <= aux.defects[*gf].y);
                                    mp.addConstraint(fy + h4[g][l][m][n][k] + input.param.plateHeight * (1 - c[g][l][m][n][k][f]) >= aux.defects[*gf].y + aux.defects[*gf].h);
                                }
                            }
                        }
                    }
                }
                ID f = 0;
                for (auto gf = aux.plates[plateId + g].begin(); gf != aux.plates[plateId + g].end(); ++gf, ++f) {
                    if ((aux.defects[*gf].x + aux.defects[*gf].w) <= xOffset) { continue; } // skip passed flaws (it is impossible to cover them in any bins).
                    Expr defectCovered;
                    for (ID g = 0; g < maxBinNum[L0]; ++g) {
                        for (ID l = 0; l < maxBinNum[L1]; ++l) {
                            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                                    for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                        defectCovered += c[g][l][m][n][k][f];
                                    }
                                }
                            }
                        }
                    }
                    // defect bypassing.
                    mp.addConstraint(cr[g][f] + defectCovered == 1);
                    // defect covering (residual).
                    Length fixedWidth = ((g == 0) ? xOffset : 0);
                    mp.addConstraint(fixedWidth + l1BinWidthSum - input.param.plateWidth * (1 - cr[g][f]) <= aux.defects[*gf].x);
                }
            }
        }

        if (cfg.addBinSizeOrderCut) {
            Log(LogSwitch::Szx::Model) << "add bin size order cuts." << endl;
            // if there is no defect and order, put larger bins to the left or bottom.
            // else put all non-trivial bins to the left or bottom.
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            for (ID k = 1; k < maxBinNum[L4]; ++k) {
                                mp.addConstraint(t4[g][l][m][n][k - 1] >= t4[g][l][m][n][k]);
                                if (flawless && !ordered) { mp.addConstraint(h4[g][l][m][n][k - 1] >= h4[g][l][m][n][k]); }
                            }
                            if (n < 1) { continue; }
                            mp.addConstraint(t3[g][l][m][n - 1] >= t3[g][l][m][n]);
                            if (flawless && !ordered) { mp.addConstraint(w3[g][l][m][n - 1] >= w3[g][l][m][n]); }
                        }
                        if (m < 1) { continue; }
                        mp.addConstraint(t4[g][l][m - 1][0][0] >= t4[g][l][m][0][0]);
                        if (flawless && !ordered) { mp.addConstraint(h2[g][l][m - 1] >= h2[g][l][m]); }
                    }
                    if (l < 1) { continue; }
                    mp.addConstraint(t3[g][l - 1][0][0] >= t3[g][l][0][0]);
                    if (flawless && !ordered) { mp.addConstraint(w1[g][l - 1] >= w1[g][l]); }
                }
            }
        }

        if (cfg.addEmptyBinMergingCut) {
            Log(LogSwitch::Szx::Model) << "add empty bin merging cuts." << endl;
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    // OPTIMIZE[szx][0]: make sure adding this will not cut the optima! (some defects make an area wider than input.param.plateWidth that can not place any item)
                    if (l > 0) { mp.addConstraint(p1[g][l - 1] + p1[g][l] >= t3[g][l][0][0]); }
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        if (m > 0) { mp.addConstraint(p2[g][l][m - 1] + p2[g][l][m] >= t4[g][l][m][0][0]); }
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            if (n > 0) { mp.addConstraint(p3[g][l][m][n - 1] + p3[g][l][m][n] >= t3[g][l][m][n]); }
                            for (ID k = 1; k < maxBinNum[L4]; ++k) {
                                mp.addConstraint(p4[g][l][m][n][k - 1] + p4[g][l][m][n][k] >= t4[g][l][m][n][k]);
                            }
                        }
                    }
                }
            }
        }

        if (cfg.addAreaBoundCut) { // user cut for covered area should be less than the plate or bin.
            Log(LogSwitch::Szx::Model) << "add area bound cuts." << endl;
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                Expr plateArea;
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    Expr l1BinArea;
                    for (ID i = 0; i < itemNum; ++i) {
                        if (skipItem[i]) { continue; }
                        Expr putInBin;
                        for (ID m = 0; m < maxBinNum[L2]; ++m) {
                            for (ID n = 0; n < maxBinNum[L3]; ++n) {
                                for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                    putInBin += pi4[i][g][l][m][n][k];
                                }
                            }
                        }
                        l1BinArea += (aux.items[i].w * aux.items[i].h * putInBin);
                    }
                    mp.addConstraint(l1BinArea <= input.param.maxL1Width * input.param.plateHeight);
                    plateArea += l1BinArea;
                }
                mp.addConstraint(plateArea <= input.param.plateWidth * input.param.plateHeight);
            }
        }

        if (cfg.addL1BinWidthSumCut) {
            Log(LogSwitch::Szx::Model) << "add min L1 width sum cut." << endl;
            mp.addConstraint(input.param.plateHeight * coveredWidth >= coveredArea);
        }

        // TODO[szx][0]: minimize (width used to place in this iteration) + (estimated width to be used in later iteration) and remove the following constraint?
        // in case the optimal ratio is below optRatio (then the optima of this transformed obj will be 0 and nothing will be placed).
        mp.addConstraint(placedItem >= 1);

        Log(LogSwitch::Szx::Model) << "add objectives." << endl;
        // minimize the wasted area.
        // OPTIMIZE[szx][2]: set it to greater value or make more iterations? this may not get better result since the construction itself is empirical.
        double optRatio = cfg.initCoverageRatio;
        mp.addObjective(coveredArea - optRatio * input.param.plateHeight * coveredWidth, MpSolver::OptimaOrientation::Maximize, 0, 0, 0, timeoutPerIteration);

        // solve.
        if (cfg.setMipFocus) { mp.setMipFocus(MpSolver::MipFocusMode::ImproveFeasibleSolution); }
        bool feasible = mp.optimize();
        if (feasible) {
            // record solution.
            using Node = Problem::Output::Node;
            auto getLen = [&](const Dvar &dvar, double &margin) {
                double val = mp.getValue(dvar);
                if (Math::weakLess(val, 0)) { return 0; }
                val += margin;
                Length length = Math::lfloor(val);
                margin = (val - length);
                return length;
            };
            for (ID g = 0; g < PlateStep; ++g) {
                if ((plateId + g) >= static_cast<ID>(sln.bins.size())) {
                    sln.bins.push_back(Bin(0, 0, input.param.plateWidth, input.param.plateHeight, Node::SpecialType::Branch));
                }
                Bin &plate(sln.bins.back());
                Length x1 = ((g == 0) ? xOffset : 0);
                double margin1 = 0; // cumulative length loss due to truncation from double to int.
                for (ID l = 0; l < L1BinStep; ++l) {
                    Length l1BinWidth = getLen(w1[g][l], margin1);
                    if (l1BinWidth <= 0) { continue; }
                    plate.children.push_back(Bin(x1, 0, l1BinWidth, input.param.plateHeight, Node::SpecialType::Waste));
                    Bin &l1Bin(plate.children.back());
                    Length y2 = 0;
                    double margin2 = 0; // cumulative length loss due to truncation from double to int.
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        Length l2BinHeight = getLen(h2[g][l][m], margin2);
                        if (l2BinHeight <= 0) { continue; }
                        l1Bin.children.push_back(Bin(x1, y2, l1BinWidth, l2BinHeight, Node::SpecialType::Waste));
                        Bin &l2Bin(l1Bin.children.back());
                        Length x3 = x1;
                        double margin3 = 0; // cumulative length loss due to truncation from double to int.
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            Length l3BinWidth = getLen(w3[g][l][m][n], margin3);
                            if (l3BinWidth <= 0) { continue; }
                            l2Bin.children.push_back(Bin(x3, y2, l3BinWidth, l2BinHeight, Node::SpecialType::Waste));
                            Bin &l3Bin(l2Bin.children.back());
                            Length y4 = y2;
                            double margin4 = 0;
                            for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                Length l4BinHeight = getLen(h4[g][l][m][n][k], margin4);
                                if (l4BinHeight <= 0) { continue; }
                                l3Bin.children.push_back(Bin(x3, y4, l3BinWidth, l4BinHeight, Node::SpecialType::Waste));
                                Bin &l4Bin(l3Bin.children.back());
                                for (ID i = 0; i < itemNum; ++i) {
                                    if (skipItem[i] || !mp.isTrue(pi4[i][g][l][m][n][k])) { continue; }
                                    l1Bin.type = l2Bin.type = l3Bin.type = Node::SpecialType::Branch;
                                    l4Bin.type = i;
                                    #if SZX_DEBUG
                                    Length w = (mp.isTrue(d[i])) ? aux.items[i].h : aux.items[i].w;
                                    Length h = (mp.isTrue(d[i])) ? aux.items[i].w : aux.items[i].h;
                                    if (!Math::weakEqual(l3BinWidth, w)) { Log(Log::Error) << "the width of an item does not fit its containing L3 bin." << endl; }
                                    if (!Math::weakEqual(l4BinHeight, h)) { Log(Log::Error) << "the height of an item does not fit its containing L4 bin." << endl; }
                                    #endif // SZX_DEBUG
                                    break;
                                }
                                y4 += l4BinHeight;
                            }
                            x3 += l3BinWidth;
                        }
                        y2 += l2BinHeight;
                    }
                    x1 += l1BinWidth;
                }
            }

            #if SZX_DEBUG
            // visualization.
            constexpr double DefectHintRadius = 32;
            const char FontColor[] = "000000";
            const char FillColor[] = "CCFFCC";
            constexpr double PlateGap = 100;
            double offset = (input.param.plateHeight + PlateGap) * plateId;
            for (ID g = 0; g < PlateStep; ++g, offset += (input.param.plateHeight + PlateGap)) {
                // draw plate.
                if ((plateId + g) >= static_cast<ID>(plates.size())) {
                    plates.push_back(Drawer::Rect(0, offset, input.param.plateWidth, input.param.plateHeight));
                }
                // draw items.
                double x1 = ((g == 0) ? xOffset : 0);
                for (ID l = 0; l < L1BinStep; ++l) {
                    double y2 = offset;
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        double x3 = x1;
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            double y4 = y2;
                            for (ID k = 0; k < maxBinNum[L4]; ++k) {
                                for (ID i = 0; i < itemNum; ++i) {
                                    if (skipItem[i] || !mp.isTrue(pi4[i][g][l][m][n][k])) { continue; }
                                    items.push_back(Drawer::Item(x3, y4, aux.items[i].w, aux.items[i].h, mp.isTrue(d[i]), i));
                                    break;
                                }
                                y4 += mp.getValue(h4[g][l][m][n][k]);
                            }
                            x3 += mp.getValue(w3[g][l][m][n]);
                        }
                        y2 += mp.getValue(h2[g][l][m]);
                    }
                    x1 += mp.getValue(w1[g][l]);
                }
                // draw cuts.
                x1 = ((g == 0) ? xOffset : 0);
                for (ID l = 0; l < L1BinStep; ++l) {
                    if (Math::weakLess(mp.getValue(w1[g][l]), 0)) { continue; }
                    double oldX1 = x1;
                    x1 += mp.getValue(w1[g][l]);
                    double y2 = offset;
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        if (Math::weakLess(mp.getValue(h2[g][l][m]), 0)) { continue; }
                        double oldY2 = y2;
                        y2 += mp.getValue(h2[g][l][m]);
                        double x3 = oldX1;
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            if (Math::weakLess(mp.getValue(w3[g][l][m][n]), 0)) { continue; }
                            x3 += mp.getValue(w3[g][l][m][n]);
                            cuts.push_back(Drawer::Cut(x3, oldY2, x3, y2, L3));
                        }
                        cuts.push_back(Drawer::Cut(oldX1, y2, x1, y2, L2));
                    }
                    cuts.push_back(Drawer::Cut(x1, offset, x1, offset + input.param.plateHeight, L1));
                }
                // draw defects.
                for (auto f = aux.plates[plateId + g].begin(); f != aux.plates[plateId + g].end(); ++f) {
                    flaws.push_back(Drawer::Rect(aux.defects[*f].x, offset + aux.defects[*f].y, aux.defects[*f].w, aux.defects[*f].h));
                }
            }

            Drawer draw;
            draw.begin(env.visualizPath(), input.param.plateWidth, input.param.plateHeight, usedPlateNum, PlateGap);
            for (auto g = plates.begin(); g != plates.end(); ++g) {
                draw.rect(g->x, g->y, g->w, g->h);
            }
            for (auto i = items.begin(); i != items.end(); ++i) {
                draw.rect(i->x, i->y, i->w, i->h, i->d, to_string(i->i), FontColor, FillColor);
            }
            for (auto c = cuts.begin(); c != cuts.end(); ++c) {
                draw.line(c->x0, c->y0, c->x1, c->y1, c->l);
            }
            for (auto f = flaws.begin(); f != flaws.end(); ++f) {
                if ((f->w < (2 * DefectHintRadius)) || (f->h < (2 * DefectHintRadius))) {
                    draw.circle(f->x + f->w * 0.5, f->y + f->h * 0.5, DefectHintRadius);
                }
                draw.rect(f->x, f->y, f->w, f->h, false, "", FontColor, FontColor);
            }
            draw.end();
            #endif // SZX_DEBUG

            // statistics.
            double width = (PlateStep > 1) ? 0 : xOffset;
            for (ID l = 0; l < L1BinStep; ++l) { width += mp.getValue(w1[PlateStep - 1][l]); }
            xOffset = Math::lfloor(width);
            Log(LogSwitch::Szx::Postprocess) << "x offset=" << xOffset << endl;
            Log(LogSwitch::Szx::Postprocess) << "coverage ratio=" << mp.getValue(coveredArea) / (input.param.plateHeight * mp.getValue(coveredWidth)) << endl;
            for (ID i = 0; i < itemNum; ++i) {
                if (!mp.isTrue(pi[i])) { continue; }
                ++placedItemNum;
                isItemPlaced[i] = true;
            }
            Log(LogSwitch::Szx::Postprocess) << usedPlateNum << " plates are used to place " << placedItemNum << "/" << itemNum << " items." << endl;

            // exit criteria.
            if (timer.isTimeOut() || (placedItemNum >= itemNum)) {
                // record obj.
                sln.totalWidth = input.param.plateWidth * plateId + xOffset;
                break;
            }
        } else {
            // TODO[szx][0]: what if there are defects and there must be an empty L1 bin or leave it empty will be better?
            ++usedPlateNum;
            ++plateId;
            xOffset = 0;
            Log(LogSwitch::Szx::Postprocess) << "use new plate " << plateId << endl;
        }
    }
}
#pragma endregion Solver

#pragma region Solver::Solution
Solver::Solution::operator Problem::Output() {
    Problem::Output output;

    output.totalWidth = totalWidth;
    ID id = 0;
    ID plateId = 0;
    for (auto g = bins.begin(); g != bins.end(); ++g, ++plateId) {
        traverse(*g, 0, Problem::InvalidItemId, [&](Bin &bin, ID depth, ID parent) {
            Problem::Output::Node node;
            node.plateId = plateId;
            node.id = id++;
            node.x = bin.rect.x;
            node.y = bin.rect.y;
            node.width = bin.rect.w;
            node.height = bin.rect.h;
            node.type = ((bin.type < 0) ? bin.type : solver->idMap.item.toArbitraryId(bin.type)); // TODO[szx][0]: merge nodes if an item is put in this L1/L2 bin directly and solely.
            node.cut = depth;
            node.parent = parent;
            output.nodes.push_back(node);
            return node.id;
        });
    }
    // mark the last residual as real residual.
    if (!output.nodes.empty() && (output.nodes.back().cut == 1)
        && (output.nodes.back().type == Problem::Output::Node::SpecialType::Waste)) {
        output.nodes.back().type = Problem::Output::Node::SpecialType::Residual;
    }

    return output;
}

void Solver::Solution::traverse(Bin &bin, ID depth, ID parent, OnNode onNode) {
    ID id = onNode(bin, depth, parent);
    if (bin.children.empty()) { return; }

    bool horizontal = Math::isOdd(++depth);
    Length dx = 0;
    Length dy = 0;
    for (auto b = bin.children.begin(); b != bin.children.end(); ++b) {
        dx += (horizontal ? b->rect.w : 0);
        dy += (horizontal ? 0 : b->rect.h);
        traverse(*b, depth, id, onNode);
    }
    if ((dx >= bin.rect.w) || (dy >= bin.rect.h)) { return; } // the bin is filled up.
    // add the waste to the bin.
    Bin b(RectArea(bin.rect.x + dx, bin.rect.y + dy, bin.rect.w - dx, bin.rect.h - dy),
        Problem::Output::Node::SpecialType::Waste);
    onNode(b, depth, id);
    if (depth > 1) { Log(Log::Error) << "unexpected waste detected." << endl; }
}
#pragma endregion Solver::Solution

}
