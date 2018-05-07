#include "Solver.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

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
    solver.output.save(env.solutionPathWithSuffix());
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

    List<Problem::Output> outputs(env.jobNum);

    Log(LogSwitch::Szx::Framework) << "launch " << env.jobNum << " workers." << endl;
    List<thread> threadList;
    threadList.reserve(env.jobNum);
    for (int i = 0; i < env.jobNum; ++i) {
        // OPTIMIZE[szx][3]: add a list to specify a series of algorithm to be used by each threads in sequence.
        threadList.emplace_back([&]() { optimize(outputs[i], i); });
    }
    for (int i = 0; i < env.jobNum; ++i) { threadList.at(i).join(); }

    Log(LogSwitch::Szx::Framework) << "collect best result among all workers." << endl;
    output = *min_element(outputs.begin(), outputs.end(),
        [](Problem::Output &l, Problem::Output &r) { return (l.totalWidth < r.totalWidth); });
}

void Solver::record() const {
    int generation = 0, iteration = 0;

    System::MemoryUsage mu = System::peakMemoryUsage();

    ostringstream log;

    // record basic information.
    log << env.friendlyLocalTime() << ","
        << env.rid << ","
        << env.instName << ","
        << checkFeasibility() << "," << checkObjective() << ","
        << output.totalWidth << ","
        << timer.elapsedSeconds() << ","
        << mu.physicalMemory << "," << mu.virtualMemory << ","
        << env.randSeed << ","
        << cfg.toBriefStr() << ","
        << generation << "," << iteration << ",";

    // record solution vector.
    // EXTEND[szx][2]: save solution in log.
    log << endl;

    // append all text atomically.
    static mutex logFileMutex;
    lock_guard<mutex> logFileGuard(logFileMutex);

    ofstream logFile(env.logPath, ios::app);
    logFile.seekp(0, ios::end);
    if (logFile.tellp() <= 0) {
        logFile << "Time,ID,Instance,Feasible,ObjMatch,ObjValue,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration,Solution" << endl;
    }
    logFile << log.str();
    logFile.close();
}

bool Solver::checkFeasibility() const {
    // TODO[szx][4]: check feasibility.
    return false;
}

bool Solver::checkObjective() const {
    // TODO[szx][4]: check objective.
    return false;
}

void Solver::init() {
    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());

    // assign internal id to items and stacks, then push items into stacks.
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        ID itemId = idMap.item.toConsecutiveId(i->id);
        aux.items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));

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

