#pragma once
#ifndef GUILLOTINE_CUT_BATCH_H
#define GUILLOTINE_CUT_BATCH_H

#include "../Common.h"
#include "Problem.h"
#include "../data/Placement.h"
#include <cassert> 

namespace szx {

class Batch {   /* 实现物品栈，支持增减物品，查询栈首物品的操作 */
public:
    /* 默认构造函数 */
    Batch() : left_items_(0) {}

    /* 通过拷贝list构造
       Input: stacks（存放物品的栈），reverse_stack（是否翻转栈中的物品）*/
    Batch(const List<List<TID>> &stacks, bool reverse_stack = false) {
        // 将item从stacks拷贝到stacks_
        for (auto stack = stacks.begin(); stack != stacks.end(); ++stack) {
            if (!stack->empty()) {
                stacks_.push_back(*stack);
                if (reverse_stack)
                    reverse(stacks_.back().begin(), stacks_.back().end());
            }
        }
        // 构建item id与stack id的映射关系
        left_items_ = 0;
        for (int i = 0; i < stacks_.size(); ++i) {
            for (auto item : stacks_[i]) {
                item2stack_.insert(std::make_pair(item, i));
            }
            left_items_ += static_cast<TID>(stacks_[i].size());
        }
    }

    /* 拷贝构造函数 */
    Batch(const Batch &rhs) : left_items_(rhs.left_items_),
        stacks_(rhs.stacks_), item2stack_(rhs.item2stack_) {}
    /* 转移构造函数 */
    Batch(Batch &&rhs) : left_items_(rhs.left_items_),
        stacks_(move(rhs.stacks_)), item2stack_(move(rhs.item2stack_)) {}

    /* 拷贝赋值函数 */
    Batch& operator=(const Batch &rhs) {
        if (this != &rhs) {
            left_items_ = rhs.left_items_;
            stacks_ = rhs.stacks_;
            item2stack_ = rhs.item2stack_;
        }
        return *this;
    }
    /* 转移赋值函数 */
    Batch& operator=(Batch &&rhs) {
        left_items_ = rhs.left_items_;
        stacks_ = move(rhs.stacks_);
        item2stack_ = move(rhs.item2stack_);
        return *this;
    }

    /* 从Batch中弹出一个物品 */
    void pop(TID item) {
		assert(stacks_[item2stack_[item]].back() == item); // [?] 增加断言或者其他判断
        stacks_[item2stack_[item]].pop_back();
        left_items_--;
    }
    /* 从Batch中弹出多个物品，输入：sol（包含待弹出物品节点） */
    Batch& operator>>(const Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            pop(it->item);
        }
        return *this;
    }

    /* 向Batch中添加一个物品，必须是之前从Batch中弹出的物品 */
    void push(TID item) {
        stacks_[item2stack_[item]].push_back(item);  // [?] 增加断言
        left_items_++;
    }
    /* 向Batch中添加多个物品，输入：sol（包含待添加物品节点） */
    Batch& operator<<(const Solution &sol) {
        for (auto it = sol.rbegin(); it != sol.rend(); ++it) {
            push(it->item);
        }
        return *this;
    }

    /* 获取第stack_id个栈的栈首物品 */
    TID get(TID stack_id) const {
        if (!stacks_[stack_id].empty()) {
            return stacks_[stack_id].back();
        }
        return Problem::InvalidItemId;
    }
    /* 获取第stack_id个栈中，第offset个物品的id */
    TID get(TID stack_id, TID offset) const {
        if (stack_id < stacks_.size() && offset < stacks_[stack_id].size()) {
            return stacks_[stack_id][stacks_[stack_id].size() - offset - 1];
        }
        return Problem::InvalidItemId;
    }

    /* 返回Batch中剩余物品数目 */
    inline TID size() const { return left_items_; }

	/* 返回第stack_id个栈中剩余物品数目 */
	TID size(TID stack_id) const { 
		if (stack_id < stacks_.size()) {
			return static_cast<TID>(stacks_[stack_id].size());
		}
		return 0;
	}

    /* 返回Batch中栈的数目，Ps：栈的数目是不定的 */
    inline TID stack_num() const { return static_cast<TID>(stacks_.size()); }

private:
    TID left_items_;                 // 剩余物品数目
    List<List<TID>> stacks_;         // 存放物品的栈
    HashMap<TID, int> item2stack_;   // 物品TID到栈TID的映射
};

}

#endif // !GUILLOTINE_CUT_BATCH_H
