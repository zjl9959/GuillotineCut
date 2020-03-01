#!/usr/bin/python3

import os
import csv
import json
import random
import datetime
import matplotlib
import matplotlib.pyplot as plt


plt.rcParams['font.sans-serif']=['SimHei']  #用来正常显示中文标签
plt.rcParams['axes.unicode_minus']=False    #用来正常显示负号

json_info_path = 'INFO.json'
statistic_dir = '../Deploy/Statistic/'
img_output_dir = 'C:\\毕业论文\\我的论文\\数据图表\\统计分析\\'
date_time = datetime.datetime.now().__format__('%y%m%d%H%M%S')

with open(json_info_path, 'r') as f:
    json_data = json.load(f)

class Statistic:
    def __init__(self, time, usage_rates):
        self.time = time / 1000
        self.usage_rates = usage_rates
        self.average_usage_rate = sum(usage_rates) / len(usage_rates)

def load_statistic(path):
    res = list()
    with open(path, 'r') as f:
        reader = csv.reader(f, delimiter=',')
        for col in reader:
            time = float(col[0])
            temp_list = list()
            for i in range(1, len(col)):
                temp_list.append(float(col[i])*100)
            res.append(Statistic(time, temp_list))
    return res

def draw_lines(datas, labels, title, path):
    fig, ax = plt.subplots()
    ax.set_title(title)
    for i in range(len(datas)):
        x = range(0, len(datas[i]))
        ax.plot(x, datas[i], labels=labels[i])
    ax.legend()
    plt.save(path)

def filter_file_name(inst):
    res = list()
    for file_name in os.listdir(statistic_dir):
        if inst in file_name:
            res.append(file_name)
    return res

def draw_total_waste(datas, inst):
    x, y = list(), list()
    for data in datas:
        x.append(data.time)
        y.append(data.average_usage_rate)
    title = '整体利用率随时间变化（算例' + inst + '）'
    path  = img_output_dir + title + date_time + '.png'
    fig, ax = plt.subplots()
    ax.set_title(title)
    ax.plot(x, y)
    ax.set_xlabel('时间/秒')
    ax.set_ylabel('利用率/%')
    plt.savefig(path)
    print('保存图片到：', path)

def run():
    inp = input('请输入算例名：').strip('\n')
    names = filter_file_name(inp)
    inp2 = '0'
    if len(names) > 1:
        for i in range(len(names)):
            print(i, ':', names[i])
        inp2 = input('请选择算例索引：').strip('\n')
    datas = load_statistic(statistic_dir + names[int(inp2)])
    draw_total_waste(datas, inp)

if __name__ == "__main__":
    run()
