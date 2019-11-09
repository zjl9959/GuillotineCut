#pragma once
#ifndef GUILLOTINE_CUT_TEST_H

#include "../data/Auxiliary.h"
#include "../data/Placement.h"
#include "../data/Configuration.h"

namespace unit_test {

szx::Solution test_CutSearch(szx::Auxiliary &aux);
szx::Solution test_PlateSearch(szx::Timer &timer, szx::Random &rand, szx::Configuration &cfg, szx::Auxiliary &aux);
bool test_PFSTree();

}


#endif // !GUILLOTINE_CUT_TEST_H
