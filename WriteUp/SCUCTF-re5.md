---
title: SCUCTF-re5
date: 2020-09-26 01:01:52
tags: re
---

~~不清楚为什么这文件IDA不能远程windows调试，OD和IDA之间切换好麻烦~~

​	<!-- more -->

有两次输入  首先尝试修改第一次输入且不修改第二次输入  得到的结果是一样的  说明第一次输入对结果无影响  只分析第二次输入干了什么就可以了

![](https://i.loli.net/2020/09/26/t8UAVONbGsXgYxj.png)

7CE是一个base64加密 其中表是被换过的 直接动态dump出新表

![](https://i.loli.net/2020/09/26/QArjvxgPLXNpDmE.png)

47C是一个随机生成文件名的函数 不过多分析

重点是242函数 跟进去看看

![](https://i.loli.net/2020/09/26/YCQL3F41AHxhKku.png)

432里面对table前四位做了修改

![](https://i.loli.net/2020/09/26/fBrDOUHMzcmtypu.png)

314里面循环了256次 很像是rc4构造密匙流 看参数这里的密匙是table被修改后的前四位 储存在Dst中

rc4加密最后步骤是明文和密匙流异或 而只有3A0里面有异或 跟进去看看

![](https://i.loli.net/2020/09/26/BPOCGul8X5cWqvt.png)

明文没有简单的直接去异或密匙流，但是异或的值由密匙流得到且与明文无关 如图所说我们的密匙是固定的 所有之后的所有值 直到最终异或的值都是固定的 可以动态dump一下

![](https://i.loli.net/2020/09/26/bkt4iy6PuWOHRnl.jpg)

我们先换表解密一下给出的加密字符串 得到长度为29 我们就在这里断29次 记录低8位的值 记作数组number

解密的大致流程就是

1.base64换表解密得到串s

2.s异或dump出来的number数组 得到flag

```python
import string
import base64

table1 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
table2 = ''
for i in range(26):
    table2 += chr(ord('A') + i)
    table2 += chr(ord('a') + i)
for i in range(10):
    table2 += chr(ord('0') + i)
table2 += '/'
table2 += '+'
print(table2)
s = 'T2aNoifxUjIgKYMxKCsHKxAUZsDDGStqwV8Gh3W='
s = base64.b64decode(s.translate(str.maketrans(table2,table1)))
print (s)
print (len(s)) # len = 29
a = [0xc8,0x23,0x0f,0x36,0x46,0xa9,0xda,0x76,0x4d,0x20,0x35,0x61,0x0f,0x20,0x3d,0x0d,0xb9,0x46,0xbe,0x62,0xf4,0x57,0x3a,0x95,0xdf,0xc1,0x6b,0x60,0x06]
flag = ''
for i in range(29):
    flag += chr(s[i] ^ a[i])
print (flag)

```

