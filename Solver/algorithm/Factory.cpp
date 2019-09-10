#include "factory.h"

#include "../utility/LogSwitch.h"

using namespace std;

namespace szx {

TCoord get_next_1cut(int index, const Solution &sol);
TCoord get_next_2cut(int index, const Solution &sol);

/* 将Input中的数据转换成Auxiliary中的数据 */
Auxiliary createAuxiliary(const Problem::Input & input) {
    constexpr TID InvalidItemId = Problem::InvalidItemId;
    Auxiliary aux;
    aux.items.reserve(input.batch.size());
    aux.stacks.reserve(input.batch.size());
    aux.idMap = IdMap();
    // 将分配物品到每一个栈，并将其放入栈中
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
    // 清除栈中无效物品
    for (auto s = aux.stacks.begin(); s != aux.stacks.end(); ++s) {
        s->erase(remove(s->begin(), s->end(), InvalidItemId), s->end());
    }
    aux.plate_defect_x.resize(Problem::MaxPlateNum);
    aux.plate_defect_y.resize(Problem::MaxPlateNum);
    for (TID p = 0; p < Problem::MaxPlateNum; ++p) { aux.idMap.plate.toConsecutiveId(p); }
    // 将瑕疵映射到每块原料上
    for (auto d = input.defects.begin(); d != input.defects.end(); ++d) {
        TID defectId = aux.idMap.defect.toConsecutiveId(d->id);
        TID plateId = aux.idMap.plate.toConsecutiveId(d->plateId);
        if (aux.plate_defect_x.size() <= plateId) { aux.plate_defect_x.resize(plateId + 1); }
        aux.plate_defect_x[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
        if (aux.plate_defect_y.size() <= plateId) { aux.plate_defect_y.resize(plateId + 1); }
        aux.plate_defect_y[plateId].push_back(Defect(defectId, d->x, d->y, d->width, d->height));
    }
    aux.item_area.reserve(aux.items.size());
    // 计算每块物品面积
    for (int i = 0; i < aux.items.size(); ++i) {
        aux.item_area.push_back(aux.items[i].h*aux.items[i].w);
    }
    // 翻转顺序，便于入栈和出栈
    for (int i = 0; i < aux.stacks.size(); i++)
        reverse(aux.stacks[i].begin(), aux.stacks[i].end());
    // 将瑕疵按照x坐标排序
    for (int p = 0; p < aux.plate_defect_x.size(); ++p)
        sort(aux.plate_defect_x[p].begin(), aux.plate_defect_x[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.x < rhs.x; });
    // 将瑕疵按照y坐标排序
    for (int p = 0; p < aux.plate_defect_y.size(); ++p)
        sort(aux.plate_defect_y[p].begin(), aux.plate_defect_y[p].end(), [](Defect &lhs, Defect &rhs) { return lhs.y < rhs.y; });
    aux.param = input.param;
    aux.support_thread = thread::hardware_concurrency();
    return aux;
}

/* 将Solution格式的解转换成Problem::Output格式的解 */
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
    // parent_L0/1/2/3代表当前结点所在的1/2/3层桶的节点id
    ID nodeId = 0, parent_L0 = -1, parent_L1 = -1, parent_L2 = -1, parent_L3 = -1;
    Coord c1cpl = 0, c1cpr = 0, c2cpb = 0, c2cpu = 0, c3cp = 0;
    // 模拟物品放置切割过程
    for (int n = 0; n < sol.size(); ++n) {
        if (sol[n].getFlagBit(Placement::NEW_PLATE)) { // 使用一块新的原料
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
            // 更新当前各个cut的位置信息
            cur_plate += 1;
            c1cpl = 0;
            c1cpr = get_next_1cut(n, sol);
            c2cpb = 0;
            c2cpu = get_next_2cut(n, sol);
            c3cp = 0;
            // 添加节点
            parent_L0 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 原料节点
                cur_plate, nodeId++, 0, 0, PW, PH, SpecialType::Branch, 0, -1));
            parent_L1 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 一层桶节点
                cur_plate, nodeId++, 0, 0, c1cpr, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 二层桶节点
                cur_plate, nodeId++, 0, 0, c1cpr, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (sol[n].getFlagBit(Placement::NEW_L1)) { // 使用新的一层桶
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
            output.nodes.push_back(Problem::Output::Node( // + 一层桶
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, PH, SpecialType::Branch, 1, parent_L0));
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 二层桶
                cur_plate, nodeId++, c1cpl, 0, c1cpr - c1cpl, c2cpu, SpecialType::Branch, 2, parent_L1));
        } else if (sol[n].getFlagBit(Placement::NEW_L2)) {  // 使用新的二层桶
            if (c3cp < c1cpr) { // + waste_2
                output.nodes.push_back(Problem::Output::Node(
                    cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
            }
            c2cpb = c2cpu;
            c2cpu = get_next_2cut(n, sol);
            c3cp = c1cpl;
            parent_L2 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 二层桶
                cur_plate, nodeId++, c1cpl, c2cpb, c1cpr - c1cpl, c2cpu - c2cpb, SpecialType::Branch, 2, parent_L1));
        }
        // 获取物品宽高
        const Length item_w = sol[n].getFlagBit(Placement::ROTATE) ?
            aux.items[sol[n].item].h : aux.items[sol[n].item].w;
        const Length item_h = sol[n].getFlagBit(Placement::ROTATE) ?
            aux.items[sol[n].item].w : aux.items[sol[n].item].h;
        /*  ________________
           |   | |<-waste_5 |
           |   | |          |
           |   |d|  item    |
           |___|_|__________|
        */// 添加物品平移到瑕疵右侧所产生waste
        if (sol[n].getFlagBit(Placement::DEFECT_R)) {
            output.nodes.push_back(Problem::Output::Node( // + waste
                cur_plate, nodeId++, c3cp, c2cpb, sol[n].c3cp - item_w - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
        }
        // 如果该物品属于四层桶，直接把它放到上一个3层桶中，否则新建一个3层桶
        if (sol[n].getFlagBit(Placement::BIN4)) {
            output.nodes.push_back(Problem::Output::Node( // + 物品
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
            continue;
        } else {
            parent_L3 = nodeId;
            output.nodes.push_back(Problem::Output::Node( // + 三层桶
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - c2cpb, SpecialType::Branch, 3, parent_L2));
        }
        /*  ___________________
           |        |          |
           |  item  |          |
           |________|          |
           |   d    |<-waste_6 |
           |________|__________|
        */// 添加物品平移到瑕疵上侧所产生的waste
        if (sol[n].getFlagBit(Placement::DEFECT_U)) {
            output.nodes.push_back(Problem::Output::Node( // + waste
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpb, item_w, c2cpu - item_h - c2cpb, SpecialType::Waste, 4, parent_L3));
            output.nodes.push_back(Problem::Output::Node( // + 物品
                cur_plate, nodeId++, sol[n].c3cp - item_w, c2cpu - item_h, item_w, item_h, sol[n].item, 4, parent_L3));
        } else {
            output.nodes.push_back(Problem::Output::Node( // + 物品
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
        output.nodes.push_back(Problem::Output::Node( // + 最后一个waste_3
            cur_plate, nodeId++, c3cp, c2cpb, c1cpr - c3cp, c2cpu - c2cpb, SpecialType::Waste, 3, parent_L2));
    }
    if (c2cpu < aux.param.plateHeight) { // + 最后一个waste_2
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpl, c2cpu, c1cpr - c1cpl, PH - c2cpu, SpecialType::Waste, 2, parent_L1));
    }
    if (c1cpr < aux.param.plateWidth) { // + 余料
        output.nodes.push_back(Problem::Output::Node(
            cur_plate, nodeId++, c1cpr, 0, PW - c1cpr, PH, SpecialType::Residual, 1, parent_L0));
    }
    // 计算已使用原料的宽度
    output.totalWidth = 0;
    for (int i = 0; i < sol.size(); ++i) {
        if (output.totalWidth < cur_plate*aux.param.plateWidth + sol[i].c1cpr) {
            output.totalWidth = cur_plate * aux.param.plateWidth + sol[i].c1cpr;
        }
    }
    return output;
}

/* 从sol[index]开始，获取下一个1-cut的位置 */
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

/* 从sol[index]开始，获取下一个2-cut的位置 */
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
