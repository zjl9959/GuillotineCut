#include "Picker.h"

using namespace std;

namespace szx {

/*
����source�����ѡ���max_num����Ʒ��
���룺max_num����������ٸ���Ʒ����max_width����Ʒ��������ƣ�
�����target_batch����ѡ��������Ʒ���γɵ�ջ����true����ѡ�ɹ���false����ѡʧ��
*/
bool Picker::rand_pick(int max_num, Length max_width, Batch &target_batch) {
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
    int item_id = Problem::InvalidItemId;
    for (int i = 0; i < stack_num; ++i) {
        item_id = source_.get(i, 0);
        if (!(Problem::InvalidItemId != item_id && max_width < aux_.items[item_id].w)) {
            // ��Ʒ�Ƿ�����Ʒ��ȳ���������ƣ��򲻻���ѡ����Ʒ����ջ
            swap(pool[i], pool[--max_rand_num]);
        }
    }
    List<List<ID>> chosen; chosen.resize(stack_num); // ����ѡ����Ʒ�б�
    List<ID> item_set; item_set.reserve(max_num);  // ������Ʒid���ϣ������ж��ظ�
    ID chosen_item_num = 0; // �Ѿ�����ѡ����Ʒ����
    while (chosen_item_num < max_num) {
        // �����ѡһ����Ʒ��������chosen�б���
        int rand_num = rand_.pick(max_rand_num);
        ID chosen_stack = pool[rand_num].first;
        ID chosen_item = source_.get(chosen_stack, pool[rand_num].second);
        chosen[chosen_stack].push_back(chosen_item);
        chosen_item_num++;
        item_set.push_back(chosen_item);
        // ��source_batch���ó�һ����Ʒ
        ID substitute_item = source_.get(chosen_stack, ++pool[rand_num].second);
        if (!(Problem::InvalidItemId != item_id && max_width < aux_.items[substitute_item].w)) {
            // �������Ʒ���������Ʒ�������������
            swap(pool[rand_num], pool[--max_rand_num]);
            if (!max_rand_num) break;  // pool���ˣ���ǰֹͣ
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
