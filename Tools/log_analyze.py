#!/usr/bin/python3

import json
import csv
import numpy
import copy
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
        self.gap = (waste - json_data['offical_3600'][instance]) / waste * 100
        item_area = json_data['item_area'][instance]
        self.usage_rate = item_area / (waste + item_area) * 100
        self.configure = configure

class DataGroup():
    def __init__(self, x_labels, y_lable):
        self.x_labels = x_labels
        self.y_label = y_lable
        self.datas = list()
        self.names = list()

    def add_data(self, name, data):
        if len(data) == len(self.x_labels):
            self.datas.append(data)
            self.names.append(name)

def parse_log_file():
    # [0]Time, [1]Instance, [2]RandSeed, [3]Duration, [4]Waste, [5]Configure
    log_list = []
    nb_error_log = 0
    with open(log_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()
        for row in reader:
            if len(row) < 5:
                continue
            if int(row[4]) < 0: # 目标函数值小于0代表出错。
                nb_error_log += 1
                continue
            instance = row[1][row[1].find('/') + 1:]
            log_list.append(Log(instance, float(row[3]), int(row[4]), row[5]))
    print('共记录{}个无重复算例，筛除{}个错误算例'.format(len(log_list), nb_error_log))
    return log_list

def filter_by_group(logs, group):
    res = list()
    for log in logs:
        if group == log.instance[0]:
            res.append(log)
    return res

def filter_by_inst(logs, inst):
    res = list()
    for log in logs:
        if inst != log.instance:
            res.append(log)
    return res

def filter_by_cfg(logs, cfg):
    res = list()
    for log in logs:
        if cfg in log.configure:
            res.append(log)
    return res

# merge_mode:"ins":[返回不同的instance]，"cfg":[返回不同的configure]，"all":[返回不同的instance+configure]
# cmp_mode:"worst":[保留最差的目标值]，"best":[保留最好的目标值]，"average":[取平均值]。
def merge_logs(logs, merge_mode, cmp_mode):
    log_map = dict()
    for log in logs:
        log_id = log.instance + log.configure
        if merge_mode == 'ins':
            log_id = log.instance
        elif merge_mode == 'cfg':
            log_id = log.configure
        if log_id in log_map:
            log_map[log_id].append(log)
        else:
            log_map[log_id] = [log]
    res = list()
    for log_id in log_map.keys():
        temp_log = copy.deepcopy(log_map[log_id][0])
        for log in log_map[log_id][1:]:
            if cmp_mode == 'worst' and log.waste > temp_log.waste:
                temp_log.waste = log.waste
                temp_log.gap = log.gap
                temp_log.usage_rate = log.usage_rate
            elif cmp_mode == 'best' and log.waste < temp_log.waste:
                temp_log.waste = log.waste
                temp_log.gap = log.gap
                temp_log.usage_rate = log.usage_rate
            elif cmp_mode == 'average':
                temp_log.waste += log.waste
                temp_log.usage_rate += log.usage_rate
                temp_log.gap += log.gap
        if cmp_mode == 'average':
            temp_log.waste /= len(log_map[log_id])
            temp_log.usage_rate /= len(log_map[log_id])
            temp_log.gap /= len(log_map[log_id])
        res.append(temp_log)
    return res

def autolabel(rect, ax):
    height = round(rect.get_height(), 2)
    ax.annotate('{:.2f}'.format(height),
                xy=(rect.get_x() + rect.get_width() / 2, height),
                xytext=(0, 3),  # 3 points vertical offset
                textcoords="offset points",
                ha='center', va='bottom')

def draw_bars(bar_data, title = None, img_path = None):
    ind = numpy.arange(len(bar_data.x_labels))
    width = 0.7 / len(bar_data.datas)
    fig, ax = plt.subplots(figsize=(10.0, 8.0))
    if title:
        ax.set_title(title)
    rects_list = list()
    for i in range(len(bar_data.datas)):
        x_pos = ind + (i - (len(bar_data.datas)-1)*0.5)*width
        rects = ax.bar(x_pos, bar_data.datas[i], width, label=bar_data.names[i])
        rects_list.append(rects)
    ax.set_ylabel(bar_data.y_label)
    ax.set_xticks(ind)
    ax.set_xticklabels(bar_data.x_labels)
    if len(bar_data.names) > 1:
        ax.legend()
    fig.tight_layout()
    if len(rects_list) == 0:
        return
    if len(bar_data.datas) < 3:
        for j in range(len(rects_list[0])):
            height_set = set()
            for i in range(len(rects_list)):
                height = '{:.2f}'.format(rects_list[i][j].get_height())
                if height not in height_set:
                    autolabel(rects_list[i][j], ax)
                    height_set.add(height)
    if img_path:
        plt.savefig(img_path)
    else:
        plt.show()

def draw_polylines(bar_data, title=None, img_path=None):
    ind = numpy.arange(len(bar_data.x_labels))
    fig, ax = plt.subplots(figsize=(10.0, 8.0))
    if title:
        ax.set_title(title)
    for i in range(len(bar_data.datas)):
        ax.plot(ind, bar_data.datas[i], label=bar_data.names[i])
    ax.set_ylabel(bar_data.y_label)
    ax.set_xticks(ind)
    ax.set_xticklabels(bar_data.x_labels)
    if len(bar_data.names) > 1:
        ax.legend()
    if img_path:
        plt.savefig(img_path)
    else:
        plt.show()

def draw_group_inst_log(group, logs):
    logs = filter_by_group(logs, group)
    logs = merge_logs(logs, 'ins', 'best')
    title =  '与最优解之间的GAP（算例集' + group + '）'
    img_path = img_output_dir + '\\' + title + date_time + '.png'
    max_index = 15
    data = [0 for _ in range(max_index)]
    labels = [' ' for _ in range(max_index)]
    for log in logs:
        instid = int(log.instance[1:])
        data[instid - 1] = log.gap
        labels[instid - 1] = log.instance
    data2 = list()
    for inst_name in json_data['J29_final_3600']:
        if group in inst_name:
            waste = json_data['J29_final_3600'][inst_name]
            gap = (waste - json_data['offical_3600'][inst_name]) / waste * 100
            data2.append(gap)
    bar_data = DataGroup(labels, 'GAP/%')
    bar_data.add_data('当前版本', data)
    bar_data.add_data('竞赛版本', data2)
    if labels.count(' ') < 5:
        draw_bars(bar_data, title, img_path)
        print('保存图片：' + img_path)
    else:
        print('数据不足，无法绘图！')

def draw_group_compare_log(group, logs):
    logs = filter_by_group(logs, group)
    logs_best = merge_logs(logs, 'ins', 'best')
    logs_dfs = filter_by_cfg(logs, 'D')
    logs_dfs = merge_logs(logs_dfs, 'ins', 'best')
    logs_pfs = filter_by_cfg(logs, 'P')
    logs_pfs = merge_logs(logs_pfs, 'ins', 'best')
    logs_beam = filter_by_cfg(logs, 'B')
    logs_beam = merge_logs(logs_beam, 'ins', 'best')
    title =  '子算法求解效果对比图（算例集' + group + '）'
    img_path = img_output_dir + '\\' + title + date_time + '.png'
    max_index = 15
    datas_best = [0 for _ in range(max_index)]
    for log in logs_best:
        instid = int(log.instance[1:])
        datas_best[instid - 1] = log.waste
    datas_dfs = [0 for _ in range(max_index)]
    labels = [group + str(i + 1) for i in range(max_index)]
    for log in logs_dfs:
        instid = int(log.instance[1:]) - 1
        datas_dfs[instid] = 100 - (log.waste - datas_best[instid])/log.waste*100
    datas_pfs = [0 for _ in range(max_index)]
    for log in logs_pfs:
        instid = int(log.instance[1:]) - 1
        datas_pfs[instid] = 100 - (log.waste - datas_best[instid])/log.waste*100
    datas_beam = [0 for _ in range(max_index)]
    for log in logs_beam:
        instid = int(log.instance[1:]) - 1
        datas_beam[instid] = 100 - (log.waste - datas_best[instid])/log.waste*100
    bar_data = DataGroup(labels, 'Score')
    bar_data.add_data('HISA-DFS', datas_dfs)
    bar_data.add_data('HISA-A*', datas_pfs)
    bar_data.add_data('HISA-Beam', datas_beam)
    draw_bars(bar_data, None, img_path)
    print('保存图片：' + img_path)

def draw_time_log(logs):
    logs_dfs = filter_by_cfg(logs, 'D')
    logs_dfs = merge_logs(logs_dfs, 'ins', 'average')
    logs_pfs = filter_by_cfg(logs, 'P')
    logs_pfs = merge_logs(logs_pfs, 'ins', 'average')
    logs_beam = filter_by_cfg(logs, 'B')
    logs_beam = merge_logs(logs_beam, 'ins', 'average')
    title =  '子算法运行耗时对比图'
    img_path = img_output_dir + '\\' + title + date_time + '.png'
    labels = ['X' + str(i + 1) for i in range(15)]
    labels += ['B' + str(i + 1) for i in range(15)]
    datas_dfs = [0 for _ in range(30)]
    for log in logs_dfs:
        instid = int(log.instance[1:]) - 1
        if log.instance[0] == 'B':
            instid += 15
        datas_dfs[instid] = log.duration
    datas_pfs = [0 for _ in range(30)]
    for log in logs_pfs:
        instid = int(log.instance[1:]) - 1
        if log.instance[0] == 'B':
            instid += 15
        datas_pfs[instid] = log.duration
    datas_beam = [0 for _ in range(30)]
    for log in logs_beam:
        instid = int(log.instance[1:]) - 1
        if log.instance[0] == 'B':
            instid += 15
        datas_beam[instid] = log.duration
    line_data = DataGroup(labels, 'CPUTIME/S')
    line_data.add_data('HISA-DFS', datas_dfs)
    line_data.add_data('HISA-A*', datas_pfs)
    line_data.add_data('HISA-Beam', datas_beam)
    draw_polylines(line_data, None, img_path)
    print('保存图片：' + img_path)

def draw_inst_cfg_log(inst, logs):
    logs = filter_by_inst(logs, inst)
    logs = merge_logs(logs, 'cfg', 'best')
    title = '各参数对比图（算例' + inst + '）'
    img_path = img_output_dir + '\\' + title + date_time + '.png'
    data, labels = list(), list()
    for log in logs:
        data.append(log.gap)
        labels.append(log.configure.replace('_', '\n'))
    bar_data = DataGroup(labels, 'GAP/%')
    bar_data.add_data('', data)
    if len(labels) > 2:
        draw_bars(bar_data, title, img_path)
        print('保存图片：' + img_path)
    else:
        print('数据不足，无法绘图！')

def run():
    logs = parse_log_file()
    inp1 = input('筛选条件：').strip('\n')
    if len(inp1) > 1:
        draw_inst_cfg_log(inp1, logs)
    else:
        #draw_group_inst_log(inp1, logs)
        draw_group_compare_log(inp1, logs)
    #draw_time_log(logs)

if __name__ == "__main__":
    run()
