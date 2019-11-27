#pragma once
#ifndef GUILLOTINE_CUT_TEST_H

#include "Solver/data/header/Placement.h"
#include "Solver/data/header/Environment.h"

namespace unit_test {

szx::Solution test_CutSearch();
szx::Solution test_PlateSearch();
void test_copy_speed_of_placement(int repeat, int nb_element);

}


#endif // !GUILLOTINE_CUT_TEST_H
