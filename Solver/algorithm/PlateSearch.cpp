#include "PlateSearch.h"
#include "../utility/ThreadPool.h"

using namespace std;

namespace szx{

Score PlateSearch::run(const Batch &batch, Solution &sol, bool fast_opt = false) {
    // [zjl][TODO]: add code.
    return Score();
}

Score PlateSearch::beam_search(const Batch &source_batch, Solution &sol) {
    // [zjl][TODO]: add code.
}

ScorePair PlateSearch::branch(Coord start_pos, const Batch &batch, List<Solution>& sols) {
    // [zjl][TODO]: add code.
}

Area PlateSearch::evaluate(const Batch &batch, Solution &sol) {
    // [zjl][TODO]: add code.
}

}