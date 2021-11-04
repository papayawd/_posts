---
title: 'BUU-[FlareOn2]sender'
date: 2020-10-21 19:23:18
tags: 十月姐姐温柔要求每天一道BUU-re
---

简单的换表base64加密

​	<!-- more -->

![](https://i.loli.net/2020/10/21/pktgLlqDcAJVsIx.png)

分析猜测key.txt为输入，250函数里面🈶️加法运算，这里不过多赘述

关键是2A0函数，进入查看

![](https://i.loli.net/2020/10/21/KGpvbnIV1TC3Zko.png)

这里红框的种种操作，我们猜测该加密算法为base64 ，分析知，table应该在410EB8 进入看看

![](https://i.loli.net/2020/10/21/G4Kxdm6kgXZlIpc.png)

Table大小写替换

000函数是一个Internet交互函数 重点为红框内的请求文发送，每四个字节的base64编码发送一次

![](https://i.loli.net/2020/10/22/PGxvQM3FyOCnHc9.png)

我们直接打开流量包查看，找到明文

![](https://i.loli.net/2020/10/22/AlCKyh4EYe8tHim.jpg)

```python
import base64

s = 'UDYs1D7bNmdE1o3g5ms1V6RrYCVvODJF1DpxKTxAJ9xuZW=='
k = ''
for i in range(len(s)):
    if ord('A') <= ord(s[i]) and ord(s[i]) <= ord('Z'):
        k += chr(ord(s[i]) + 32)
    elif ord('a') <= ord(s[i]) and ord(s[i]) <= ord('z'):
        k += chr(ord(s[i]) - 32)
    else:
        k += s[i]
print k
s = base64.b64decode(k)
k = 'flarebearstare'
flag = ''
for i in range(len(s)):
    flag += chr(ord(s[i]) - ord(k[i%len(k)]))
print 'flag{' + flag + '}'
```

