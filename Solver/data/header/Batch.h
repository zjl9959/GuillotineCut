#pragma once
#ifndef GUILLOTINE_CUT_BATCH_H
#define GUILLOTINE_CUT_BATCH_H

#include "Solver/utility/Common.h"
#include "Solver/data/header/Placement.h"
#include "Solver/data/header/Global.h"

#include <cassert> 

namespace szx {

class Batch {   /* 实现物品栈，支持增减物品，查询栈首物品的操作 */
public:
    /* 默认构造函数 */
    Batch() {}
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
        for (int i = 0; i < stacks_.size(); ++i) {
            for (auto item : stacks_[i]) {
                item2stack_.insert(std::make_pair(item, i));
                total_item_area_ += gv::item_area[item];
            }
            left_items_ += static_cast<TID>(stacks_[i].size());
        }
        left_item_area_ = total_item_area_;
    }
    /* 拷贝构造函数 */
    Batch(const Batch &other) : left_items_(other.left_items_), total_item_area_(other.left_item_area_),
        left_item_area_(other.left_item_area_), stacks_(other.stacks_), item2stack_(other.item2stack_) {}
    /* 转移构造函数 */
    Batch(Batch &&other) : left_items_(other.left_items_), total_item_area_(other.left_item_area_),
        left_item_area_(other.left_item_area_), stacks_(move(other.stacks_)), item2stack_(move(other.item2stack_)) {}

    /* 拷贝赋值函数 */
    Batch& operator=(const Batch &other) {
        if (this != &other) {
            left_items_ = other.left_items_;
            total_item_area_ = other.total_item_area_;
            left_item_area_ = other.left_item_area_;
            stacks_ = other.stacks_;
            item2stack_ = other.item2stack_;
        }
        return *this;
    }
    /* 转移赋值函数 */
    Batch& operator=(Batch &&other) {
        if (this != &other) {
            left_items_ = other.left_items_;
            total_item_area_ = other.total_item_area_;
            left_item_area_ = other.left_item_area_;
            stacks_ = move(other.stacks_);
            item2stack_ = move(other.item2stack_);
        }
        return *this;
    }

    /* 从Batch中删除一个物品，输入：物品id */
    void remove(TID item) {
		assert(stacks_[item2stack_[item]].back() == item);
        stacks_[item2stack_[item]].pop_back();
        left_items_--;
        left_item_area_ -= gv::item_area[item];
    }
    /* 从Batch中删除多个物品，输入：sol（包含待弹出物品节点） */
    void remove(const Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            remove(it->item);
        }
    }

    /* 向Batch中添加一个物品，必须是之前从Batch中弹出的物品 */
    void add(TID item) {
        assert(item2stack_.count(item));
        stacks_[item2stack_[item]].push_back(item);
        left_items_++;
        left_item_area_ += gv::item_area[item];
    }
    /* 向Batch中添加多个物品，输入：sol（包含待添加物品节点） */
    void add(const Solution &sol) {
        for (auto it = sol.rbegin(); it != sol.rend(); ++it) {
            add(it->item);
        }
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
    TID size() const { return left_items_; }
	/* 返回第stack_id个栈中剩余物品数目 */
	TID size(TID stack_id) const { 
		if (stack_id < stacks_.size()) {
			return static_cast<TID>(stacks_[stack_id].size());
		}
		return 0;
	}
    /* 返回Batch中栈的数目，Ps：栈的数目是不定的 */
    TID stack_num() const { return static_cast<TID>(stacks_.size()); }

    /* 返回Batch中物品总面积 */
    Area total_item_area() const { return total_item_area_; }
    /* 返回Batch中剩余物品总面积 */
    Area left_item_area() const { return left_item_area_; }
    /* 返回Batch中已经使用的物品总面积 */
    Area used_item_area() const { return total_item_area_ - left_item_area_; }

private:
    TID left_items_ = 0;             // 剩余物品数目
    Area total_item_area_ = 0;       // batch中物品总面积
    Area left_item_area_ = 0;        // 剩余物品面积和
    List<List<TID>> stacks_;         // 存放物品的栈
    HashMap<TID, int> item2stack_;   // 物品TID到栈TID的映射
};

}

#endif // !GUILLOTINE_CUT_BATCH_H
