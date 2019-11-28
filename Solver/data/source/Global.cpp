#include "Solver/data/header/Global.h"

#include "Solver/utility/LogSwitch.h"

using namespace std;

namespace szx {

namespace gv {          // Solver��ȫ�ֱ���
Configuration cfg;                  // ���ò���
Random rand;                        // ������ӷ�����
Timer timer;                        // ��ʱ��
List<Rect> items;                   // ��Ʒ������Ϣ
List<Area> item_area;               // ��Ʒ���
List<List<TID>> stacks;             // ��Ʒ��ÿ��ջ�е�˳��
List<List<Defect>> defect_x;        // ÿ��ԭ���ϵ�覴ã�����x��������
List<List<Defect>> defect_y;        // ÿ��ԭ���ϵ�覴ã�����y��������
Problem::Input::Param param;        // Լ������
IdMap idMap;                        // ����TID������TID֮���ӳ��
int support_thread;                 // ֧���߳���Ŀ
}

/*
* ��ʼ��Solver��ȫ�ֱ�����
*/
void init_global_variables(const Problem::Input &input, const Environment &env) {
    gv::cfg = Configuration();
    // [zjl][TODO]: load configure from file.
    gv::rand = Random(env.randSeed);
    gv::timer = Timer(std::chrono::milliseconds(env.msTimeout));
    constexpr TID InvalidItemId = Problem::InvalidItemId;
    gv::items.clear();
    gv::items.reserve(input.batch.size());
    gv::stacks.clear();
    gv::stacks.reserve(input.batch.size());
    gv::idMap = IdMap();
    // ��������Ʒ��ÿһ��ջ�����������ջ��
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        TID itemId = gv::idMap.item.toConsecutiveId(i->id);
        gv::items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
        if (itemId != i->id) {
            Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
        }
        TID stackId = gv::idMap.stack.toConsecutiveId(i->stack);
        if (gv::stacks.size() <= stackId) { gv::stacks.push_back(List<TID>()); }
        List<TID> &stack(gv::stacks[stackId]);
        if (stack.size() <= i->seq) { stack.resize(i->seq + 1, InvalidItemId); }
        stack[i->seq] = itemId;
    }
    // ���ջ����Ч��Ʒ
    for (auto s = gv::stacks.begin(); s != gv::stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
    }
    gv::defect_x.clear();
    gv::defect_x.resize(Problem::MaxPlateNum);
    gv::defect_y.clear();
    gv::defect_y.resize(Problem::MaxPlateNum);
    for (TID p = 0; p < Problem::MaxPlateNum; ++p) { gv::idMap.plate.toConsecutiveId(p); }
    // ��覴�ӳ�䵽ÿ��ԭ����
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        TID defectId = gv::idMap.defect.toConsecutiveId(d->id);
        TID plateId = gv::idMap.plate.toConsecutiveId(d->plateId);
        if (gv::defect_x.size() <= plateId) { gv::defect_x.resize(plateId + 1); }
        gv::defect_x[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
        if (gv::defect_y.size() <= plateId) { gv::defect_y.resize(plateId + 1); }
        gv::defect_y[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
    }
    gv::item_area.clear();
    gv::item_area.reserve(gv::items.size());
    // ����ÿ����Ʒ���
    for (int i = 0; i < gv::items.size(); ++i) {
        gv::item_area.push_back(gv::items[i].h*gv::items[i].w);
    }
    // ��ת˳�򣬱�����ջ�ͳ�ջ
    for (int i = 0; i < gv::stacks.size(); i++)
        reverse(gv::stacks[i].begin(), gv::stacks[i].end());
    // ��覴ð���x��������
    for (int p = 0; p < gv::defect_x.size(); ++p)
        sort(gv::defect_x[p].begin(), gv::defect_x[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.x < rhs.x; });
    // ��覴ð���y��������
    for (int p = 0; p < gv::defect_y.size(); ++p)
        sort(gv::defect_y[p].begin(), gv::defect_y[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.y < rhs.y; });
    gv::param = input.param;
    gv::support_thread = thread::hardware_concurrency();
}

}
