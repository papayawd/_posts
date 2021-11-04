---
title: re-ncov
date: 2020-09-26 20:49:19
tags: re
---

一道迷宫题目

​	<!-- more -->

[exe文件](https://github.com/papayawd/files_used_in_MyBlog/raw/master/ncov.exe)

[idb分析文件](https://github.com/papayawd/files_used_in_MyBlog/raw/master/ncov.idb)

这题思路比较清晰，flag由四个‘-’分割成5个部分

第一部分为纯数字，这决定了接下来使用哪一个迷宫

剩下的四个部分为走迷宫的方案

因为有控制剩下四个部分的方案长度 且长度各不相同 所以4个迷宫一定也对应着这四个长度不相同的方案

把迷宫dump出来如下

```python
'''
|||||||||||||||
|||||||||||||||
|||||||||||||||
||s.........|||
|||||||||||.|||
||d||||||||.|||
||.||||||||.|||
||.||||||||.|||
||..........|||
|||||||||||||||
dddddddddsssssaaaaaaaaawww

|||||||||||||||||||
||s|||||||||||||d||
||..|||||||||||..||
|||..|||||||||..|||
||||..|||||||..||||
|||||..|||||..|||||
||||||..|||..||||||
|||||||..|..|||||||
||||||||...||||||||
|||||||||||||||||||
sdsdsdsdsdsdsddwdwdwdwdwdwdw

|||||||||||||||
||.........s|||
||.||||||||||||
||.||||||||||||
||.||||||||||||
||.||||||||||||
||.||||||||||||
||.||||||||||||
||.........d|||
|||||||||||||||
|||||||||||||||
aaaaaaaaasssssssddddddddd

|||||||||||||||
|||||||||||||||
|||..........||
|||.||||||||.||
|||.||||||||.||
|||.||||||||.||
|||.||||||||.||
|||s||||||||d||
|||||||||||||||
wwwwwdddddddddsssss

'''
```

果然每个迷宫对应的方案不同，然后按照相应的顺序 加上‘-’符号 构成flag

```python
s1 = 'dddddddddsssssaaaaaaaaawww'
s2 = 'sdsdsdsdsdsdsddwdwdwdwdwdwdw'
s3 = 'aaaaaaaaasssssssddddddddd'
s4 = 'wwwwwdddddddddsssss'
print(hex(len(s1)))
print(hex(len(s2)))
print(hex(len(s3)))
print(hex(len(s4)))
print('4312' + '-' + s4 + '-' + s3 + '-' + s1 + '-' + s2)

```

