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
        struct Item {
            ID id;
            Length width;
            Length height;
            ID stack;
            ID seq;
        };

        struct Defect {
            ID id;
            ID plateId;
            Coord x;
            Coord y;
            Length width;
            Length height;
        };

        enum BatchColumn { ItemId, ItemWidth, ItemHeight, Stack, Seq };
        enum DefectsColumn { DefectId, PlateId, DefectX, DefectY, DefectWidth, DefectHeight };

        bool loadBatch(const String &batchFilePath);
        bool loadDefects(const String &defectsFilePath);
        bool load(const String &batchFilePath, const String &defectsFilePath) {
            return (loadBatch(batchFilePath) && loadDefects(defectsFilePath));
        }

        List<Item> batch;
        List<Defect> defects;
    };

    struct Output {
        struct Node {
            enum Type { Residual = -3, Branch = -2, Waste = -1, ItemBegin = 0 };

            ID plateId;
            ID id;
            Coord x;
            Coord y;
            Length width;
            Length height;
            int type; // Item.id or Type.
            int cut; // 0 for plate. 1, 2, 3, 4 for real cuts.
            ID parent;
        };

        enum SlnColumn { PlateId, NodeId, NodeX, NodeY, NodeWidth, NodeHeight, NodeType, CutNum, Parent };

        void load(const String &filePath);
        void save(const String &filePath) const;

        Length totalWidth; // the objective value.
        // OPTIMIZE[szx][0]: optimize the solution representation!
        List<Node> nodes; // the solution vector.
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
