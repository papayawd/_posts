---
title: 'BUU-[FlareOn1]Shellolololol'
date: 2020-09-29 11:09:24
tags: 十月姐姐温柔要求每天一道BUU-re
---

IDA似乎不太好分析。应该有复杂的自解密。用OD打开

​	<!-- more -->

![](https://i.loli.net/2020/09/29/75KnAyqp3NOrt9X.png)

OD断在了这里。  401000就是关键函数

走进去是一些操作 并且跳转到栈上运行 我们单步一直跟

这里把栈的数据用数据窗口显示。我们单步发现里面的数据有变。许多句子显现出来

![](https://i.loli.net/2020/09/29/io8ad72LS9Oj6Rc.jpg)



最后找到了@flare 关键词。找到flag