#pragma once
#ifndef SMART_ZJL_GUILLOTINE_CUT_SOLVER_H
#define SMART_ZJL_GUILLOTINE_CUT_SOLVER_H

#include "Config.h"

#include <sstream>

#include "Common.h"
#include "Utility.h"
#include "Problem.h"
#include "Algorithm.h"

namespace szx {

// commmand line interface.
struct Cli {
    static constexpr int MaxArgLen = 256;
    static constexpr int MaxArgNum = 32;

    static String InstancePathOption() { return "-p"; }
    static String SolutionPathOption() { return "-o"; }
    static String RandSeedOption() { return "-s"; }
    static String TimeoutOption() { return "-t"; }
    static String MaxIterOption() { return "-i"; }
    static String JobNumOption() { return "-j"; }
    static String RunIdOption() { return "-rid"; }
    static String EnvironmentPathOption() { return "-env"; }
    static String ConfigPathOption() { return "-cfg"; }
    static String LogPathOption() { return "-log"; }

    static String AuthorNameSwitch() { return "-name"; }
    static String HelpSwitch() { return "-h"; }

    static String AuthorName() { return "J29"; }

    // a dummy main function.
    static int run(int argc, char *argv[]);
};

// describe the requirements to the input and output data interface.
struct Environment {
    static constexpr int DefaultTimeout = (1 << 30);
    static constexpr int DefaultMaxIter = (1 << 30);
    static constexpr int DefaultJobNum = 0;
    // preserved time for IO in the total given time.
    static constexpr int SaveSolutionTimeInMillisecond = 1000;

    static constexpr Duration RapidModeTimeoutThreshold = 600 * static_cast<Duration>(Timer::MillisecondsPerSecond);

    static String BatchSuffix() { return "_batch.csv"; }
    static String DefectsSuffix() { return "_defects.csv"; }
    static String SolutionSuffix() { return "_solution.csv"; }

    static String DefaultInstanceDir() { return "Instance/"; }
    static String DefaultSolutionDir() { return "Solution/"; }
    static String DefaultVisualizationDir() { return "Visualization/"; }
    static String DefaultEnvPath() { return "env.csv"; }
    static String DefaultCfgPath() { return "cfg.csv"; }
    static String DefaultLogPath() { return "log.csv"; }

    Environment(const String &instanceName, const String &solutionPath,
        int randomSeed = Random::generateSeed(), double timeoutInSecond = DefaultTimeout,
        Iteration maxIteration = DefaultMaxIter, int jobNumber = DefaultJobNum, String runId = "",
        const String &cfgFilePath = DefaultCfgPath(), const String &logFilePath = DefaultLogPath())
        : instName(instanceName), slnPath(solutionPath), randSeed(randomSeed),
        msTimeout(static_cast<Duration>(timeoutInSecond * Timer::MillisecondsPerSecond)), maxIter(maxIteration),
        jobNum(jobNumber), rid(runId), cfgPath(cfgFilePath), logPath(logFilePath), localTime(Timer::getTightLocalTime()) {}
    Environment() : Environment("", "") {}

    void load(const Map<String, char*> &optionMap);

    void calibrate(); // adjust job number and timeout to fit the platform.

    String batchPath() const { return instName + BatchSuffix(); }
    String defectsPath() const { return instName + DefectsSuffix(); }
    String solutionPath() const { return slnPath; }
    String solutionPathWithTime() const { return slnPath + "." + localTime + ".csv"; }

    String visualizPath() const { return DefaultVisualizationDir() + friendlyInstName() + "." + localTime + ".html"; }
    template<typename T>
    String visualizPath(const T &msg) const { return DefaultVisualizationDir() + friendlyInstName() + "." + localTime + "." + std::to_string(msg) + ".html"; }
    String friendlyInstName() const { // friendly to file system (without special char).
        auto pos = instName.find_last_of('/');
        return (pos == String::npos) ? instName : instName.substr(pos + 1);
    }
    String friendlyLocalTime() const { // friendly to human.
        return localTime.substr(0, 4) + "-" + localTime.substr(4, 2) + "-" + localTime.substr(6, 2)
            + "_" + localTime.substr(8, 2) + ":" + localTime.substr(10, 2) + ":" + localTime.substr(12, 2);
    }

    // essential information.
    String instName;
    String slnPath;
    int randSeed;
    Duration msTimeout;

    // optional information. highly recommended to set in benchmark.
    Iteration maxIter;
    int jobNum; // number of solvers working at the same time.
    String rid; // the id of each run.
    String cfgPath;
    String logPath;

    // auto-generated data.
    String localTime;
};

struct Configuration {
    int mcin; // maximum choose item number.
    int mbpn; // maximum branch plate number.
    int mhcn; // maximum hopeful 1-cut number.
    String toBriefStr() const {
        std::ostringstream os;
		os << "GB2"
			<< ";mcin=" << mcin
			<< ";mbpn=" << mbpn
			<< ";mhcn=" << mhcn;
        return os.str();
    }
    Configuration(int MCIN = 8, int MBPN = 4, int MHCN = 1):
        mcin(MCIN), mbpn(MBPN), mhcn(MHCN) {}
};

class Solver {
public:
    Solver(const Problem::Input &inputData, const Environment &environment, const Configuration &config)
        : input(inputData), env(environment), cfg(config), rand(environment.randSeed),
        timer(std::chrono::milliseconds(environment.msTimeout)) {}
    virtual ~Solver() {};
	virtual void solve();
    void record() const;
protected:
    bool check(Length &checkerObj) const;
    void toOutput();
private:
    const TCoord get_next_1cut(int index) const;
    const TCoord get_next_2cut(int index) const;
public:
    Problem::Input input;
    Problem::Output output;
    Environment env;
    Configuration cfg;
protected:
    Random rand;
    Timer timer;
    List<SolutionNode> best_solution;
    Area total_item_area = 0;
};

}

#endif // !SMART_ZJL_GUILLOTINE_CUT_SOLVER_H
