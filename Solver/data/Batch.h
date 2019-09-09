#pragma once
#ifndef GUILLOTINE_CUT_BATCH_H
#define GUILLOTINE_CUT_BATCH_H

#include "../Common.h"
#include "Problem.h"

namespace szx {

class Batch {   /* 实现物品栈，支持增减物品，查询栈首物品的操作 */
public:
    /* 默认构造函数 */
    Batch() {}

    /* 通过拷贝list构造
       Input: stacks（存放物品的栈），reverse_stack（是否翻转栈中的物品）*/
    Batch(const List<List<ID>> &stacks, bool reverse_stack = false) : stack_index_(0) {
        // 将item从stacks拷贝到stacks_
        for (auto stack = stacks.begin(); stack != stacks.end(); ++stack) {
            if (!stack->empty()) {
                if (reverse_stack)
                    reverse(stack->begin(), stack->end());
                stacks_.push_back(*stack);
            }
        }
        // 构建item id与stack id的映射关系
        left_items_ = 0;
        for (int i = 0; i < stacks_.size(); ++i) {
            for (auto item : stacks_[i]) {
                item2stack_.insert(std::make_pair(item, i));
            }
            left_items_ += static_cast<ID>(stacks_[i].size());
        }
    }

    /* 拷贝构造函数 */
    Batch(const Batch &rhs) : stack_index_(0), left_items_(rhs.left_items_),
        stacks_(rhs.stacks_), item2stack_(rhs.item2stack_) {}
    /* 转移构造函数 */
    Batch(Batch &&rhs) : stack_index_(0), left_items_(rhs.left_items_),
        stacks_(move(rhs.stacks_)), item2stack_(move(rhs.item2stack_)) {}

    /* 拷贝赋值函数 */
    Batch& operator=(const Batch &rhs) {
        if (this != &rhs) {
            stack_index_ = 0;
            left_items_ = rhs.left_items_;
            stacks_ = rhs.stacks_;
            item2stack_ = rhs.item2stack_;
        }
        return *this;
    }
    /* 转移赋值函数 */
    Batch& operator=(Batch &&rhs) {
        stack_index_ = 0;
        left_items_ = rhs.left_items_;
        stacks_ = move(rhs.stacks_);
        item2stack_ = move(rhs.item2stack_);
        return *this;
    }

    /* 从Batch中弹出一个物品 */
    void pop(ID item) {
        stacks_[item2stack_[item]].pop_back();
        left_items_--;
    }
    /* 从Batch中弹出多个物品，输入：sol（包含待弹出物品节点） */
    Batch& operator>>(Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            pop(it->item);
        }
        return *this;
    }

    /* 向Batch中添加一个物品，必须是之前从Batch中弹出的物品 */
    void push(ID item) {
        stacks_[item2stack_[item]].push_back(item);
        left_items_++;
    }
    /* 向Batch中添加多个物品，输入：sol（包含待添加物品节点） */
    Batch& operator<<(Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            push(it->item);
        }
        return *this;
    }

    /* 依次获取栈首物品，如果超过最后一个栈，则返回无效的物品ID */
    ID get() {
        while (stack_index_ < stacks_.size() &&
            stacks_[stack_index_].empty()) {
            stack_index_++;   // 跳过空的栈
        }
        if (stack_index_ = stacks_.size()) {
            stack_index_ = 0;
            return Problem::InvalidItemId;
        }
        return stacks_[stack_index_++].back();
    }
    /* 获取第stack_id个栈中，第offset个物品的id */
    ID get(ID stack_id, ID offset) const {
        if (stack_id < stacks_.size() && offset < stacks_[stack_id].size()) {
            return stacks_[stack_id][stacks_[stack_id].size() - offset - 1];
        }
        return Problem::InvalidItemId;
    }

    /* 返回Batch中剩余物品数目 */
    ID size() const { return left_items_; }

    /* 返回Batch中栈的数目 */
    ID stack_num() const { return stacks_.size(); }
private:
    ID stack_index_;                // 指示下一次get要获取哪个栈中的物品
    ID left_items_;                 // 剩余物品数目
    List<List<ID>> stacks_;         // 存放物品的栈
    HashMap<ID, int> item2stack_;   // 物品ID到栈ID的映射
};

}

#endif // !GUILLOTINE_CUT_BATCH_H
