---
title: 'BUU-re [BSidesCF 2019]Dribbles'
date: 2020-10-22 15:59:30
tags: 十月姐姐温柔要求每天一道BUU-re
---

感觉就是一个pwn题

​	<!-- more -->

![](https://i.loli.net/2020/10/22/DbMVrYaLqTZ1Efh.png)

read_flag吧flag放在了参数的[1]部分，且最后还把地址异或了一下，这个可以dump出来

![](https://i.loli.net/2020/10/22/bPYel3Ajtgf1xBI.png)

而参数的[0]赋值为1 估计是为了方便我们验证我们的定位

![](https://i.loli.net/2020/10/22/9vbnExFUh84d1MB.png)

offer_symbols函数泄漏出了func_name在栈上的地址，求一下偏移就可以定位到main里面异或后 flag指针的地址

查看泄漏的栈地址

![](https://i.loli.net/2020/10/22/oec3JwK8bpVFRAI.png)

根据偏移 找到 +0x120的地方 为last_ebp

![](https://i.loli.net/2020/10/22/Cjle1S2w3ADf6gi.png)

last_ebp网上-8就是该**void指针 继续查询

![](https://i.loli.net/2020/10/22/WuOHcZnqNj217s8.png)

这里的前4字节DWORD为1 说明我们找对地方了，那么后8字节就是加密后的flag地址，解密后查询即可获得flag

![](https://i.loli.net/2020/10/22/DXdmRQJfYWUhNVP.png)

