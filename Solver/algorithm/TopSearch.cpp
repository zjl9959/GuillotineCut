#include "TopSearch.h"
#include "Picker.h"

namespace szx {

void TopSearch::beam_search() {
    // [zjl][TODO]:add code.
}

Score TopSearch::get_platesol(ID plate_id, const Batch &source_batch, Solution &sol) {
    Picker picker(source_batch, rand_, aux_);
    Batch batch;
    Picker::Terminator terminator(0, aux_.param.plateHeight * aux_.param.plateWidth * 1.2);
    if (picker.rand_pick(batch, terminator)) {
        PlateSearch solver(plate_id, cfg_, rand_, timer_, aux_);
        solver.beam_search(batch);
        return solver.get_bestsol(sol);
    }
    return -2.0;
}

Length TopSearch::greedy_evaluate(const Batch &source_batch, Solution &sol) {
    // [zjl][TODO]:add code.
    return Length();
}

}
