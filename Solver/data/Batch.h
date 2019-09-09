#pragma once
#ifndef GUILLOTINE_CUT_BATCH_H
#define GUILLOTINE_CUT_BATCH_H

#include "../Common.h"
#include "Problem.h"

namespace szx {

class Batch {   /* ʵ����Ʒջ��֧��������Ʒ����ѯջ����Ʒ�Ĳ��� */
public:
    /* Ĭ�Ϲ��캯�� */
    Batch() {}

    /* ͨ������list����
       Input: stacks�������Ʒ��ջ����reverse_stack���Ƿ�תջ�е���Ʒ��*/
    Batch(const List<List<ID>> &stacks, bool reverse_stack = false) : stack_index_(0) {
        // ��item��stacks������stacks_
        for (auto stack = stacks.begin(); stack != stacks.end(); ++stack) {
            if (!stack->empty()) {
                if (reverse_stack)
                    reverse(stack->begin(), stack->end());
                stacks_.push_back(*stack);
            }
        }
        // ����item id��stack id��ӳ���ϵ
        left_items_ = 0;
        for (int i = 0; i < stacks_.size(); ++i) {
            for (auto item : stacks_[i]) {
                item2stack_.insert(std::make_pair(item, i));
            }
            left_items_ += static_cast<ID>(stacks_[i].size());
        }
    }

    /* �������캯�� */
    Batch(const Batch &rhs) : stack_index_(0), left_items_(rhs.left_items_),
        stacks_(rhs.stacks_), item2stack_(rhs.item2stack_) {}
    /* ת�ƹ��캯�� */
    Batch(Batch &&rhs) : stack_index_(0), left_items_(rhs.left_items_),
        stacks_(move(rhs.stacks_)), item2stack_(move(rhs.item2stack_)) {}

    /* ������ֵ���� */
    Batch& operator=(const Batch &rhs) {
        if (this != &rhs) {
            stack_index_ = 0;
            left_items_ = rhs.left_items_;
            stacks_ = rhs.stacks_;
            item2stack_ = rhs.item2stack_;
        }
        return *this;
    }
    /* ת�Ƹ�ֵ���� */
    Batch& operator=(Batch &&rhs) {
        stack_index_ = 0;
        left_items_ = rhs.left_items_;
        stacks_ = move(rhs.stacks_);
        item2stack_ = move(rhs.item2stack_);
        return *this;
    }

    /* ��Batch�е���һ����Ʒ */
    void pop(ID item) {
        stacks_[item2stack_[item]].pop_back();
        left_items_--;
    }
    /* ��Batch�е��������Ʒ�����룺sol��������������Ʒ�ڵ㣩 */
    Batch& operator>>(Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            pop(it->item);
        }
        return *this;
    }

    /* ��Batch�����һ����Ʒ��������֮ǰ��Batch�е�������Ʒ */
    void push(ID item) {
        stacks_[item2stack_[item]].push_back(item);
        left_items_++;
    }
    /* ��Batch����Ӷ����Ʒ�����룺sol�������������Ʒ�ڵ㣩 */
    Batch& operator<<(Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            push(it->item);
        }
        return *this;
    }

    /* ���λ�ȡջ����Ʒ������������һ��ջ���򷵻���Ч����ƷID */
    ID get() {
        while (stack_index_ < stacks_.size() &&
            stacks_[stack_index_].empty()) {
            stack_index_++;   // �����յ�ջ
        }
        if (stack_index_ = stacks_.size()) {
            stack_index_ = 0;
            return Problem::InvalidItemId;
        }
        return stacks_[stack_index_++].back();
    }
    /* ��ȡ��stack_id��ջ�У���offset����Ʒ��id */
    ID get(ID stack_id, ID offset) const {
        if (stack_id < stacks_.size() && offset < stacks_[stack_id].size()) {
            return stacks_[stack_id][stacks_[stack_id].size() - offset - 1];
        }
        return Problem::InvalidItemId;
    }

    /* ����Batch��ʣ����Ʒ��Ŀ */
    ID size() const { return left_items_; }

    /* ����Batch��ջ����Ŀ */
    ID stack_num() const { return stacks_.size(); }
private:
    ID stack_index_;                // ָʾ��һ��getҪ��ȡ�ĸ�ջ�е���Ʒ
    ID left_items_;                 // ʣ����Ʒ��Ŀ
    List<List<ID>> stacks_;         // �����Ʒ��ջ
    HashMap<ID, int> item2stack_;   // ��ƷID��ջID��ӳ��
};

}

#endif // !GUILLOTINE_CUT_BATCH_H
