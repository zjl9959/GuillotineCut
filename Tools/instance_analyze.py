#!/usr/bin/python3
#coding:utf-8

import csv
import matplotlib
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable

plt.rcParams['font.sans-serif']=['SimHei']  #用来正常显示中文标签
plt.rcParams['axes.unicode_minus']=False    #用来正常显示负号

instance_dir = '../Deploy/Instance/'
batch_suffix = '_batch.csv'
defects_suffix = '_defects.csv'
# 根据实际需要进行更改。
img_output_dir = 'C:\\毕业论文\\我的论文\\数据图表\\算例分析\\'

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

def parse_defects(defects_path):
    plate_defects = [0 for _ in range(100)] # 最多有100块原料。
    with open(defects_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()
        for row in reader:
            plate_id = int(row[1])
            plate_defects[plate_id] += 1
    return plate_defects

def autolabel(rect, ax):
    height = rect.get_height()
    ax.annotate('{}'.format(height),
                xy=(rect.get_x() + rect.get_width() / 2, height),
                xytext=(0, 3),  # 3 points vertical offset
                textcoords="offset points",
                ha='center', va='bottom')

def draw_log_bar(nb_item_list, nb_stack_list, instance_list, out_path, title):
    x = np.arange(len(instance_list))  # the label locations
    width = 0.35  # the width of the bars
    fig, ax = plt.subplots(figsize=(8.0, 6.5))
    ax.set_title(title)
    rects1 = ax.bar(x - width/2, nb_item_list, width, label='成品')
    rects2 = ax.bar(x + width/2, nb_stack_list, width, label='栈')
    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('数量/个')
    ax.set_xticks(x)
    ax.set_xticklabels(instance_list)
    ax.legend()
    ax.set_yscale('log')
    for i in range(len(rects1)):
        autolabel(rects1[i], ax)
        # 高度相同的Bar只添加一个数字标签。
        if rects1[i].get_height() != rects2[i].get_height():
            autolabel(rects2[i], ax)
    fig.tight_layout()
    plt.savefig(out_path)

def draw_scatter_hist(x, y, out_path, title):
    fig, axScatter = plt.subplots(figsize=(6.5, 4.0))
    axScatter.scatter(x, y)
    axScatter.set_aspect(1.)
    axScatter.set_xlabel('宽度/毫米')
    axScatter.set_ylabel('高度/毫米')
    divider = make_axes_locatable(axScatter)
    axHistx = divider.append_axes("top", 1.2, pad=0.1, sharex=axScatter)
    axHisty = divider.append_axes("right", 1.2, pad=0.1, sharey=axScatter)
    axHistx.xaxis.set_tick_params(labelbottom=False)
    axHisty.yaxis.set_tick_params(labelleft=False)
    x_min, x_max = 0, 6000
    y_min, y_max = 0, 3210
    bins_x = np.arange(x_min, x_max, (x_max - x_min)/50)
    bins_y = np.arange(y_min, y_max, (y_max - y_min)/30)
    axHistx.hist(x, bins=bins_x)
    axHisty.hist(y, bins=bins_y, orientation='horizontal')
    axHistx.set_ylabel('计数/个')
    axHisty.set_xlabel('计数/个')
    axHistx.set_title(title)
    plt.savefig(out_path)

def run():
    for group in ['A', 'B', 'X']:
        max_ins_id = 15
        if group == 'A':
            max_ins_id = 20
        nb_item_list = list()
        nb_stack_list = list()
        instance_list = list()
        for i in range(1, max_ins_id + 1):
            inst_name = group + str(i)
            '''
            scatter_hist_path = img_output_dir + '算例' + inst_name + '物品长宽分布统计图.png'
            size = parse_batch_size(instance_dir + inst_name + batch_suffix)
            draw_scatter_hist(size[0], size[1], scatter_hist_path, '                 算例' + inst_name + '物品长宽分布统计图')
            print('保存图片：' + scatter_hist_path)
            '''
            res = parse_batch(instance_dir + inst_name + batch_suffix)
            nb_item_list.append(res[0])
            nb_stack_list.append(res[1])
            instance_list.append(inst_name)
        count_path = img_output_dir + '算例集' + group + '成品和栈的数量统计图.png' 
        draw_log_bar(nb_item_list, nb_stack_list, instance_list, count_path, '算例集' + group + '成品和栈的数量统计图')
        print("保存图片：" + count_path)

if __name__ == "__main__":
    run()
