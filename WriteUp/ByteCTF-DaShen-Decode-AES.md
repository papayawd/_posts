---
title: ByteCTF DaShen Decode AES
date: 2020-11-03 19:54:45
tags: re
---

​	<!-- more -->

没什么好说的，主要是为了放一个frida-hook的实例

反编译后可以得知封装ba的d函数的三个参数分别为 input，key，vi

直接frida hook得到后面两个参数的值

```js
Java.perform(function () {
	var ba = Java.use('ba');
	ba.d.implementation = function (p1, p2, p3) {
		console.log(p1);
		console.log(p2);
		console.log(p3);
	}
});
```

给出了加密后的数据，解AES-CBC即可