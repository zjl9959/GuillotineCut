#include "Solver/algorithm/header/Picker.h"

#include "Solver/data/header/Global.h"

using namespace std;

namespace szx {

Picker::Picker(const Batch & source) : source_(source), cache_(static_cast<ID>(gv::items.size())) {}

bool Picker::rand_pick(Batch &target_batch, Terminator terminator, Filter filter) {
    /* ʵ��ԭ��
      pool:         chosen:       source_bacth:
       B              A            A  B  C  D  E
                                         ^
       G(x)           F            F  G  H  I
                                         ^
       L              J  K         J  K  L  M  N  O  P
                                            ^
    */
    TID stack_num = source_.stack_num();
    List<pair<TID, TID>> pool; pool.reserve(stack_num); // pair<��ѡջid����Ӧջ����Ʒoffset>
    TID item_id = Problem::InvalidItemId;
    for (int i = 0; i < stack_num; ++i) {
        item_id = source_.get(i, 0);
        if (Problem::InvalidItemId != item_id && filter(item_id)) {
            // ��ѡ�Ϸ���Ʒջ���뱸ѡ��
            pool.push_back(make_pair(i, 0));
        }
    }
    if (pool.empty())return false;
    int max_rand_num = static_cast<int>(pool.size());
    List<List<TID>> chosen; chosen.resize(stack_num); // ����ѡ����Ʒ�б�
    List<TID> item_set;                               // ������Ʒid���ϣ������ж��ظ�
    while (true) {
        // �����ѡһ����Ʒ��������chosen�б���
        int rand_num = gv::rand.pick(max_rand_num);
        TID chosen_stack = pool[rand_num].first;
        item_id = source_.get(chosen_stack, pool[rand_num].second);
        if (terminator(item_id))
            break;
        chosen[chosen_stack].push_back(item_id);
        item_set.push_back(item_id);
        if (item_id == 103)
            item_id = item_id;
        // ��source_batch���ó�һ����Ʒ
        TID substitute_item = source_.get(chosen_stack, ++pool[rand_num].second);
        if (!(Problem::InvalidItemId != substitute_item && filter(substitute_item))) {
            // �������Ʒ���������Ʒ�������������
            swap(pool[rand_num], pool[--max_rand_num]);
            if (!max_rand_num) break;  // pool���ˣ���ǰֹͣ
        }
    }
    if (item_set.empty())
        return false;
    if (!cache_.set(item_set))
        return false;
    target_batch = move(Batch(chosen, true));
    return true;
}

bool Picker::Filter::operator()(TID item) {
    return (item != Problem::InvalidItemId) && (    // item��id��Ч��
        (gv::items[item].w < max_width_ && gv::items[item].h < max_height_) ||  // ����תitemʱ�����ߴ硣
        (gv::items[item].h < max_width_ && gv::items[item].w < max_height_));   // ��תitemʱ�����ߴ硣
}

bool Picker::Terminator::operator()(TID item) {
    if (item == Problem::InvalidItemId)
        return false;
    cur_item_num_++;
    cur_total_area_ += gv::item_area[item];
    if(max_item_num_ && cur_item_num_ > max_item_num_)
        return true;
    if (total_area_lb_ && cur_total_area_ > total_area_lb_)
        return true;
    return false;
}

}
