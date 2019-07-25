#include "PlateSearch.h"

using namespace std;

namespace szx {

// ���ؿ����������㷨��max_iter�������ڵ������չ��Ŀ
Score PlateSearch::mcts(List<List<TID>>& batch, List<SolutionNode>& sol, int max_iter) {
    // [zjl][TODO]: add code.
    return Score();
}

// ��batch���������һ��������Ʒ���У�����֤�����к�node�ӽڵ��е���Ʒ���в�ͬ��
bool PlateSearch::gen_item_sequence(int seq_len, MCTNode *node, List<List<TID>>& batch, List<TID>& seq) {
    int left_item = 0;
    for (int i = 0; i < batch.size(); ++i)
        left_item += batch[i].size();
    seq_len = min(seq_len, left_item);  // seq_len������batch��ʣ����Ʒ��
    for (int i = 0; i < seq_len; ++i) {
        List<bool> item_hash(false, 1000);
        // ������ϣ����¼node�����ӽڵ���λ��seqλ�õ���Ʒid
        for (MCTNode *child : node->get_childrens()) {
            item_hash[child->get_solution_nodes()[i].item] = true;
        }
        // ����ջ�ף����ѡ��һ�����ظ�����Ʒ������seq��
        int samp_count = 1;     // ˮ����������
        int choose_batch = -1;  // ѡ���batch����
        for (int i = 0; i < batch.size(); ++i) {
            if (!batch[i].empty() && item_hash[batch[i].back()] == false &&
                rand_.pick(samp_count) == 0) {
                choose_batch = i;
                samp_count++;
            }
        }
        if (choose_batch >= 0) {    // ��ѡ�����Ʒ��ӽ�seq
            seq.push_back(batch[choose_batch].back());
            batch[choose_batch].pop_back();
        } else {
            break;
        }
    }
    return seq_len == seq.size();
}

}
