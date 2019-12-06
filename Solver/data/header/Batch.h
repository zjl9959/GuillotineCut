#pragma once
#ifndef GUILLOTINE_CUT_BATCH_H
#define GUILLOTINE_CUT_BATCH_H

#include "Solver/utility/Common.h"
#include "Solver/data/header/Placement.h"
#include "Solver/data/header/Global.h"

#include <cassert> 

namespace szx {

class Batch {   /* ʵ����Ʒջ��֧��������Ʒ����ѯջ����Ʒ�Ĳ��� */
public:
    /* Ĭ�Ϲ��캯�� */
    Batch() {}
    /* ͨ������list����
       Input: stacks�������Ʒ��ջ����reverse_stack���Ƿ�תջ�е���Ʒ��*/
    Batch(const List<List<TID>> &stacks, bool reverse_stack = false) {
        // ��item��stacks������stacks_
        for (auto stack = stacks.begin(); stack != stacks.end(); ++stack) {
            if (!stack->empty()) {
                stacks_.push_back(*stack);
                if (reverse_stack)
                    reverse(stacks_.back().begin(), stacks_.back().end());
            }
        }
        // ����item id��stack id��ӳ���ϵ
        for (int i = 0; i < stacks_.size(); ++i) {
            for (auto item : stacks_[i]) {
                item2stack_.insert(std::make_pair(item, i));
                total_item_area_ += gv::item_area[item];
            }
            left_items_ += static_cast<TID>(stacks_[i].size());
        }
        left_item_area_ = total_item_area_;
    }
    /* �������캯�� */
    Batch(const Batch &other) : left_items_(other.left_items_), total_item_area_(other.left_item_area_),
        left_item_area_(other.left_item_area_), stacks_(other.stacks_), item2stack_(other.item2stack_) {}
    /* ת�ƹ��캯�� */
    Batch(Batch &&other) : left_items_(other.left_items_), total_item_area_(other.left_item_area_),
        left_item_area_(other.left_item_area_), stacks_(move(other.stacks_)), item2stack_(move(other.item2stack_)) {}

    /* ������ֵ���� */
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
    /* ת�Ƹ�ֵ���� */
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

    /* ��Batch��ɾ��һ����Ʒ�����룺��Ʒid */
    void remove(TID item) {
		assert(stacks_[item2stack_[item]].back() == item);
        stacks_[item2stack_[item]].pop_back();
        left_items_--;
        left_item_area_ -= gv::item_area[item];
    }
    /* ��Batch��ɾ�������Ʒ�����룺sol��������������Ʒ�ڵ㣩 */
    void remove(const Solution &sol) {
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            remove(it->item);
        }
    }

    /* ��Batch�����һ����Ʒ��������֮ǰ��Batch�е�������Ʒ */
    void add(TID item) {
        assert(item2stack_.count(item));
        stacks_[item2stack_[item]].push_back(item);
        left_items_++;
        left_item_area_ += gv::item_area[item];
    }
    /* ��Batch����Ӷ����Ʒ�����룺sol�������������Ʒ�ڵ㣩 */
    void add(const Solution &sol) {
        for (auto it = sol.rbegin(); it != sol.rend(); ++it) {
            add(it->item);
        }
    }

    /* ��ȡ��stack_id��ջ��ջ����Ʒ */
    TID get(TID stack_id) const {
        if (!stacks_[stack_id].empty()) {
            return stacks_[stack_id].back();
        }
        return Problem::InvalidItemId;
    }
    /* ��ȡ��stack_id��ջ�У���offset����Ʒ��id */
    TID get(TID stack_id, TID offset) const {
        if (stack_id < stacks_.size() && offset < stacks_[stack_id].size()) {
            return stacks_[stack_id][stacks_[stack_id].size() - offset - 1];
        }
        return Problem::InvalidItemId;
    }

    /* ����Batch��ʣ����Ʒ��Ŀ */
    TID size() const { return left_items_; }
	/* ���ص�stack_id��ջ��ʣ����Ʒ��Ŀ */
	TID size(TID stack_id) const { 
		if (stack_id < stacks_.size()) {
			return static_cast<TID>(stacks_[stack_id].size());
		}
		return 0;
	}
    /* ����Batch��ջ����Ŀ��Ps��ջ����Ŀ�ǲ����� */
    TID stack_num() const { return static_cast<TID>(stacks_.size()); }

    /* ����Batch����Ʒ����� */
    Area total_item_area() const { return total_item_area_; }
    /* ����Batch��ʣ����Ʒ����� */
    Area left_item_area() const { return left_item_area_; }
    /* ����Batch���Ѿ�ʹ�õ���Ʒ����� */
    Area used_item_area() const { return total_item_area_ - left_item_area_; }

private:
    TID left_items_ = 0;             // ʣ����Ʒ��Ŀ
    Area total_item_area_ = 0;       // batch����Ʒ�����
    Area left_item_area_ = 0;        // ʣ����Ʒ�����
    List<List<TID>> stacks_;         // �����Ʒ��ջ
    HashMap<TID, int> item2stack_;   // ��ƷTID��ջTID��ӳ��
};

}

#endif // !GUILLOTINE_CUT_BATCH_H
