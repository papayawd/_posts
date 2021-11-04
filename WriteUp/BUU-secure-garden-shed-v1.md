---
title: BUU-secure-garden-shed-v1
date: 2021-01-15 10:59:11
tags: 十月姐姐温柔要求每天一道BUU-re
---

​	<!-- more -->

按照README里面说的，需要把lock.sgsc命令行传参进去

```text
./sgs-exec-release lock.sgsc
```

这时候分析程序，读了这个文件，然后检查了 .code 和 .data是否存在

因为运行后会有错误提示，但是就在可执行文件里边搜不到字符串，猜测字符串在lock.sgsc文件里

动态调试单步执行到lock.sgsc  这时候就能搜索到错误提示，再搜索lock.sgsc 找到得到flag

![](../picture/a.png)











