#!/usr/bin/python3

import json
import csv
import numpy
import datetime
import matplotlib
import matplotlib.pyplot as plt


plt.rcParams['font.sans-serif']=['SimHei']  #用来正常显示中文标签
plt.rcParams['axes.unicode_minus']=False    #用来正常显示负号

json_info_path = 'INFO.json'
log_path = '../Deploy/log.csv'
# 根据实际需要进行更改。
img_output_dir = 'C:\\毕业论文\\我的论文\\数据图表\\日志分析\\'
date_time = datetime.datetime.now().__format__('%y%m%d%H%M%S')

# 加载最优解参数信息。
with open(json_info_path, 'r') as f:
    json_data = json.load(f)

class Log():
    def __init__(self, instance, duration, waste, configure):
        self.instance = instance
        self.duration = duration
        self.waste = waste
        self.gap = (waste - json_data['best_sol_all'][instance]) / waste * 100
        item_area = json_data['item_area'][instance]
        self.usage_rate = item_area / (waste + item_area) * 100
        self.configure = configure

def parse_log_file():
    # [0]Time, [1]Instance, [2]RandSeed, [3]Duration, [4]Waste, [5]Configure
    log_list = []
    nb_error_log = 0
    with open(log_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()
        for row in reader:
            if int(row[4]) < 0: # 目标函数值小于0代表出错。
                nb_error_log += 1
                continue
            instance = row[1][row[1].find('/') + 1:]
            log_list.append(Log(instance, float(row[3]), int(row[4]), row[5]))
    print('共记录{}个无重复算例，筛除{}个错误算例'.format(len(log_list), nb_error_log))
    return log_list

def filter_by_group(logs, group):
    inst_log = dict()
    for log in logs:
        if group != log.instance[0]:
            continue
        inst = log.instance
        if inst in inst_log:
            if log.waste < inst_log[inst].waste:
                inst_log[inst] = log
        else:
            inst_log[inst] = log
    return inst_log

def filter_by_inst(logs, inst):
    cfg_log = dict()
    for log in logs:
        if inst != log.instance:
            continue
        cfg = log.configure
        if cfg in cfg_log:
            if log.waste < cfg_log[cfg].waste:
                cfg_log[cfg] = log
        else:
            cfg_log[cfg] = log
    return cfg_log

def autolabel(rect, ax):
    height = round(rect.get_height(), 2)
    ax.annotate('{}'.format(height),
                xy=(rect.get_x() + rect.get_width() / 2, height),
                xytext=(0, 3),  # 3 points vertical offset
                textcoords="offset points",
                ha='center', va='bottom')

def draw_bars(data, x_labels, y_label, title = None, img_path = None):
    ind = numpy.arange(len(x_labels))
    width = 0.35
    fig, ax = plt.subplots(figsize=(10.0, 8.0))
    if title:
        ax.set_title(title)
    rects = ax.bar(ind, data, width)
    ax.set_ylabel(y_label)
    ax.set_xticks(ind)
    ax.set_xticklabels(x_labels)
    fig.tight_layout()
    for i in range(len(rects)):
        autolabel(rects[i], ax)
    if img_path:
        plt.savefig(img_path)
    else:
        plt.show()

def draw_group_inst_log(group, logs):
    inst_log = filter_by_group(logs, group)
    title =  '与最优解之间的GAP（算例集' + group + '）'
    img_path = img_output_dir + '\\' + title + date_time + '.png'
    max_index = {'A': 20, 'B': 15, 'X': 15}[group]
    data = [0 for _ in range(max_index)]
    labels = [' ' for _ in range(max_index)]
    for inst in inst_log:
        instid = int(inst[1:])
        data[instid - 1] = inst_log[inst].gap
        labels[instid - 1] = inst
    if labels.count(' ') < 5:
        draw_bars(data, labels, 'GAP/%', title, img_path)
        print('保存图片：' + img_path)
    else:
        print('数据不足，无法绘图！')

def draw_inst_cfg_log(inst, logs):
    cfg_log = filter_by_inst(logs, inst)
    title = '各参数对比图（算例' + inst + '）'
    img_path = img_output_dir + '\\' + title + date_time + '.png'
    data, labels = list(), list()
    for cfg in cfg_log:
        data.append(cfg_log[cfg].gap)
        labels.append(cfg)
    if len(labels) > 2:
        draw_bars(data, labels, 'GAP/%', title, img_path)
        print('保存图片：' + img_path)
    else:
        print('数据不足，无法绘图！')

def run():
    logs = parse_log_file()
    inp1 = input('筛选条件：').strip('\n')
    if len(inp1) > 1:
        draw_inst_cfg_log(inp1, logs)
    else:
        draw_group_inst_log(inp1, logs)

if __name__ == "__main__":
    run()
