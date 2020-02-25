#!/usr/bin/python3

import os
import csv
import json


instance_dir = '../Deploy/Instance/'
batch_suffix = '_batch.csv'
defects_suffix = '_defects.csv'

param_index = {
    'nb_item': 0,
    'nb_stack': 1,
    'item_area': 2,
    'min_item_w': 3,
    'max_item_w': 4,
    'min_item_h': 5,
    'max_item_h': 6,
    'plate_nb_defects': 7
    }

def parse_batch(batch_path, data):
    item_set = set()
    stack_set = set()
    item_area = 0
    min_item_w = 100000
    max_item_w = 0
    min_item_h = 100000
    max_item_h = 0
    with open(batch_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()   # 跳过表头
        for row in reader:
            item_id = int(row[0])
            item_h = int(row[1])
            item_w = int(row[2])
            stack_id = int(row[3])
            if item_id not in item_set:
                item_set.add(item_id)
                item_area += item_w*item_h
                min_item_w = min(min_item_w, item_w)
                max_item_w = max(max_item_w, item_w)
                min_item_h = min(min_item_h, item_h)
                max_item_h = max(max_item_h, item_h)
            stack_set.add(stack_id)
    data[param_index['nb_item']] = len(item_set)
    data[param_index['nb_stack']] = len(stack_set)
    data[param_index['item_area']] = item_area
    data[param_index['min_item_w']] = min_item_w
    data[param_index['max_item_w']] = max_item_w
    data[param_index['min_item_h']] = min_item_h
    data[param_index['max_item_h']] = max_item_h

def parse_defects(defects_path, data):
    plate_defects = [0 for _ in range(100)] # 最多有100块原料。
    with open(defects_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()
        for row in reader:
            plate_id = int(row[1])
            plate_defects[plate_id] += 1
    data[param_index['plate_nb_defects']] = plate_defects

def parse_all_instances():
    instance_list = list()
    # 加载算例A中的所有算例。
    for index in range(1, 21):
        instance_list.append('A' + str(index))
    # 加载算例B中的所有算例。
    for index in range(1, 16):
        instance_list.append('B' + str(index))
    # 加载算例X中的所有算例。
    for index in range(1, 16):
        instance_list.append('X' + str(index))
    json_data = dict()
    for instance in instance_list:
        json_data[instance] = [None for _ in range(len(param_index))]
        parse_batch(instance_dir + instance + batch_suffix, json_data[instance])
        parse_defects(instance_dir + instance + defects_suffix,
        json_data[instance])
    with open('INFO.json', 'w+') as f:
        json.dump(json_data, f)

def draw_images():
    pass

if __name__ == "__main__":
    parse_all_instances()
