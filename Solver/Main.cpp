#include "Solver/algorithm/header/Solver.h"

int main(int argc, char *argv[]) {
    if (argc == 3) {
        szx::Problem::Output output;
        output.load(argv[1]);
        output.saveCutOrder(argv[2]);
        return 0;
    }
    return szx::Cli::run(argc, argv);
}
