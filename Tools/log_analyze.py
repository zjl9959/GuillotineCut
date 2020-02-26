#!/usr/bin/python3

import os
import json
import csv
import git
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


class Log():

    def __init__(self, instance, duration, waste, configure, statistic):
        self.instance = instance
        self.duration = duration
        self.waste = waste
        self.configure = configure
        self.statistic = statistic


def parse_log_file(start_date, end_date):
    # [0]Time, [1]Instance, [2]RandSeed, [3]Duration, [4]Waste, [5]Configure, [6]Statistic
    log_list = []
    with open(log_path, 'r') as f:
        reader = csv.reader(f, delimiter=';')
        reader.__next__()
        for row in reader:
            date = datetime.datetime.strptime(row[0], '%Y-%m-%d_%H:%M:%S')
            if date < start_date or date >= end_date or int(row[4]) < 0:
                continue
            instance = row[1][row[1].find('/') + 1:]
            statistic_data = json.loads(row[6])
            log_list.append(Log(instance, float(row[3]),
                    int(row[4]), row[5], statistic_data))
    return log_list

def filter_by_group_dur(logs):
    group_dur_logs = dict()
    for log in logs:
        group, insid, dur = log.instance[0], int(log.instance[1:]), 180
        if log.duration > 180:
            dur = 3600
        if group in group_dur_logs:
            if dur in group_dur_logs[group]:
                if insid in group_dur_logs[group][dur]:
                    if log.waste < group_dur_logs[group][dur][insid].waste:
                        group_dur_logs[group][dur][insid] = log
                else:
                    group_dur_logs[group][dur][insid] = log
            else:
                group_dur_logs[group][dur] = {insid: log}
        else:
           group_dur_logs[group] = {dur: {insid: log}}
    return group_dur_logs

def filter_by_group_dur_cfg(logs):
    group_dur_cfg_logs = dict()
    for log in logs:
        group, insid, dur, cfg = log.instance[0], int(log.instance[1:]), 180, log.configure
        if log.duration > 180:
            dur = 3600
        if group in group_dur_cfg_logs:
            if dur in group_dur_cfg_logs[group]:
                if cfg in group_dur_cfg_logs[group][dur]:
                    if insid in group_dur_cfg_logs[group][dur][cfg]:
                        if log.waste < group_dur_cfg_logs[group][dur][cfg][insid].waste:
                            group_dur_cfg_logs[group][dur][cfg][insid] = log
                    else:
                        group_dur_cfg_logs[group][dur][cfg][insid] = log
                else:
                    group_dur_cfg_logs[group][dur][cfg] = {insid: log}
            else:
                group_dur_cfg_logs[group][dur] = {cfg: {insid: log}}
        else:
            group_dur_cfg_logs[group] = {dur: {cfg: {insid: log}}}
    return group_dur_cfg_logs

def draw_bars(data, x_labels, y_label, title = None, img_path = None):
    ind = numpy.arange(len(x_labels))
    width = 0.8 / len(data)
    fig, ax = plt.subplots(figsize=(10.0, 8.0))
    if title:
        ax.set_title(title)
    count = -0.5 * (len(data) - 1)
    for dur in data:
        ax.bar(ind + count*width, data[dur], width, label = dur)
        count += 1
    ax.set_ylabel(y_label)
    ax.set_xticks(ind)
    ax.set_xticklabels(x_labels)
    ax.legend()
    fig.tight_layout()
    if img_path:
        plt.savefig(img_path)
    else:
        plt.show()

def draw_total_gap_bars(output_dir, logs, json_data):
    group_dur_logs = filter_by_group_dur(logs)
    for group in group_dur_logs:
        title = '与J28结果对比图（算例集' + group + '）'
        img_path = output_dir + '\\' + title + '.png'
        max_index = 15
        if group == 'A':
            max_index = 20
        data = {'180s': [0 for _ in range(max_index)],
                '3600s': [0 for _ in range(max_index)]}
        labels = [' ' for _ in range(max_index)]
        for dur in group_dur_logs[group]:
            for instid in group_dur_logs[group][dur]:
                waste = group_dur_logs[group][dur][instid].waste
                gap = (waste - json_data['J28_sol_' + str(dur)][group + str(instid)]) / waste
                data[str(dur) + 's'][instid - 1] = gap*100
                labels[instid - 1] = group + str(instid)
        if labels.count(' ') < max_index // 3:
            draw_bars(data, labels, 'GAP/%', title, img_path)
            print('保存图片：' + img_path)

def draw_cfg_gap_bars(output_dir, logs, json_data):
    group_dur_cfg_logs = filter_by_group_dur_cfg(logs)
    for group in group_dur_cfg_logs:
        for dur in group_dur_cfg_logs[group]:
            title = '各参数对比（算例集' + group + str(dur) +  's）'
            img_path = output_dir + '\\' + title + '.png'
            max_index = 15
            if group == 'A':
                max_index = 20
            data = dict()
            labels = [' ' for _ in range(max_index)]
            for cfg in group_dur_cfg_logs[group][dur]:
                if cfg not in data:
                    data[cfg] = [0 for _ in range(max_index)]
                for instid in group_dur_cfg_logs[group][dur][cfg]:
                    waste = group_dur_cfg_logs[group][dur][cfg][instid].waste
                    gap = (waste - json_data['J28_sol_' + str(dur)][group + str(instid)]) / waste
                    data[cfg][instid - 1] = gap*100
                    labels[instid - 1] = group + str(instid)
            if labels.count(' ') < max_index // 3:
                draw_bars(data, labels, 'GAP/%', title, img_path)
                print('保存图片：' + img_path)

def run():
    # 加载最优解参数信息。
    json_data = None
    with open(json_info_path, 'r') as f:
        json_data = json.load(f)
    if json_data == None:
        return
    # 输入要分析的程序版本。
    repo = git.Repo("../")
    recent_commit = repo.head.commit
    inp1 = input('请输入起始提交版本：').strip('\n')
    if len(inp1) < 6:
        return
    st_commit = repo.commit(inp1)
    start_date = datetime.datetime.fromtimestamp(st_commit.authored_date)
    end_date = datetime.datetime.now()
    output_dir = img_output_dir + start_date.__format__('%y%m%d_') + inp1[0:6]
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)
    if st_commit != recent_commit:
        inp2 = input('请输入终止提交版本：').strip('\n')
        if inp2 != 'now' and len(inp2) >= 6:
            end_date = datetime.datetime.fromtimestamp(repo.commit(inp2).authored_date)
    # 筛选日志并绘图。
    logs = parse_log_file(start_date, end_date)
    print('共发现满足条件的日志{}条'.format(len(logs)))
    draw_total_gap_bars(output_dir, logs, json_data)
    draw_cfg_gap_bars(output_dir, logs, json_data)

if __name__ == "__main__":
    run()
