# bedb85 Bug Fix

2019.10.28

## 问题分析

![img](Imgs/bedb85_B5.jpg)

物品(id:24)放在四层桶中时，与瑕疵冲突。

## bug位置

CutSearch.cpp::line228::sliptoDefectRight(old.c3cp - item.w, old.c4cp, item.w, item.h) != old.c3cp - item.w 中的!=换为==。