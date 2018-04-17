////////////////////////////////
/// usage : 1.	interface for mathematical programming solvers.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_GATE_REASSIGNMENT_MP_SOLVER_BASE_H
#define SMART_SZX_GATE_REASSIGNMENT_MP_SOLVER_BASE_H


#include "Config.h"

#include <exception>


namespace szx {

using MpException = std::exception;

class MpSolverBase {
public:
    enum InternalSolver {
        ScipMip = 0,
        CbcMip = 0,
        GurobiMip = 1,
        ClpLp = 0,
        GlopLp = 0,
        GurobiLp = 1,
    };


    static bool isTrue(double value) { return (value > 0.5); }
};

}


#endif // SMART_SZX_GATE_REASSIGNMENT_MP_SOLVER_BASE_H
