---
title: 'BUU-[RCTF2019]DontEatMe'
date: 2020-09-25 15:52:54
tags: 十月姐姐温柔要求每天一道BUU-re
---

看下面的wsad  和那个判断  应该是一个迷宫

​	<!-- more -->

前边有固定随机种子生成固定的8个数字。然后第一个数字赋值为0

之后代码内容过于复杂。怀疑是某个加密或者解密

![](https://i.loli.net/2020/09/25/6Rdy51aUADiHsIv.jpg)

这里还🈶️对输入16进制转换  

百度一下blowfish加密过程

![](https://i.loli.net/2020/09/25/vLEKcOsYQMJU7Tb.jpg)

64位就是8个字符  推测最后的走迷宫的方案长度是8的倍数

存在反调试  我直接断在调试前  然后修改EIP到调试后

![](https://i.loli.net/2020/09/25/cihs4R17FOTA8ng.jpg)

然后我断在了switch  这时候所有的初始化都弄完了  可以直接dump迷宫和密匙

![](https://i.loli.net/2020/09/25/XqoOD8LAEYcvdVx.jpg)

![](https://i.loli.net/2020/09/25/cihs4R17FOTA8ng.jpg)

迷宫如下

```reStructuredText
000000000111111
011111110111111
011111110111111
01111000e000111
011110111110111
011110111110111
011110000110111
011111110110111
011111110110111
0000s0000110111
111101111110111
111100000000111
111111111111111
111111111111111
111111111111111
```

由反汇编可知 出发点为 （10，5） 目的地为（4，9）需要在16步以内走完(估计就是16步  手动走一下得到路径

```reStructuredText
ddddwwwaaawwwddd
```

对应前面的加密，刚好16位是8的倍数 不用padding分组blowfish加密  加密后转hex屏凑得到flag

```py
import blowfish
slover = blowfish.Cipher(b"\x00\x0F\x1A\x01\x35\x3A\x3B\x20")
s = slover.encrypt_block(b'ddddwwwa') + slover.encrypt_block(b'aawwwddd')
flag = ''
for i in s:
    flag += hex(i)[2:]
print(flag)
```