void Solver::optimize(Problem::Output &optimum, ID workerId) {
    Log(LogSwitch::Szx::Framework) << "worker " << workerId << " starts." << endl;

    optimizeSinglePlate();

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
void Solver::optimizeSinglePlate() {
    using Dvar = MpSolver::DecisionVar;
    using Expr = MpSolver::LinearExpr;
    using VarType = MpSolver::VariableType;

    enum Layer { L0, L1, L2, L3 };
    constexpr ID maxBinNum[] = { 2, 12, 10, 8 };
    constexpr bool flawless = false;
    constexpr bool maxCoveredArea = true;

    MpSolver::Configuration mpcfg(MpSolver::Configuration::DefaultSolver, timer.restSeconds(), true, true);
    MpSolver mp(mpcfg);

    Arr<Dvar> d(aux.items.size()); // direction (if i should rotate 90 degree).

    Arr<Dvar> p(maxBinNum[L0]); // item placed in plate.
    Arr2D<Dvar> p1(maxBinNum[L0], maxBinNum[L1]); // there are some items placed in L1 virtual bin.
    Arr2D<Arr<Dvar>> p2(maxBinNum[L0], maxBinNum[L1], Arr<Dvar>(maxBinNum[L2])); // there are some items placed in L2 virtual bin.
    Arr2D<Arr2D<Dvar>> p3(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3])); // there are some items placed in L3 virtual bin.
    Arr<Arr2D<Arr2D<Dvar>>> pi3(aux.items.size(), Arr2D<Arr2D<Dvar>>(maxBinNum[L0], maxBinNum[L1], Arr2D<Dvar>(maxBinNum[L2], maxBinNum[L3]))); // item placed in L3 virtual bin.

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

    Expr coveredArea; // the sum of placed items' area.
    Expr coveredWidth;

    Log(LogSwitch::Szx::Model) << "add decisions variables." << endl;
    for (ID i = 0; i < aux.items.size(); ++i) {
        d[i] = mp.makeVar(VarType::Bool, 0, 1);
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        pi3[i][g][l][m][n] = mp.makeVar(VarType::Bool, 0, 1);
                    }
                }
            }
        }
    }
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        p[g] = mp.makeVar(VarType::Bool, 0, 1);
        e[g] = mp.makeVar(VarType::Bool, 0, 1);
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            p1[g][l] = mp.makeVar(VarType::Bool, 0, 1);
            w1[g][l] = mp.makeVar(VarType::Real, 0, input.param.plateWidth); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                p2[g][l][m] = mp.makeVar(VarType::Bool, 0, 1);
                h2[g][l][m] = mp.makeVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                t2[g][l][m] = mp.makeVar(VarType::Bool, 0, 1);
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    p3[g][l][m][n] = mp.makeVar(VarType::Bool, 0, 1);
                    w3[g][l][m][n] = mp.makeVar(VarType::Real, 0, input.param.plateWidth); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    h4u[g][l][m][n] = mp.makeVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    h4l[g][l][m][n] = mp.makeVar(VarType::Real, 0, input.param.plateHeight); // OPTIMIZE[szx][7]: use MpSolver::Infinity?
                    t3[g][l][m][n] = mp.makeVar(VarType::Bool, 0, 1);
                    t4u[g][l][m][n] = mp.makeVar(VarType::Bool, 0, 1);
                    t4l[g][l][m][n] = mp.makeVar(VarType::Bool, 0, 1);
                }
            }
        }
    }
    r = mp.makeVar(VarType::Real, 0, input.param.plateWidth);

    Log(LogSwitch::Szx::Model) << "add constraints." << endl;
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        Expr plateItems;
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            Expr l1BinItems;
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                Expr l2BinItems;
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    Expr l3BinItems;
                    for (ID i = 0; i < aux.items.size(); ++i) { l3BinItems += pi3[i][g][l][m][n]; }
                    l2BinItems += l3BinItems;
                    // exclusive placement.
                    mp.makeConstraint(l3BinItems <= 1);
                    // L3 item placement.
                    mp.makeConstraint(l3BinItems == p3[g][l][m][n]); // OPTIMIZE[szx][5]: can be <= if the obj is not maximizing the area of placed items.
                }
                l1BinItems += l2BinItems;
                // L2 item placement.
                mp.makeConstraint(p2[g][l][m] <= l2BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
                mp.makeConstraint(l2BinItems <= aux.items.size() * p2[g][l][m]);
            }
            plateItems += l1BinItems;
            // L1 item placement.
            mp.makeConstraint(p1[g][l] <= l1BinItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
            mp.makeConstraint(l1BinItems <= aux.items.size() * p1[g][l]);
        }
        // plate used.
        mp.makeConstraint(p[g] <= plateItems); // OPTIMIZE[szx][5]: can be omitted if the obj is not maximizing the area of placed items.
        mp.makeConstraint(plateItems <= aux.items.size() * p[g]);

        coveredWidth += input.param.plateWidth * p[g];
    }
    coveredWidth += r;

    auto width = [&](ID itemId) {
        return (aux.items[itemId].w * (1 - d[itemId]) + aux.items[itemId].h * d[itemId]);
    };
    auto height = [&](ID itemId) {
        return (aux.items[itemId].h * (1 - d[itemId]) + aux.items[itemId].w * d[itemId]);
    };
    for (ID i = 0; i < aux.items.size(); ++i) {
        Length itemLen = aux.items[i].w;
        Expr putInBin;
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        putInBin += pi3[i][g][l][m][n];
                        // horizontal fitting.
                        mp.makeConstraint(width(i) - itemLen * (1 - pi3[i][g][l][m][n]) <= w3[g][l][m][n]);
                        mp.makeConstraint(w3[g][l][m][n] <= width(i) + input.param.plateWidth * (1 - pi3[i][g][l][m][n]));
                        // vertical fitting.
                        mp.makeConstraint(height(i) - itemLen * (1 - pi3[i][g][l][m][n]) <= h2[g][l][m] - h4u[g][l][m][n] - h4l[g][l][m][n]);
                        mp.makeConstraint(h2[g][l][m] - h4u[g][l][m][n] - h4l[g][l][m][n] <= height(i) + input.param.plateHeight * (1 - pi3[i][g][l][m][n]));
                    }
                }
            }
        }
        // single placement.
        mp.makeConstraint(maxCoveredArea ? (putInBin <= 1) : (putInBin == 1));
        // covered area.
        coveredArea += (aux.items[i].w * aux.items[i].h * putInBin);
    }

    // total width and height.
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
                mp.makeConstraint(l3BinWidthSum == w1[g][l]);
            }
            // L2 total height.
            mp.makeConstraint(l2BinHeightSum == input.param.plateHeight);
        }
        // L1 total width.
        mp.makeConstraint(input.param.plateWidth * (1 - e[g]) <= l1BinWidthSum);
        mp.makeConstraint(l1BinWidthSum <= input.param.plateWidth - input.param.minWasteWidth * e[g]);

        // residual position.
        if (g + 1 < maxBinNum[L0]) {
            mp.makeConstraint(l1BinWidthSum - input.param.plateWidth * (1 - p[g] + p[g + 1]) <= r);
            mp.makeConstraint(r <= l1BinWidthSum + input.param.plateWidth * (1 - p[g] + p[g + 1]));
        } else {
            mp.makeConstraint(l1BinWidthSum - input.param.plateWidth * (1 - p[g]) <= r);
            mp.makeConstraint(r <= l1BinWidthSum + input.param.plateWidth * (1 - p[g]));
        }
    }

    // width and height bound.
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        for (ID l = 0; l < maxBinNum[L1]; ++l) {
            // L1 width bound.
            mp.makeConstraint(input.param.minL1Width * p1[g][l] <= w1[g][l]);
            mp.makeConstraint(w1[g][l] <= input.param.maxL1Width);
            for (ID m = 0; m < maxBinNum[L2]; ++m) {
                // L2 min height.
                mp.makeConstraint(input.param.minL2Height * p2[g][l][m] <= h2[g][l][m]);
                mp.makeConstraint(input.param.minWasteHeight * t2[g][l][m] - input.param.plateHeight * p2[g][l][m] <= h2[g][l][m]);
                mp.makeConstraint(h2[g][l][m] <= input.param.plateHeight * t2[g][l][m] + input.param.plateHeight * p2[g][l][m]);
                for (ID n = 0; n < maxBinNum[L3]; ++n) {
                    // L3 min width.
                    mp.makeConstraint(input.param.minWasteWidth * t3[g][l][m][n] - input.param.plateWidth * p3[g][l][m][n] <= w3[g][l][m][n]);
                    mp.makeConstraint(w3[g][l][m][n] <= input.param.plateWidth * t3[g][l][m][n] + input.param.plateWidth * p3[g][l][m][n]);
                    // L4 min height.
                    mp.makeConstraint(input.param.minWasteHeight * t4u[g][l][m][n] <= h4u[g][l][m][n]);
                    mp.makeConstraint(h4u[g][l][m][n] <= input.param.plateHeight * t4u[g][l][m][n]);
                    mp.makeConstraint(input.param.minWasteHeight * t4l[g][l][m][n] <= h4l[g][l][m][n]);
                    mp.makeConstraint(h4l[g][l][m][n] <= input.param.plateHeight * t4l[g][l][m][n]);
                    // max waste.
                    mp.makeConstraint(t4u[g][l][m][n] + t4l[g][l][m][n] <= 1);
                }
            }
        }
    }

    if (flawless) { // put larger bins to the left or bottom.
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                if (l + 1 < maxBinNum[L1]) { mp.makeConstraint(w1[g][l] >= w1[g][l + 1]); }
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    if (m + 1 < maxBinNum[L2]) { mp.makeConstraint(h2[g][l][m] >= h2[g][l][m + 1]); }
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        if (n + 1 < maxBinNum[L3]) { mp.makeConstraint(w3[g][l][m][n] >= w3[g][l][m][n + 1]); }
                    }
                }
            }
        }
    } else { // put all non-trivial bins to the left or bottom.
        for (ID g = 0; g < maxBinNum[L0]; ++g) {
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                if (l + 1 < maxBinNum[L1]) { mp.makeConstraint(input.param.plateWidth * w1[g][l] >= w1[g][l + 1]); }
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    if (m + 1 < maxBinNum[L2]) { mp.makeConstraint(input.param.plateHeight * h2[g][l][m] >= h2[g][l][m + 1]); }
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        if (n + 1 < maxBinNum[L3]) { mp.makeConstraint(input.param.plateWidth * w3[g][l][m][n] >= w3[g][l][m][n + 1]); }
                    }
                }
            }
        }
    }

    // user cut for glass order.
    for (ID g = 1; g < maxBinNum[L0]; ++g) { mp.makeConstraint(p[g - 1] >= p[g]); }

    // user cut for covered area should be less than the plate.
    for (ID g = 0; g < maxBinNum[L0]; ++g) {
        Expr area;
        for (ID i = 0; i < aux.items.size(); ++i) {
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
        mp.makeConstraint(area <= input.param.plateWidth * input.param.plateHeight);
    }

    Log(LogSwitch::Szx::Model) << "add objectives." << endl;
    // maximize the area of placed items.
    mp.addObjective(coveredArea, MpSolver::OptimaOrientation::Maximize, 0, 0, 0, 3000);
    // minimize the width of used plates.
    mp.addObjective(coveredWidth, MpSolver::OptimaOrientation::Minimize, 1, 0, 0, 0);

    // solve.
    if (mp.optimize()) {
        // statistics.
        ID usedPlateNum = 0;
        for (; (usedPlateNum < maxBinNum[L0]) && mp.isTrue(p[usedPlateNum]); ++usedPlateNum) {}
        ID placedItemNum = 0;
        for (ID i = 0; i < aux.items.size(); ++i) {
            for (ID g = 0; g < maxBinNum[L0]; ++g) {
                for (ID l = 0; l < maxBinNum[L1]; ++l) {
                    for (ID m = 0; m < maxBinNum[L2]; ++m) {
                        for (ID n = 0; n < maxBinNum[L3]; ++n) {
                            if (mp.isTrue(pi3[i][g][l][m][n])) { ++placedItemNum; }
                        }
                    }
                }
            }
        }
        Log(LogSwitch::Szx::Postprocess) << usedPlateNum << " plates are used to place " << placedItemNum << " items." << endl;

        // visualization.
        constexpr double PlateGap = 100;
        const char FontColor[] = "000000";
        const char FillColor[] = "CCFFCC";
        Drawer draw;
        draw.begin("visc.html", input.param.plateWidth, input.param.plateHeight, usedPlateNum, PlateGap);

        double offset = 0;
        for (ID g = 0; g < maxBinNum[L0]; ++g, offset += (input.param.plateHeight + PlateGap)) {
            draw.rect(0, offset, input.param.plateWidth, input.param.plateHeight);
            double x1 = 0;
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                double y2 = offset;
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    double x3 = x1;
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        for (ID i = 0; i < aux.items.size(); ++i) {
                            if (!mp.isTrue(pi3[i][g][l][m][n])) { continue; }
                            draw.rect(x3, y2, aux.items[i].w, aux.items[i].h, mp.isTrue(d[i]), to_string(i), FontColor, FillColor);
                        }
                        x3 += mp.getValue(w3[g][l][m][n]);
                    }
                    y2 += mp.getValue(h2[g][l][m]);
                }
                x1 += mp.getValue(w1[g][l]);
            }
            x1 = 0;
            for (ID l = 0; l < maxBinNum[L1]; ++l) {
                if (mp.getValue(w1[g][l]) == 0) { continue; }
                double oldX1 = x1;
                x1 += mp.getValue(w1[g][l]);
                double y2 = offset;
                for (ID m = 0; m < maxBinNum[L2]; ++m) {
                    if (mp.getValue(h2[g][l][m]) == 0) { continue; }
                    double oldY2 = y2;
                    y2 += mp.getValue(h2[g][l][m]);
                    double x3 = oldX1;
                    for (ID n = 0; n < maxBinNum[L3]; ++n) {
                        if (mp.getValue(w3[g][l][m][n]) == 0) { continue; }
                        x3 += mp.getValue(w3[g][l][m][n]);
                        draw.line(x3, oldY2, x3, y2, L3);
                    }
                    draw.line(oldX1, y2, x1, y2, L2);
                }
                draw.line(x1, offset, x1, offset + input.param.plateHeight, L1);
            }
        }

        draw.end();
    } else {
        //mp.computeIIS("iis.ilp");
    }
}
#pragma endregion Solver

}
