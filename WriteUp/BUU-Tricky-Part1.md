---
title: BUU-Tricky-Part1
date: 2020-09-28 19:49:08
tags: 十月姐姐温柔要求每天一道BUU-re
---

过于简单 出得太快 我都没有来得及抓住爱情

​	<!--more-->

![](https://i.loli.net/2020/09/28/FVtJBzyq6O29uWn.png)

程序流程很清晰 先输入 然后stack_check函数构造flag 最后比较      进入stack_check

![](https://i.loli.net/2020/09/28/4BMg6dFv83nNWQS.png)

这里吧unk_4011D8放入v8  然后循环异或base里面的内容      第一个箭头指向的[]符号是c++反编译出来的数组取值的符号  这里的源码应该是

```c++
v1 = v8[v1 % v2]
```

base在bss段   交叉引用找到初始化地方

![](https://i.loli.net/2020/09/28/ZYOVvGI8fnFSswJ.png)

然后写脚本直接跑出flag

```python
f = open('so.in')
a = []
s = f.readline()
for j in range(8):
    a.append(int(s[j*3:j*3+2],16))
for i in range(4):
    s = f.readline()
    for j in range(16):
        a.append(int(s[j*3:j*3+2],16))
print a
b = [0x47,0x44,0x42]
s = ''
for i in range(len(a)):
    a[i] ^= b[i % 3]
    s += chr(a[i])
print a
print s





so.in
0E 0A 11 06 3F 01 1F 1C
1D 76 37 1D 2F 70 30 23 77 30 18 22 72 35 1B 31
33 70 36 76 27 1D 73 2A 76 2B 75 31 3E 37 1D 30
2C 71 29 1B 26 74 26 37 20 23 71 35 1B 24 73 75
2E 34 39 00 00 00 00 00 00 00 00 00 00 00 00 00
```

