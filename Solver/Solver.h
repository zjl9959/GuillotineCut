////////////////////////////////
/// usage : 1.	the entry to the algorithm for the guillotine cut problem.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_SOLVER_H
#define SMART_SZX_GUILLOTINE_CUT_SOLVER_H


#include "Config.h"

#include <chrono>
#include <functional>

#include "Common.h"
#include "Utility.h"
#include "LogSwitch.h"
#include "Problem.h"


namespace szx {

class Solver {
    #pragma region Type
public:
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

        static String AuthorName() { return ""; } // TODO[szx][0]: fill the author name!
        static String HelpInfo() {
            return "Pattern (args can be in any order):\n"
                "  exe (-p path) (-o path) [-s int] [-t seconds] [-name]\n"
                "      [-iter int] [-j int] [-id string] [-h]\n"
                "      [-env path] [-cfg path] [-log path]\n"
                "Switches:\n"
                "  -name  return the identifier of the authors.\n"
                "  -h     print help information.\n"
                "Options:\n"
                "  -p     input instance file path.\n"
                "  -o     output solution file path.\n"
                "  -s     rand seed for the solver.\n"
                "  -t     max running time of the solver.\n"
                "  -i     max iteration of the solver.\n"
                "  -j     max number of working solvers at the same time.\n"
                "  -rid   distinguish different runs in log file and output.\n"
                "  -env   environment file path.\n"
                "  -cfg   configuration file path.\n"
                "  -log   activate logging and specify log file path.\n"
                "Note:\n"
                "  0. in pattern, () is non-optional group, [] is optional group\n"
                "     when -env option is not given.\n"
                "  1. an environment file contains information of all options.\n"
                "     explicit options get higher priority than this.\n"
                "  2. reaching either timeout or iter will stop the solver.\n"
                "     if you specify neither of them, the solver will be running\n"
                "     for a long time. so you should set at least one of them.\n"
                "  3. the solver will still try to generate an initial solution\n"
                "     even if the timeout or max iteration is 0. but the solution\n"
                "     is not guaranteed to be feasible.\n";
        }

        // a dummy main function.
        static int run(int argc, char *argv[]);
    };

    // controls the I/O data format, exported contents and general usage of the solver.
    struct Configuration {
        struct CompleteModel {
            bool maxCoveredArea = false;

            bool addBinSizeOrderCut = false;
            bool addGlassOrderCut = true;
            bool addPlacementOrderCut = false;
            bool addCoveredAreaOnEachPlateCut = true;
            bool addTotalCoveredAreaCut = true;
        };

        Configuration() {}

        String toBriefStr() const { return ""; }

        void load(const String &filePath);
        void save(const String &filePath) const;

        CompleteModel cm;
        // OPTIMIZE[szx][3]: add a list to specify a series of algorithm to be used by each threads in sequence.
    };

    // describe the requirements to the input and output data interface.
    struct Environment {
        static constexpr int DefaultTimeout = (1 << 30);
        static constexpr int DefaultMaxIter = (1 << 30);
        static constexpr int DefaultJobNum = 0;
        // preserved time for IO in the total given time.
        static constexpr int SaveSolutionTimeInMillisecond = 1000;

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
        void load(const String &filePath);
        void loadWithoutCalibrate(const String &filePath);
        void save(const String &filePath) const;

        void calibrate(); // adjust job number and timeout to fit the platform.

        String batchPath() const { return instName + BatchSuffix(); }
        String defectsPath() const { return instName + DefectsSuffix(); }
        String solutionPath() const { return slnPath; }
        String solutionPathWithSuffix() const { return slnPath + SolutionSuffix(); }
        String solutionPathWithTime() const { return slnPath + "." + localTime + ".csv"; }

        String visualizPath() const { return DefaultVisualizationDir() + instName + "." + localTime; }
        String friendlyLocalTime() const {
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

    struct Rect {
        Rect() {}
        Rect(Length width, Length height) : w(width), h(height) {}

        Length w; // width.
        Length h; // height.
    };
    struct RectArea : public Rect { // a rectangular area on the plate.
        RectArea() {}
        RectArea(Coord left, Coord bottom, Length width, Length height) : Rect(width, height), x(left), y(bottom) {}

        Coord x; // left.
        Coord y; // bottom.
    };

    struct Bin {
        Bin() {}
        Bin(const RectArea &rectArea, Problem::Output::Node::Type nodeType = Problem::Output::Node::SpecialType::Waste)
            : rect(rectArea), type(nodeType) {}

        Problem::Output::Node::Type type;
        RectArea rect;
        List<Bin> children;
    };

    struct Solution { // cutting patterns.
        using OnNode = std::function<ID(Bin &bin, ID depth, ID parent)>; // return the node id.

        Solution() : totalWidth(0) {}
        operator Problem::Output();

        void traverse(Bin &bin, ID depth, ID parent, OnNode onNode);

        Length totalWidth; // objective value.
        List<Bin> bins; // solution vector.
    };
    #pragma endregion Type

    #pragma region Constant
public:
    #pragma endregion Constant

    #pragma region Constructor
public:
    Solver(const Problem::Input &inputData, const Environment &environment, const Configuration &config)
        : input(inputData), env(environment), cfg(config), rand(environment.randSeed),
        timer(std::chrono::milliseconds(environment.msTimeout)) {}
    #pragma endregion Constructor

    #pragma region Method
public:
    void solve(); // solve by multiple workers together.
    bool check() const { return (checkFeasibility() && checkObjective()); }
    void record() const; // save running log.

    bool checkFeasibility() const;
    Length checkObjective() const;

protected:
    void init();
    void optimize(Solution &sln, ID workerId = 0); // optimize by a single worker.

    void optimizeCompleteModel(Solution &sln, Configuration::CompleteModel cfg); // make a copy intentionally for the convenience of multi-threading.
    #pragma endregion Method

    #pragma region Field
public:
    Problem::Input input;
    Problem::Output output;

    struct {
        List<Rect> items;
        List<RectArea> defects;

        List<List<ID>> stacks; // stacks[s][i] is the itemId of the i_th item in the stack s.
        List<List<ID>> plates; // plates[p][i] is the defectId of the i_th defect on plate p.
    } aux;

    struct {
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxItemNum> item;
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxStackNum> stack;
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxDefectNum> defect;
        ZeroBasedConsecutiveIdMap<ID, ID, Problem::MaxPlateNum> plate;
    } idMap;

    Environment env;
    Configuration cfg;

    Random rand; // all random number in Solver must be generated by this.
    Timer timer; // the solve() should return before it is timeout.
    #pragma endregion Field
}; // Solver 

}


#endif // SMART_SZX_GUILLOTINE_CUT_SOLVER_H
