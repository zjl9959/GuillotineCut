#include "Picker.h"

using namespace std;

namespace szx {

/*
根据source随机挑选最多max_num个物品。
输入：max_num（最多挑多少个物品），max_width（物品最大宽度限制）
输出：target_batch（挑选出来的物品所形成的栈），true：挑选成功，false：挑选失败
*/
bool Picker::rand_pick(int max_num, Length max_width, Batch &target_batch) {
    /* 实现原理
      pool:         chosen:       source_bacth:
       B              A            A  B  C  D  E
                                         ^
       G(x)           F            F  G  H  I
                                         ^
       L              J  K         J  K  L  M  N  O  P
                                            ^
    */
    ID stack_num = source_.stack_num();
    ID max_rand_num = stack_num;
    List<pair<ID, ID>> pool; pool.reserve(stack_num); // pair<备选栈id，对应栈内物品offset>
    // 初始化pool，备选栈id依次增长，栈偏移为0
    for (int i = 0; i < stack_num; ++i) {
        pool.push_back(make_pair(i, 0));
    }
    int item_id = Problem::InvalidItemId;
    for (int i = 0; i < stack_num; ++i) {
        item_id = source_.get(i, 0);
        if (!(Problem::InvalidItemId != item_id && max_width < aux_.items[item_id].w)) {
            // 物品非法或物品宽度超过最大限制，则不会挑选该物品所在栈
            swap(pool[i], pool[--max_rand_num]);
        }
    }
    List<List<ID>> chosen; chosen.resize(stack_num); // 被挑选的物品列表
    List<ID> item_set; item_set.reserve(max_num);  // 被挑物品id集合，用于判断重复
    ID chosen_item_num = 0; // 已经被挑选的物品数量
    while (chosen_item_num < max_num) {
        // 随机挑选一个物品，并放入chosen列表中
        int rand_num = rand_.pick(max_rand_num);
        ID chosen_stack = pool[rand_num].first;
        ID chosen_item = source_.get(chosen_stack, pool[rand_num].second);
        chosen[chosen_stack].push_back(chosen_item);
        chosen_item_num++;
        item_set.push_back(chosen_item);
        // 从source_batch中拿出一个物品
        ID substitute_item = source_.get(chosen_stack, ++pool[rand_num].second);
        if (!(Problem::InvalidItemId != item_id && max_width < aux_.items[substitute_item].w)) {
            // 无替代物品，或替代物品超过最大宽度限制
            swap(pool[rand_num], pool[--max_rand_num]);
            if (!max_rand_num) break;  // pool空了，提前停止
        }
    }
    if (!chosen_item_num)
        return false;
    if (!cache_.set(item_set))
        return false;
    target_batch = move(Batch(chosen, true));
    return true;
}

}
