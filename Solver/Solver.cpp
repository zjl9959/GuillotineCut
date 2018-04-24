#include "Solver.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>


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

    Log(LogSwitch::Szx::Cli) << "load instance." << endl;
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
    List<Problem::Output> outputs(env.jobNum);

    Log(LogSwitch::Szx::Framework) << "launch " << env.jobNum << "workers." << endl;
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

void Solver::optimize(Problem::Output &optimum, ID workerId) {
    Log(LogSwitch::Szx::Framework) << "worker " << workerId << " starts." << endl;

    // TODO[szx][0]: solve.
}
#pragma endregion Solver

}
