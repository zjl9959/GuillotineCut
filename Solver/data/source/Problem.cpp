#include "Solver/data/header/Problem.h"

#include <cstdlib>

#include "Solver/utility/Common.h"
#include "Solver/utility/Utility.h"
#include "Solver/utility/LogSwitch.h"
#include "Solver/utility/CsvReader.h"


using namespace std;


namespace szx {

#pragma region Problem::Input
bool Problem::Input::loadBatch(const String &batchFilePath) {
    ifstream ifs(batchFilePath);
    if (!ifs.is_open()) {
        Log(Log::Fatal) << "fail to open batch file." << endl;
        return false;
    }

    CsvReader cr;
    const vector<CsvReader::Row> &rows(cr.scan(ifs));
    batch.reserve(rows.size() - 2);
    for (auto r = rows.begin() + 1; r != rows.end(); ++r) { // skip header.
        batch.push_back({
            atoi(r->at(BatchColumn::ItemId)),
            atoi(r->at(BatchColumn::ItemWidth)),
            atoi(r->at(BatchColumn::ItemHeight)),
            atoi(r->at(BatchColumn::Stack)),
            atoi(r->at(BatchColumn::Seq))
        });
    }

    return true;
}

bool Problem::Input::loadDefects(const String &defectsFilePath) {
    ifstream ifs(defectsFilePath);
    if (!ifs.is_open()) {
        Log(Log::Fatal) << "fail to open batch file." << endl;
        return false;
    }

    CsvReader cr;
    const vector<CsvReader::Row> &rows(cr.scan(ifs));
    defects.reserve(rows.size() - 2);
    for (auto r = rows.begin() + 1; r != rows.end(); ++r) { // skip header.
        defects.push_back({
            atoi(r->at(DefectsColumn::DefectId)),
            atoi(r->at(DefectsColumn::PlateId)),
            atoi(r->at(DefectsColumn::DefectX)),
            atoi(r->at(DefectsColumn::DefectY)),
            atoi(r->at(DefectsColumn::DefectWidth)),
            atoi(r->at(DefectsColumn::DefectHeight))
        });
    }

    return true;
}
#pragma endregion Problem::Input

#pragma region Problem::Output
void Problem::Output::load(const String &filePath) {
    ifstream ifs(filePath);
    if (!ifs.is_open()) {
        Log(Log::Error) << "fail to open solution file." << endl;
        return;
    }

    CsvReader cr;
    const vector<CsvReader::Row> &rows(cr.scan(ifs));
    nodes.reserve(rows.size() - 2);
    for (auto r = rows.begin() + 1; r != rows.end(); ++r) { // skip header.
        nodes.push_back({
            atoi(r->at(SlnColumn::PlateId)),
            atoi(r->at(SlnColumn::NodeId)),
            atoi(r->at(SlnColumn::NodeX)),
            atoi(r->at(SlnColumn::NodeY)),
            atoi(r->at(SlnColumn::NodeWidth)),
            atoi(r->at(SlnColumn::NodeHeight)),
            atoi(r->at(SlnColumn::NodeType)),
            atoi(r->at(SlnColumn::CutNum)),
            atoi(r->at(SlnColumn::Parent))
        });
        if (strcmp(r->at(SlnColumn::Parent), "") == 0) {
            nodes.back().parent = -1;
        }
    }
}

void Problem::Output::save(const String &filePath) const {
    ofstream ofs(filePath);

    ofs << "PLATE_ID;NODE_ID;X;Y;WIDTH;HEIGHT;TYPE;CUT;PARENT" << endl;
    for (auto n = nodes.begin(); n != nodes.end(); ++n) {
        ofs << n->plateId << CsvReader::CommaChar
            << n->id << CsvReader::CommaChar
            << n->x << CsvReader::CommaChar
            << n->y << CsvReader::CommaChar
            << n->width << CsvReader::CommaChar
            << n->height << CsvReader::CommaChar
            << n->type << CsvReader::CommaChar
            << n->cut << CsvReader::CommaChar;
        if (n->parent >= 0) { ofs << n->parent; }
        ofs << endl;
    }
}

void Problem::Output::saveCutOrder(const String& filePath) {
    ofstream ofs(filePath);
    ofs << "CUT_ID,PLATE_ID,X_BEGIN,Y_BEGIN,X_END,Y_END" << endl;

    // 1. 建树，坐标从小到大排序
    List<ID> roots;
    sort(nodes.begin(), nodes.end(), [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });
    for (auto& node : nodes) {
        if (node.parent < 0) {
            roots.push_back(node.id);
            continue;
        }
        nodes[node.parent].children.push_back(node.id);
    }
    for (auto& node : nodes) {
        sort(node.children.begin(), node.children.end(), [this](auto& lhs, auto& rhs) {
            if (nodes[lhs].x == nodes[rhs].x) { return nodes[lhs].y < nodes[rhs].y; }
            return nodes[lhs].x < nodes[rhs].x;
        });
    }

    // 2. BFS
    using Point = pair<Coord, Coord>;
    using Segment = pair<Point, Point>;
    vector<Segment> segments;
    vector<ID> plates;
    ID cutId = 0;
    queue<ID> q;
    for (ID root : roots) {
        q.push(root);
        Length plateWidth = nodes[root].width, plateHeight = nodes[root].height;
        while (!q.empty()) {
            auto& node = nodes[q.front()];
            q.pop();
            for (ID child : node.children) { q.push(child); }
            if (q.empty() || (!q.empty() && node.parent != nodes[q.front()].parent) ||
                (node.cut % 2 != 0 && node.x + node.width == plateWidth) ||
                (node.cut % 2 == 0 && node.y + node.height == plateHeight)) {
                continue;
            }  //  最后一个孩子 || 与原料边界重合不输出

            //ofs << ++cutId << ',' << node.plateId << ',';
            //if (node.cut % 2) { ofs << node.x + node.width << ',' << node.y << ','; }  // 1-cut/3-cut 纵向切割
            //else { ofs << node.x << ',' << node.y + node.height << ','; }              // 2-cut/4-cut 横向切割
            //ofs << node.x + node.width << ',' << node.y + node.height << endl;

            Point p1 = node.cut % 2 ? make_pair(node.x + node.width, node.y) : make_pair(node.x, node.y + node.height);
            Point p2 = make_pair(node.x + node.width, node.y + node.height);
            bool flag = false;
            for (auto& seg : segments) {
                if (seg.first.first == seg.second.first && p1.first == p2.first ||      // 1-cut/3-cut x坐标相同
                    seg.first.second == seg.second.second && p1.second == p2.second) {  // 2-cut/4-cut y坐标相同
                    if (seg.first == p2) {
                        seg.first = p1;
                        flag = true;
                    }
                    if (seg.second == p1) {
                        seg.second = p2;
                        flag = true;
                    }
                }
                if (flag) { break; }
            }
            if (!flag) {
                segments.push_back(make_pair(p1, p2));
                plates.push_back(node.plateId);
            }
        }
    }

    for (int i = 0; i < segments.size(); ++i) {
        ofs << ++cutId << ',' << plates[i] << ','
            << segments[i].first.first << ',' << segments[i].first.second << ','
            << segments[i].second.first << ',' << segments[i].second.second << endl;
    }
}
#pragma endregion Problem::Output

}
