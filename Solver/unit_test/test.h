#pragma once
#ifndef GUILLOTINE_CUT_TEST_H

namespace szx {

class UnitTest {
public:
    void run();
private:
    void test_CutSearch();
    void test_PlateSearch();
    void test_copy_speed_of_placement(int repeat, int nb_element);
private:

};

}


#endif // !GUILLOTINE_CUT_TEST_H
