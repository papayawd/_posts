---
title: BUU-Tricky-Part2
date: 2020-10-03 09:26:04
tags: 十月姐姐温柔要求每天一道BUU-re
---

一个简单的反调试

​	<!-- more -->

![](https://i.loli.net/2020/10/03/TjH4QXcgmdRuhJE.png)

main函数里面是这样的 要求一个sha256解密 但是查表之后没找到 

这里直接定位到字符串常量所在的位置

![](https://i.loli.net/2020/10/03/KT7fHWaduXIYo9v.png)

除了main函数里面的常量 还有其他没见过的 尤其是那个Congratzs 交叉引用到另一个函数

![](https://i.loli.net/2020/10/03/3K25R1QmXDSpzuN.png)

这里就是flag