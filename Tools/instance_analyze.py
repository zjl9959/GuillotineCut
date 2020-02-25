#!/usr/bin/python3

import csv
import matplotlib
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable


instance_dir = '../Deploy/Instance/'
batch_suffix = '_batch.csv'
defects_suffix = '_defects.csv'

def parse_batch(batch_path):
    item_set = set()
    stack_set = set()
    with open(batch_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()   # 跳过表头
        for row in reader:
            item_id = int(row[0])
            stack_id = int(row[3])
            item_set.add(item_id)
            stack_set.add(stack_id)
    return [len(item_set), len(stack_set)]

def parse_batch_size(batch_path):
    item_w_list = list()
    item_h_list = list()
    with open(batch_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()   # 跳过表头
        for row in reader:
            item_w = int(row[2])
            item_h = int(row[1])
            item_w_list.append(item_w)
            item_h_list.append(item_h)
    return [item_w_list, item_h_list]

def autolabel(rects, ax):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

def parse_defects(defects_path):
    plate_defects = [0 for _ in range(100)] # 最多有100块原料。
    with open(defects_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()
        for row in reader:
            plate_id = int(row[1])
            plate_defects[plate_id] += 1
    return plate_defects

def draw_log_bar(nb_item_list, nb_stack_list, instance_list):
    x = np.arange(len(instance_list))  # the label locations
    width = 0.35  # the width of the bars
    fig, ax = plt.subplots()
    rects1 = ax.bar(x - width/2, nb_item_list, width, label='Item')
    rects2 = ax.bar(x + width/2, nb_stack_list, width, label='Stack')
    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Number')
    ax.set_xticks(x)
    ax.set_xticklabels(instance_list)
    ax.legend()
    ax.set_yscale('log')
    autolabel(rects1, ax)
    autolabel(rects2, ax)
    fig.tight_layout()
    plt.show()

def draw_scatter_hist(x, y):
    fig, axScatter = plt.subplots(figsize=(5.5, 5.5))
    axScatter.scatter(x, y)
    axScatter.set_aspect(1.)
    divider = make_axes_locatable(axScatter)
    axHistx = divider.append_axes("top", 1.2, pad=0.1, sharex=axScatter)
    axHisty = divider.append_axes("right", 1.2, pad=0.1, sharey=axScatter)
    axHistx.xaxis.set_tick_params(labelbottom=False)
    axHisty.yaxis.set_tick_params(labelleft=False)
    x_min, x_max = min(x), max(x)
    y_min, y_max = min(y), max(y)
    bins_x = np.arange(x_min*0.9, x_max*1.1, (x_max - x_min)/50)
    bins_y = np.arange(y_min*0.9, y_max*1.1, (y_max - y_min)/30)
    axHistx.hist(x, bins=bins_x)
    axHisty.hist(y, bins=bins_y, orientation='horizontal')
    plt.show()

def run():
    inp = input('请输入算例（集）：').strip('\n')
    if len(inp) < 1 or inp[0] not in ['A', 'B', 'X']:
        return
    if len(inp) > 1:
        size = parse_batch_size(instance_dir + inp + batch_suffix)
        draw_scatter_hist(size[0], size[1])
    else:
        max_ins_id = 15
        if inp == 'A':
            max_ins_id = 20
        nb_item_list = list()
        nb_stack_list = list()
        instance_list = list()
        for i in range(1, max_ins_id + 1):
            res = parse_batch(instance_dir + inp + str(i) + batch_suffix)
            nb_item_list.append(res[0])
            nb_stack_list.append(res[1])
            instance_list.append(inp + str(i))
        draw_log_bar(nb_item_list, nb_stack_list, instance_list)

if __name__ == "__main__":
    run()
