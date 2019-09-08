#pragma once
#ifndef GUILLOTINE_CUT_FACTORY_H
#define GUILLOTINE_CUT_FACTORY_H

#include "../data/Auxiliary.h"
#include "../data/Placement.h"

namespace szx {

Auxiliary createAuxiliary(const Problem::Input &input);
Problem::Output createOutput(const Solution &sol, const Auxiliary &aux);

}

#endif // !GUILLOTINE_CUT_FACTORY_H
