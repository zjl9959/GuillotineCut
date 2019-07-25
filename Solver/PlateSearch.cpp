#include "PlateSearch.h"

using namespace std;

namespace szx {

// 蒙特卡洛树搜索算法，max_iter代表树节点最大扩展数目
Score PlateSearch::mcts(List<List<TID>>& batch, List<SolutionNode>& sol, int max_iter) {
    // [zjl][TODO]: add code.
    return Score();
}

// 从batch中随机产生一个可行物品序列，并保证该序列和node子节点中的物品序列不同。
bool PlateSearch::gen_item_sequence(int seq_len, MCTNode *node, List<List<TID>>& batch, List<TID>& seq) {
    int left_item = 0;
    for (int i = 0; i < batch.size(); ++i)
        left_item += batch[i].size();
    seq_len = min(seq_len, left_item);  // seq_len不大于batch中剩余物品数
    for (int i = 0; i < seq_len; ++i) {
        List<bool> item_hash(false, 1000);
        // 构建哈希，记录node所有子节点中位于seq位置的物品id
        for (MCTNode *child : node->get_childrens()) {
            item_hash[child->get_solution_nodes()[i].item] = true;
        }
        // 遍历栈首，随机选择一个非重复的物品，放入seq中
        int samp_count = 1;     // 水塘抽样计数
        int choose_batch = -1;  // 选择的batch索引
        for (int i = 0; i < batch.size(); ++i) {
            if (!batch[i].empty() && item_hash[batch[i].back()] == false &&
                rand_.pick(samp_count) == 0) {
                choose_batch = i;
                samp_count++;
            }
        }
        if (choose_batch >= 0) {    // 将选择的物品添加进seq
            seq.push_back(batch[choose_batch].back());
            batch[choose_batch].pop_back();
        } else {
            break;
        }
    }
    return seq_len == seq.size();
}

}
