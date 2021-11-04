---
title: BUU-Memecat Battlestation
date: 2020-09-26 15:30:15
tags: 十月姐姐温柔要求每天一道BUU-re
---

一个64位的.net平台上的可执行文件  这个游戏还是很有趣的  激光打怪兽？？？

​	<!-- more-->

游戏关卡有两个 第一个是明文比较

![](https://i.loli.net/2020/09/26/hHGZAn4U3Ez6LyM.png)

得到串 RAINBOW

第二个是异或比较（都好zz 为什么做这个题的人这么少呢

![](https://i.loli.net/2020/09/26/zRFBwuGOMEAh6yV.png)

因为他给的那个反汇编我不会弄出比较的值

这里直接在最后比较函数那里下端点，dump出最后比较的值

```python
a = [0x3,0x20,0x26,0x24,0x2d,0x1e,0x2,0x20,0x2f,0x2f,0x2e,0x2f]
flag = ''
for i in range(len(a)):
    flag += chr(a[i] ^ ord('A'))
print flag
```

得到串 Bagel_Cannon

闯关成功就得到flag了

![](https://i.loli.net/2020/09/26/HREMxLkXIJjgsPr.jpg)

