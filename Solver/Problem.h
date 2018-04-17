////////////////////////////////
/// usage : 1.	data that identifies a guillotine cut problem and its solution.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_PROBLEM_H
#define SMART_SZX_GUILLOTINE_CUT_PROBLEM_H


#include "Config.h"

#include "Common.h"


namespace szx {

class Problem {
    #pragma region Type
public:
    struct Input {
        void load(const String &batchFilePath, const String &defectsFilePath);
    };

    struct Output {
        void load(const String &filePath);
        void save(const String &filePath) const;

        Length totalWidth; // the objective value.
    };
    #pragma endregion Type

    #pragma region Constant
public:
    #pragma endregion Constant

    #pragma region Constructor
public:
    #pragma endregion Constructor

    #pragma region Method
public:
    #pragma endregion Method

    #pragma region Field
public:
    #pragma endregion Field
}; // Problem

}


#endif // SMART_SZX_GUILLOTINE_CUT_PROBLEM_H
