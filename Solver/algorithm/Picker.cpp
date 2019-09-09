#include "Picker.h"

using namespace std;

namespace szx {

bool Picker::rand_pick(Batch &target_batch, Terminator &terminator, Filter &filter) {
    /* ʵ��ԭ��
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
    List<pair<ID, ID>> pool; pool.reserve(stack_num); // pair<��ѡջid����Ӧջ����Ʒoffset>
    // ��ʼ��pool����ѡջid����������ջƫ��Ϊ0
    for (int i = 0; i < stack_num; ++i) {
        pool.push_back(make_pair(i, 0));
    }
    ID item_id = Problem::InvalidItemId;
    for (int i = 0; i < stack_num; ++i) {
        item_id = source_.get(i, 0);
        if (!(Problem::InvalidItemId != item_id && filter(item_id, aux_))) {
            // ��Ʒ�Ƿ�����Ʒ��ȳ���������ƣ��򲻻���ѡ����Ʒ����ջ
            swap(pool[i], pool[--max_rand_num]);
        }
    }
    List<List<ID>> chosen; chosen.resize(stack_num); // ����ѡ����Ʒ�б�
    List<ID> item_set;                               // ������Ʒid���ϣ������ж��ظ�
    while (true) {
        // �����ѡһ����Ʒ��������chosen�б���
        int rand_num = rand_.pick(max_rand_num);
        ID chosen_stack = pool[rand_num].first;
        ID chosen_item = source_.get(chosen_stack, pool[rand_num].second);
        chosen[chosen_stack].push_back(chosen_item);
        item_set.push_back(chosen_item);
        if (terminator(chosen_item, aux_))
            break;
        // ��source_batch���ó�һ����Ʒ
        ID substitute_item = source_.get(chosen_stack, ++pool[rand_num].second);
        if (!(Problem::InvalidItemId != item_id && filter(item_id, aux_))) {
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

bool Picker::Filter::operator()(ID item, const Auxiliary &aux) {
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

bool Picker::Terminator::operator()(ID item, const Auxiliary &aux) {
    cur_item_num_++;
    cur_total_area_ += aux.item_area[item];
    if(max_item_num_ && cur_item_num_ > max_item_num_)
        return true;
    if (total_area_lb_ && cur_total_area_ > total_area_lb_)
        return true;
    return false;
}

}
