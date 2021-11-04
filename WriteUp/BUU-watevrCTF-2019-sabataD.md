---
title: 'BUU-[watevrCTF 2019]sabataD'
date: 2020-10-22 13:39:29
tags: 十月姐姐温柔要求每天一道BUU-re
---

主要是构造正确的payload

​	<!-- more -->

![](https://i.loli.net/2020/10/22/e5Qbpz8gDTa1MZJ.jpg)

图里解释的很清楚了 这里说一下程序流程

首先把输入通过函数AEA处理一下，然后按照下标对3对模，分组，组合成新的字符串

模为0对部分字符串为'Fetch from file with index'

模为1对部分字符串为'watevr-admin'

模为2对部分字符串不能为'/home/ctf/flag.txt'

这里虽然说不能为这个路径，但是flag应该是在这个路径下，这里用多个/绕过

即字符串可以为 '/home/ctf///////flag.txt'

nptr在输入的151～153位的时候成为下面某个for的循环次数，这里控制为较小的数，比如0 

满足所有条件后，该程序就会把该路径下的文件printf出来 即为flag

```python
from pwn import *


def work(x):
    if ord('A') <= ord(x) and ord(x) <= ord('Z'):
        return chr(ord('A') + ((ord(x) - ord('A') - 13) % 26))
    if ord('a') <= ord(x) and ord(x) <= ord('z'):
        return chr(ord('a') + ((ord(x) - ord('a') - 13) % 26))
    return x


#io = process('./service')
io = remote('node3.buuoj.cn', 25002)

payload = ''

s0 = 'Fetch from file with index'
s1 = 'watevr-admin'
s2 = '/home/ctf///////flag.txt'
s0 = s0.ljust(30, '\x00')
s1 = s1.ljust(30, '\x00')
s2 = s2.ljust(30, '\x00')
for i in range(30):
    payload += work(s0[i])
    payload += work(s1[i])
    payload += work(s2[i])
payload = payload.ljust(150, '\x00')
payload += '000'
sleep(0.5)
io.sendline(payload)
io.interactive()
```

