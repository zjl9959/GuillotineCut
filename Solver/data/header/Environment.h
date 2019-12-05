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

    bool pick_item = true;      // �Ƿ���ѡ��Ʒ
    MODE top_mode = TBEAM;      // topSearch�Ż�ģʽ
    MODE plate_mode = PBEAM;    // plateSearch�Ż�ģʽ
    MODE cut_mode = CBEAM;      // cutSearch�Ż�ģʽ
    size_t mtbn = 64;           // TopSearch����֧��Ŀ
    size_t mpbn = 64;           // PlateSearch����֧��Ŀ
    size_t mppn = 16;           // PlateSearchΪÿ��1-cut�����ѡ��Ʒ��Ŀ
    size_t mcbn = 30;           // CutSearch::beam_search����֧��Ŀ
    size_t mcit = 1000000;          // CutSearch::pfs����������Ŀ 

    Configuration() {}
    Configuration(const String &path) { load(path); }

    String toBriefStr() const {
        std::ostringstream os;
        if (!pick_item)os << "nopick;";
        os << "mtbn=" << mtbn
            << ";mpbn=" << mpbn;
        if (pick_item)os << ";mppn=" << mppn;
        if (cut_mode == CBEAM)
            os << ";mcbn=" << mcbn;
        else if (cut_mode == CPFS)
            os << ";mcit=" << mcit;
        os << ";mode=";
        // ������ԡ�
        if (top_mode == TBEAM)
            os << "beam";
        else if (top_mode == TLOCAL)
            os << "local";
        // ԭ�ϲ���ԡ�
        if (plate_mode == PBEAM)
            os << "+beam";
        else if (plate_mode == P0)
            os << "+0";
        // 1-cut����ԡ�
        if (cut_mode == CBEAM)
            os << "+beam";
        else if (cut_mode == CDFS)
            os << "+dfs";
        else if (cut_mode == CPFS)
            os << "+pfs";
        else if (cut_mode == CASTAR)
            os << "+A*";
        return os.str();
    }

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
