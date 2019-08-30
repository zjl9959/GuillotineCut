#pragma once
#ifndef GUILLOTINECUT_TEST_H
#define GUILLOTINECUT_TEST_H

#include "../Solver.h"

namespace zjl_test {

#pragma region functions

#pragma endregion

class TestSolver : public szx::Solver {
public:
    TestSolver(const szx::Problem::Input &inputData, const szx::Environment &environment, const Configuration &config) :
        szx::Solver(inputData, environment, config) {}
public:
    void TestToOutput();    // ≤‚ ‘solver÷–toOutput∫Ø ˝
};


}

#endif // !GUILLOTINECUT_TEST_H
