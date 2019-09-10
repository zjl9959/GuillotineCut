////////////////////////////////
/// usage : 1.	common type aliases.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef GUILLOTINE_CUT_COMMON_H
#define GUILLOTINE_CUT_COMMON_H

#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <string>

namespace szx {

// zero-based consecutive integer identifier.
using ID = int;
// the unit of width and height.
using Length = int;
// the unit of x and y coordinates.
using Coord = Length;
// zero-based consecutive integer identifier for tree search
using TID = short;
// the unit of tree search width and height
using TLength = short;
// the unit of tree search x and y coordinates.
using TCoord = TLength;
// the unit of area size
using Area = int;
// the unit of status or flag
using Status = unsigned short;
// the unit of elapsed computational time.
using Duration = int;
// number of neighborhood moves in local search.
using Iteration = int;
// the depth of tree node (no lager than item numbers[800])
using Depth = short;
// the score of the tree node
using Score = double;

template<typename T>
using List = std::vector<T>;

template<typename T>
using Set = std::set<T>;

template<typename Key, typename Val>
using Map = std::map<Key, Val>;

template<typename Key, typename Val>
using HashMap = std::unordered_map<Key, Val>;

using String = std::string;

using ScorePair = std::pair<int, Score>;

using AreaPair = std::pair<int, Area>;

using LengthPair = std::pair<int, TLength>;

class FileExtension {
public:
    static String protobuf() { return String(".pb"); }
    static String json() { return String(".json"); }
};

}


#endif // GUILLOTINE_CUT_COMMON_H
