#pragma once
#ifndef GUILLOTINE_CUT_SOLVER_H
#define GUILLOTINE_CUT_SOLVER_H

#include "Solver/utility/Common.h"
#include "Solver/data/header/Problem.h"
#include "Solver/data/header/Environment.h"

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

class Solver {
public:
    Solver(const Problem::Input &inputData, const Environment &environment) : input(inputData), env(environment) {}
    void run();
protected:
    bool check(Length &checkerObj) const;
    void record() const;
private:
    Problem::Input input;
    Environment env;
    Problem::Output output;
};

}

#endif // !GUILLOTINE_CUT_SOLVER_H
