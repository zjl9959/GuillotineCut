#pragma once
#ifndef GUILLOTINE_CUT_CONFIGURE_H
#define GUILLOTINE_CUT_CONFIGURE_H

#include <sstream>

#include "Solver/utility/Common.h"
#include "Solver/utility/Utility.h"

namespace szx {

struct Configuration {
    enum MODE {
        TBEAM, TLOCAL,
        PBEAM, P0,
        CBEAM, CDFS, CPFS, CASTAR
    };

    bool pick_item = true;      // 是否挑选物品
    MODE top_mode = TBEAM;      // topSearch优化模式
    MODE plate_mode = PBEAM;    // plateSearch优化模式
    MODE cut_mode = CBEAM;      // cutSearch优化模式
    size_t mtbn = 64;           // TopSearch最大分支数目
    size_t mpbn = 64;           // PlateSearch最大分支数目
    size_t mppn = 16;           // PlateSearch为每个1-cut最大挑选物品数目
    size_t mcbn = 30;           // CutSearch::beam_search最大分支数目
    size_t mcit = 1000000;          // CutSearch::pfs的最大迭代数目 

    Configuration() {}
    Configuration(const String &path) { load(path); }

    String toBriefStr() const;
    void load(const String &path);
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
    static String DefaultCfgPath() { return "cfg.txt"; }
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

}

#endif
