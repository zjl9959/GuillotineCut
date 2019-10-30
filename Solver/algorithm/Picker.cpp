#include "Picker.h"

using namespace std;

namespace szx {

bool Picker::rand_pick(Batch &target_batch, Terminator &terminator, Filter &filter) {
    /* 实现原理
      pool:         chosen:       source_bacth:
       B              A            A  B  C  D  E
                                         ^
       G(x)           F            F  G  H  I
                                         ^
       L              J  K         J  K  L  M  N  O  P
                                            ^
    */
    TID stack_num = source_.stack_num();
    List<pair<TID, TID>> pool; pool.reserve(stack_num); // pair<备选栈id，对应栈内物品offset>
    TID item_id = Problem::InvalidItemId;
    for (int i = 0; i < stack_num; ++i) {
        item_id = source_.get(i, 0);
        if (Problem::InvalidItemId != item_id && filter(item_id, aux_)) {
            // 挑选合法物品栈放入备选池
            pool.push_back(make_pair(i, 0));
        }
    }
    if (pool.empty())return false;
    TID max_rand_num = pool.size();
    List<List<TID>> chosen; chosen.resize(stack_num); // 被挑选的物品列表
    List<TID> item_set;                               // 被挑物品id集合，用于判断重复
    while (true) {
        // 随机挑选一个物品，并放入chosen列表中
        int rand_num = rand_.pick(max_rand_num);
        TID chosen_stack = pool[rand_num].first;
        item_id = source_.get(chosen_stack, pool[rand_num].second);
        if (terminator(item_id, aux_))
            break;
        chosen[chosen_stack].push_back(item_id);
        item_set.push_back(item_id);
        if (item_id == 103)
            item_id = item_id;
        // 从source_batch中拿出一个物品
        TID substitute_item = source_.get(chosen_stack, ++pool[rand_num].second);
        if (!(Problem::InvalidItemId != substitute_item && filter(substitute_item, aux_))) {
            // 无替代物品，或替代物品超过最大宽度限制
            swap(pool[rand_num], pool[--max_rand_num]);
            if (!max_rand_num) break;  // pool空了，提前停止
        }
    }
    if (item_set.empty())
        return false;
    if (!cache_.set(item_set))
        return false;
    target_batch = move(Batch(chosen, true));
    return true;
}

bool Picker::Filter::operator()(TID item, const Auxiliary &aux) {
    if (item == Problem::InvalidItemId)
        return false;
    if(min_width_ && aux.items[item].w < min_width_)
        return false;
    if (max_width_ && aux.items[item].w > max_width_)
        return false;
    if (min_height_ && aux.items[item].h < min_height_)
        return false;
    if (max_height_ && aux.items[item].h > max_height_)
        return false;
    return true;
}

bool Picker::Terminator::operator()(TID item, const Auxiliary &aux) {
    if (item == Problem::InvalidItemId)
        return false;
    cur_item_num_++;
    cur_total_area_ += aux.item_area[item];
    if(max_item_num_ && cur_item_num_ > max_item_num_)
        return true;
    if (total_area_lb_ && cur_total_area_ > total_area_lb_)
        return true;
    return false;
}

}
