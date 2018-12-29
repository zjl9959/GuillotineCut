////////////////////////////////
/// usage : 1.	the entry to the algorithm for the guillotine cut problem.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_SOLVER_H
#define SMART_SZX_GUILLOTINE_CUT_SOLVER_H


#include "Config.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <sstream>
#include <thread>

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

        static String AuthorName() { return "S28"; }
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
        struct IteratedModel {
            String toBriefStr() const {
                std::ostringstream oss;
                oss << "st=" << std::setprecision(2) << minSecTimeoutPerIteration
                    << ";sn=" << std::setprecision(2) << approxPlacedItemNumPerIteration
                    << ";sc=" << maxItemToConsiderPerIteration
                    << ";sr=" << std::setprecision(4) << initCoverageRatio
                    << ";sf=" << setMipFocus
                    << ";sl=" << lookaheads[0] << "*" << lookaheads[1] << "*" << lookaheads[2] << "*" << lookaheads[3]
                    << ";ss=" << steps[0] << "*" << steps[1]
                    << ";or=" << maxCoverRatio
                    << ";ui=" << addTrivialityInheritanceCut
                    << ";uf=" << addDefectFittingCut
                    << ";ub=" << addBinSizeOrderCut
                    << ";ue=" << addEmptyBinMergingCut
                    << ";ua=" << addAreaBoundCut
                    << ";uw=" << addL1BinWidthSumCut;
                return oss.str();
            }


            // strategy.
            double minSecTimeoutPerIteration = 10;
            double approxPlacedItemNumPerIteration = 3;
            ID maxItemToConsiderPerIteration = 80;
            double initCoverageRatio = 0.9;
            bool setMipFocus = true; // prefer true.
            bool placeAllItems = false; // all items must be placed once.

            // step control.
            List<ID> lookaheads = { 1, 1, 6, 5 }; // L0, L1, L2, L3.
            List<ID> steps = { 1, 1 }; // L0, L1.

            // constraint.

            // objective.
            bool maxCoverRatio = true;

            // user cut.
            bool addTrivialityInheritanceCut = false; // prefer false.
            bool addDefectFittingCut = false; // prefer false.
            bool addBinSizeOrderCut = false; // prefer false.
            bool addEmptyBinMergingCut = false; // prefer false.
            bool addAreaBoundCut = false;
            bool addL1BinWidthSumCut = false;
        };


        enum Algorithm { IteratedMp };


        Configuration() {}

        void load(const String &filePath);
        void save(const String &filePath) const;


        String toBriefStr() const {
            String threshold(std::to_string(itemNumThresholdForCompleteModel));
            String threadNum(std::to_string(threadNumPerWorker));
            switch (alg) {
            case Algorithm::IteratedMp:
                return "[I" + threshold + "x" + threadNum + "]" + im.toBriefStr();
            default:
                return "";
            }
        }


        Algorithm alg = Configuration::Algorithm::IteratedMp; // OPTIMIZE[szx][3]: make it a list to specify a series of algorithms to be used by each threads in sequence.
        int threadNumPerWorker = (std::min)(4, static_cast<int>(std::thread::hardware_concurrency()));
        ID itemNumThresholdForCompleteModel = 20;

        IteratedModel im;
    };

    // describe the requirements to the input and output data interface.
    struct Environment {
        static constexpr int DefaultTimeout = (1 << 30);
        static constexpr int DefaultMaxIter = (1 << 30);
        static constexpr int DefaultJobNum = 0;
        // preserved time for IO in the total given time.
        static constexpr int SaveSolutionTimeInMillisecond = 3000;

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
        void load(const String &filePath);
        void loadWithoutCalibrate(const String &filePath);
        void save(const String &filePath) const;

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
        Bin(double left, double bottom, double width, double height, Problem::Output::Node::Type nodeType = Problem::Output::Node::SpecialType::Waste)
            : rect(Math::lfloor(left), Math::lfloor(bottom), Math::lfloor(width), Math::lfloor(height)), type(nodeType) {}

        Problem::Output::Node::Type type;
        RectArea rect;
        List<Bin> children;
    };

    struct Solution { // cutting patterns.
        using OnNode = std::function<ID(Bin &bin, ID depth, ID parent)>; // return the node id.

        Solution(Solver *pSolver = nullptr) : totalWidth(Problem::Output::MaxWidth), solver(pSolver) {}
        operator Problem::Output();

        void traverse(Bin &bin, ID depth, ID parent, OnNode onNode);

        Length totalWidth; // objective value.
        List<Bin> bins; // solution vector.

        Solver *solver;
    };
    #pragma endregion Type

    #pragma region Constant
public:
    #pragma endregion Constant

    #pragma region Constructor
public:
    Solver(const Problem::Input &inputData, const Environment &environment, const Configuration &config)
        : input(inputData), env(environment), cfg(config), rand(environment.randSeed),
        timer(std::chrono::milliseconds(environment.msTimeout)), iteration(1) {}
    #pragma endregion Constructor

    #pragma region Method
public:
    bool solve(); // return true if exit normally. solve by multiple workers together.
    bool check(Length &obj) const;
    void record() const; // save running log.

    Length totalItemArea() const;

protected:
    void init();
    bool optimize(Solution &sln, ID workerId = 0); // optimize by a single worker.

    bool optimizeIteratedModel(Solution &sln, Configuration::IteratedModel cfg, ID workerId = 0); // make a copy of cfg intentionally for the convenience of multi-threading.
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
    Iteration iteration;
    #pragma endregion Field
}; // Solver 

}


#endif // SMART_SZX_GUILLOTINE_CUT_SOLVER_H
