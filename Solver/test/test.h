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
    void test_toOutput();    // ����solver��toOutput����
};

class TestCutSearch : public szx::CutSearch {
public:
    TestCutSearch(const szx::TID plate, const szx::TCoord start_pos) : szx::CutSearch(plate, start_pos) {};
public:
    void test_branch(const ItemNode &old, szx::TID &item);
};

void test_cutSearch_branch();   // ����cutSearch�е�branch����

}

#endif // !GUILLOTINECUT_TEST_H
