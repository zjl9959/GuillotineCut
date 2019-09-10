#include "factory.h"

#include "../utility/LogSwitch.h"

using namespace std;

namespace szx {

TCoord get_next_1cut(int index, const Solution &sol);
TCoord get_next_2cut(int index, const Solution &sol);

/* ��Input�е�����ת����Auxiliary�е����� */
Auxiliary createAuxiliary(const Problem::Input & input) {
    constexpr TID InvalidItemId = Problem::InvalidItemId;
    Auxiliary aux;
    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());
    aux.idMap = IdMap();
    // ��������Ʒ��ÿһ��ջ�����������ջ��
    for (auto i = input.batch.begin(); i != input.batch.end(); ++i) {
        TID itemId = aux.idMap.item.toConsecutiveId(i->id);
        aux.items.push_back(Rect(max(i->width, i->height), min(i->width, i->height)));
        if (itemId != i->id) {
            Log(LogSwitch::Szx::Preprocess) << "map item " << i->id << " to " << itemId << endl;
        }
        TID stackId = aux.idMap.stack.toConsecutiveId(i->stack);
        if (aux.stacks.size() <= stackId) { aux.stacks.push_back(List<TID>()); }
        List<TID> &stack(aux.stacks[stackId]);
        if (stack.size() <= i->seq) { stack.resize(i->seq + 1, InvalidItemId); }
        stack[i->seq] = itemId;
    }
    // ���ջ����Ч��Ʒ
    for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
    }
    aux.plate_defect_x.resize(Problem::MaxPlateNum);
    aux.plate_defect_y.resize(Problem::MaxPlateNum);
    for (TID p = 0; p < Problem::MaxPlateNum; ++p) { aux.idMap.plate.toConsecutiveId(p); }
    // ��覴�ӳ�䵽ÿ��ԭ����
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        TID defectId = aux.idMap.defect.toConsecutiveId(d->id);
        TID plateId = aux.idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plate_defect_x.size() <= plateId) { aux.plate_defect_x.resize(plateId + 1); }
        aux.plate_defect_x[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
        if (aux.plate_defect_y.size() <= plateId) { aux.plate_defect_y.resize(plateId + 1); }
        aux.plate_defect_y[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
    }
    aux.item_area.reserve(aux.items.size());
    // ����ÿ����Ʒ���
    for (int i = 0; i < aux.items.size(); ++i) {
        aux.item_area.push_back(aux.items[i].h*aux.items[i].w);
    }
    // ��ת˳�򣬱�����ջ�ͳ�ջ
    for (int i = 0; i < aux.stacks.size(); i++)
        reverse(aux.stacks[i].begin(), aux.stacks[i].end());
    // ��覴ð���x��������
    for (int p = 0; p < aux.plate_defect_x.size(); ++p)
        sort(aux.plate_defect_x[p].begin(), aux.plate_defect_x[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.x < rhs.x; });
    // ��覴ð���y��������
    for (int p = 0; p < aux.plate_defect_y.size(); ++p)
        sort(aux.plate_defect_y[p].begin(), aux.plate_defect_y[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.y < rhs.y; });
    aux.param = input.param;
    aux.support_thread = thread::hardware_concurrency();
    return aux;
}

