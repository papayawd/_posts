---
title: frida js脚本使用方法
date: 2020-10-18 16:14:56
tags: re
---

前几天听了（没有，有事出去了）🐶v0id的frida在线教学，自己也来总结一下具体的使用方法

​	<!-- more -->

首先这里使用的是js脚本，非python或者其他脚本

安装frida使用如下命令

```text
pip install frida-tools
```

该使用管理员/root权限就是用

至于安装多久我就不知道了，反正我在windows下没挂梯子安装了近一个小时

使用某个脚本用一以下命令

```text
frida -n [正在运行文件名] -l [js脚本路径]
比我要用当前目录的a.js脚本 去hook一个正在执行的文件b
frida -n b -l "./a.js"
```

使用js脚本主要的麻烦还是不知道有哪些函数，可以怎么使用，有些时候就在[使用文档](https://frida.re/docs/javascript-api/)里搜索关键词，比如 'alloc'

frida里面最常用的变量是[NativePointer](https://frida.re/docs/javascript-api/#nativepointer) 该变量的很多种运算都有内置函数实现，比如加法使用add()而不能直接使用 '+'

把一个数值申请为一个NativePointer变量使用 ptr() 比如

```javascript
var addr = ptr(0x12345678)
```

接下来的使用方法在frida的应用里面介绍（先给一下我用到的文件.  [ELF文件](https://github.com/papayawd/files_used_in_MyBlog/raw/master/b)     [源代码](https://github.com/papayawd/files_used_in_MyBlog/blob/master/b.c) ）

## 1.dump数值

[Hexdump使用](https://frida.re/docs/javascript-api/#hexdump) 这里就不放代码了 挺简单的

## 2.获取并且修改被hook函数的参数

（相当于动态调试获取某个值，有些时候配合hexdump，可以吧关键数据直接弄出来）

frida一般使用[Interceptor](https://frida.re/docs/javascript-api/#interceptor)模块里面attach的进行hook 结构如下

```js
Interceptor.attach( ptr(hook_addr),
{
    onEnter: function(args) // 这里的angr就是参数数组 即args[0]是第一个参数
    {
//				do something before function begin
    }, // 注意他们之间用逗号分隔开
  	onLeave: function (retval)// 函数的返回值
  	{
//      	do something after function end
//				在这里面可以替换返回值
    }
});
```

hook WindowsAPI 可以使用Module模块的getExportByName  返回值是一个NativePointer 不需要用ptr转一下，比如找一下MessaageBoxA使用方法如下

```js
var Function = Module.getExportByName('user32,dll','MessaageBoxA')
```

这里演示一下如何修改给出文件里的encode函数的参数

```js
Interceptor.attach(ptr(0x4006E7),
    {
        onEnter: function (args)
        {
          	console.log('angr[0] -> ' + args[0]) // 要输出某个信息到frida
            var str = Memory.allocUtf8String("654322");
           	args[0] = str; // 可以直接修改
        }
    }
);
```

其中使用Memory的allocUtf8String模块 申请一个字符串，返回类型也是NativePointer

如果"654322"是正确的输入 那么程序最终就会输出ok

## 3.调用某个存在的函数

（可用来爆破某个加密/解密算法）

这里直接上脚本

```js
var number = 100000;
Interceptor.attach(ptr(0x4006E7),
    {
        onLeave: function (retval)
        {
            console.log('hook begin');
            for(;number <= 999999;number++)
            {
                if(number % 100000 == 0)
                {
                    console.log(number);
                }    
                var Function = new NativeFunction(ptr(0x4006E7),'bool',['pointer']);
                var str = Memory.allocUtf8String(number.toString());
                var k = Function(str);
                if (k == 0)
                {
                    console.log('key : ' + number);
                    break;
                }
            }
        }
    }
);
```

因为有提到输入是六位数，直接爆破在时间方面是有可行度的

既然是爆破 就需要反复调用某个函数 这里使用 NativeFunction 使用规则如下

```js
var Function = new NativeFunction(ptr(func_addr),'ret_type',['args_array'])
//参数依次是 调用函数地址，返回类型，参数数组
//经过试验 NativeFunction申请的函数不能被hook 具体原因未知（上过当
```

## 4. Android Hook（MuMu虚拟机）mac物理机

学习借鉴博客：https://www.jianshu.com/p/7be526b77bd2     https://www.jianshu.com/p/d4a44f803f33

### 1.安装工具

```python
brew cask install android-platform-tools # 安装adb
```

​		打开虚拟机之后

```python
adb shell getprop ro.product.cpu.abi # 获取虚拟机处理器型号
frida --version # 获取frida的版本
```

​		然后去[github官网](https://github.com/frida/frida/releases)找对应的版本，对应安卓处理器型号的frida-server

### 2.运行frida-server

​		解压以后修改一下文件名为frida-server 然后通过adb推送给虚拟机

```python
adb push frida-server /data/local/tmp/ #推送server给虚拟机的这个路径下
adb shell # 进入虚拟机的shell
cd /data/local/tmp/ #进入推送目录
chmod 777 frida-server #给执行权限
./frida-server #执行
```

​		然后这个命令行窗口就一直挂着就行了

### 	3.启动程序并且hook

```python
frida -U -l [js脚本路径] --no-pause -f [程序包名称]
adb shell pm list package #所有程序包的名称，可以过滤一下
```

### 	4.脚本编写

​	所有的代码都要封装在

```js
Java.perform(function(){
  // do something
});
```

​	如果要获取一个JavaScript wrapper（封装）使用

```js
var className = Java.use([className])
```

​	然后继续，如果要hook这个封装里面的某个函数 使用

```js
className.[funcName].implementation = function(arg1, ... ){
  // do something
} 
//这里的参数根据原函数填充
```

​	如果改封装有重名函数（重载） 这样使用

Android代码

```js
public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        findViewById(R.id.mBtnTest).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View v) {
                helloAndroid();
                test1();
                test2(123);
                test3("str");
                test4("str", true);
            }
        });
    }
    
    private void helloAndroid() {
        System.out.println("helloAndroid()");
    }

    private void test() {
        System.out.println("test1()");
    }

    private void test(int i) {
        System.out.println("test2(int) " + i);
    }

    private void test(String s) {
        System.out.println("test3(String) " + s);
    }

    private void test(String s, boolean b) {
        System.out.println("test4(String, boolean) " + s + ", " + b);
    }
}

```

JS Hook代码

```js
Java.perform(function () {
    var MainActivity = Java.use("com.github.fridademo.MainActivity");
    MainActivity.helloAndroid.implementation = function () {
        console.log("helloAndroid()");
        this.private_func();
    };
    MainActivity.test.overload().implementation = function () {
        console.log("test1()");
        this.private_func();
    };
    MainActivity.test.overload("int").implementation = function (i) {
        console.log("test2(int): " + i);
        this.private_func(i);
    };
    MainActivity.test.overload("java.lang.String").implementation = function () {
        console.log("test3(String): " + arguments[0]);
        this.private_func(arguments[0]);
    };
    MainActivity.test.overload("java.lang.String", "boolean").implementation = function (s, b) {
        console.log("test4(String,boolean): " + s + ", " + b);
        this.private_func(s, b);
    };
});
```

（上两个代码片段均摘抄自借段首提到的 借鉴学习的博客 ）

​		可以看出规律，使用overload(typename, ... )可以定位到想要hook的函数

​	实战例子见该博客写的ByteCTF-DaShen-Decode-AES

​		

​		



