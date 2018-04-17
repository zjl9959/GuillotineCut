////////////////////////////////
/// usage : 1.	read csv file into an jagged array.
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_CSV_READER_H
#define SMART_SZX_GUILLOTINE_CUT_CSV_READER_H


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <vector>


namespace szx {

class CsvReader {
public:
    using Row = std::vector<char*>;

    const std::vector<Row>& scan(std::ostringstream &oss) {
        oss << '\n'; // add a sentinel so that end of file check is only necessary in onNewLine().
        data = oss.str();
        begin = const_cast<char*>(data.data());  // the const cast can be omitted since C++17.
        end = begin + data.size();

        onNewLine(begin);

        return rows;
    }
    const std::vector<Row>& scan(std::ifstream &ifs) {
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return scan(oss);
    }

protected:
    void onNewLine(char *s) {
        while ((s != end) && (NewLineChars.find(*s) != NewLineChars.end())) { ++s; } // remove empty lines.
        if (s == end) { return; }

        rows.push_back(Row());

        onSpace(s);
    }

    void onSpace(char *s) {
        while (SpaceChars.find(*s) != SpaceChars.end()) { ++s; } // trim spaces.

        onValue(s);
    }

    void onValue(char *s) {
        rows.back().push_back(s);

        char c = *s;
        if (EndCellChars.find(c) == EndCellChars.end()) {
            while (EndCellChars.find(*(++s)) == EndCellChars.end()) {}
            c = *s;

            char *space = s;
            while (SpaceChars.find(*(space - 1)) != SpaceChars.end()) { --space; }
            *space = 0; // trim spaces and remove comma or line ending.
        } else { // empty cell.
            *s = 0;
        }

        (NewLineChars.find(c) != NewLineChars.end()) ? onNewLine(++s) : onSpace(++s);
    }

    // TODO[szx][2]: handle quote (comma will not end cell).
    // EXTEND[szx][5]: make trim space configurable.

    static const std::set<char> NewLineChars;
    static const std::set<char> SpaceChars;
    static const std::set<char> EndCellChars;

    char *begin;
    const char *end;

    std::string data;
    std::vector<Row> rows;
};

}


#endif // SMART_SZX_GUILLOTINE_CUT_CSV_READER_H