/* ��Solution��ʽ�Ľ�ת����Problem::Output��ʽ�Ľ� */
Problem::Output createOutput(const Solution & sol, const Auxiliary &aux) {
/*      _________________________________
       |_________________|<-waste_2    | |
 4-cut_|______|<-waste_4 |             | |
       |      |          |             | |
 2-cut_|______|__________|_____________| |<-waste_1
       |               | |             | |
       |               | |<-waste_3    | |
       |_______________|_|_____________|_|
                       ^               ^
                      3-cut           1-cut
*/
    Problem::Output output;
    if (sol.size() == 0)return output;
    using SpecialType = Problem::Output::Node::SpecialType;
    const Length PW = aux.param.plateWidth, PH = aux.param.plateHeight;
    ID cur_plate = -1;
    // parent_L0/1/2/3����ǰ������ڵ�1/2/3��Ͱ�Ľڵ�id
    ID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    Coord c1cpl = 0, c1cpr = 0, c2cpb = 0, c2cpu = 0, c3cp = 0;
    // ģ����Ʒ�����и����
    for (int n = 0; n < sol.size(); ++n) {
        if (sol[n].getFlagBit(Placement::NEW_PLATE)) { // ʹ��һ���µ�ԭ��
            if (c3cp < c1cpr) { // + waste_3
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (cur_plate != -1 && c2cpu < aux.param.plateHeight) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            if (cur_plate != -1 && c1cpr < aux.param.plateWidth) { // + waste_1
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Waste, 1, parent_L0));
            }
            // ���µ�ǰ����cut��λ����Ϣ
            cur_plate += 1;
            c1cpl = 0;
            c1cpr = get_next_1cut(n, sol);
            c2cpb = 0;
            c2cpu = get_next_2cut(n, sol);
            c3cp = 0;
            // ��ӽڵ�
            parent_L0 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + ԭ�Ͻڵ�
                cur_plate, nodeId++, 0, 0, PW, PH, SpecialType::Branch, 0, -1));
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + һ��Ͱ�ڵ�
                cur_plate, nodeId++, 0, 0, c1cpr, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + ����Ͱ�ڵ�
                cur_plate, nodeId++, 0, 0, c1cpr, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (sol[n].getFlagBit(Placement::NEW_L1)) { // ʹ���µ�һ��Ͱ
            if (c3cp < c1cpr) { // + waste_3
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            if (c2cpu < aux.param.plateHeight) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
            }
            c1cpl = c1cpr;
            c1cpr = get_next_1cut(n, sol);
            c2cpb = 0;
            c2cpu = get_next_2cut(n, sol);
            c3cp = c1cpl;
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + һ��Ͱ
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + ����Ͱ
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (sol[n].getFlagBit(Placement::NEW_L2)) {  // ʹ���µĶ���Ͱ
            if (c3cp < c1cpr) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            c2cpb = c2cpu;
            c2cpu = get_next_2cut(n, sol);
            c3cp = c1cpl;
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + ����Ͱ
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        // ��ȡ��Ʒ���
        const Length item_w = sol[n].getFlagBit(Placement::ROTATE) ?
            aux.items[sol[n].item].h : aux.items[sol[n].item].w;
        const Length item_h = sol[n].getFlagBit(Placement::ROTATE) ?
            aux.items[sol[n].item].w : aux.items[sol[n].item].h;
        /*  ________________
           |   | |<-waste_5 |
           |   | |          |
           |   |d|  item    |
           |___|_|__________|
        */// �����Ʒƽ�Ƶ�覴��Ҳ�������waste
        if (sol[n].getFlagBit(Placement::DEFECT_R)) {
            output.nodes.push_back(Problem::Output::Node( // + waste
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // �������Ʒ�����Ĳ�Ͱ��ֱ�Ӱ����ŵ���һ��3��Ͱ�У������½�һ��3��Ͱ
        if (sol[n].getFlagBit(Placement::BIN4)) {
            output.nodes.push_back(Problem::Output::Node( // + ��Ʒ
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
            continue;
        } else {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + ����Ͱ
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        /*  ___________________
           |        |          |
           |  item  |          |
           |________|          |
           |   d    |<-waste_6 |
           |________|__________|
        */// �����Ʒƽ�Ƶ�覴��ϲ���������waste
        if (sol[n].getFlagBit(Placement::DEFECT_U)) {
            output.nodes.push_back(Problem::Output::Node( // + waste
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
            output.nodes.push_back(Problem::Output::Node( // + ��Ʒ
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
        } else {
            output.nodes.push_back(Problem::Output::Node( // + ��Ʒ
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, item_h, sol[n].item, 4, parent_L3));
            if (c2cpu > sol[n].c4cp && n + 1 < sol.size() && !sol[n + 1].getFlagBit(Placement::BIN4)) {
                output.nodes.push_back(Problem::Output::Node( // + waste_4
                    cur_plate, nodeId++, sol[n].c3cp - item_w, sol[n].c4cp, item_w, c2cpu - sol[n].c4cp, SpecialType::Waste, 4, parent_L3));
            }
        }
        c3cp = sol[n].c3cp;
    }
    const Placement last_item = sol.back();
    const Length last_item_w = last_item.getFlagBit(Placement::ROTATE) ?
        aux.items[last_item.item].h : aux.items[last_item.item].w;
    if (last_item.c4cp < c2cpu && !last_item.getFlagBit(Placement::DEFECT_U)) {
        output.nodes.push_back(Problem::Output::Node( // + waste_4
            cur_plate, nodeId++, last_item.c3cp - last_item_w,
            last_item.c4cp, last_item_w, c2cpu - last_item.c4cp, SpecialType::Waste, 4, parent_L3));
    }
    if (c3cp < c1cpr) {
        output.nodes.push_back(Problem::Output::Node( // + ���һ��waste_3
            cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
    }
    if (c2cpu < aux.param.plateHeight) { // + ���һ��waste_2
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
    }
    if (c1cpr < aux.param.plateWidth) { // + ����
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Residual, 1, parent_L0));
    }
    // ������ʹ��ԭ�ϵĿ��
    output.totalWidth = 0;
    for (int i = 0; i < sol.size(); ++i) {
        if (output.totalWidth < cur_plate*aux.param.plateWidth + sol[i].c1cpr) {
            output.totalWidth = cur_plate * aux.param.plateWidth + sol[i].c1cpr;
        }
    }
    return output;
}

/* ��sol[index]��ʼ����ȡ��һ��1-cut��λ�� */
TCoord get_next_1cut(int index, const Solution &sol) {
    TCoord res = sol[index].c1cpr;
    ++index;
    while (index < sol.size()) {
        if (sol[index].getFlagBit(Placement::NEW_L1))
            break;
        if (res < sol[index].c1cpr)
            res = sol[index].c1cpr;
        ++index;
    }
    if (index == sol.size())
        return sol.back().c1cpr;
    return res;
}

/* ��sol[index]��ʼ����ȡ��һ��2-cut��λ�� */
TCoord get_next_2cut(int index, const Solution &sol) {
    TCoord res = sol[index].c2cpu;
    ++index;
    while (index < sol.size()) {
        if (sol[index].getFlagBit(Placement::NEW_L2))
            break;
        if (res < sol[index].c2cpu)
            res = sol[index].c2cpu;
        ++index;
    }
    if (index == sol.size())
        return sol.back().c2cpu;
    return res;
}

}
