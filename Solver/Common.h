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
#include <sstream>
#include <algorithm>
#include <cassert>

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

template<typename T>
using List = std::vector<T>;

template<typename T>
using Set = std::set<T>;

template<typename Key, typename Val>
using Map = std::map<Key, Val>;

template<typename Key, typename Val>
using HashMap = std::unordered_map<Key, Val>;

using String = std::string;

using AreaPair = std::pair<int, Area>;

using LengthPair = std::pair<int, TLength>;

class FileExtension {
public:
    static String protobuf() { return String(".pb"); }
    static String json() { return String(".json"); }
};

class UsageRate {
    // [TODO]: test this class.
protected:
    static constexpr int base = 1000;    // 将浮点数转化为小数时保留的小数位数。
    static constexpr int invalid_value = INT_MAX;
    int usage_rate_int; // 使用整数来保存浮点数。
public:
    UsageRate() : usage_rate_int(invalid_value) {}
    explicit UsageRate(double rate) : usage_rate_int(static_cast<int>(rate*base)) { assert(rate < 1.0); }
    
    UsageRate& operator= (const UsageRate &other) {
        this->usage_rate_int = other.usage_rate_int;
        return *this;
    }

    bool valid() const { return usage_rate_int != invalid_value; }
    
    virtual bool operator< (const UsageRate &rhs) const {
        return this->usage_rate_int < rhs.usage_rate_int;
    }

    virtual String str() const {
        std::ostringstream os;
        os << usage_rate_int / base << "."
            << usage_rate_int % base << "%";
        return os.str();
    }
};

static constexpr TID INVALID = -1;

}


#endif // GUILLOTINE_CUT_COMMON_H
