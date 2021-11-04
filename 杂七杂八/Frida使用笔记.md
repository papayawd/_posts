---
title: Frida使用笔记
date: 2021-10-08 15:04:56
tags: re
---

​	<!-- more -->

```
https://www.jianshu.com/p/ca8381d3e094
```

[TOC]

**环境：windows + 网易mumu模拟器**

## Android Studio  与  adb 安装及连接

去[官网下载](https://developer.android.google.cn/studio)windows平台的Android Studio 安装之后打开Android Studio在设置中可以找到SDK的位置，其\platform-tools目录下就有adb.exe 只需要将该目录路径添加到PATH环境变量中就可以在任意目录中使用adb

网易mumu的固定端口为7555  有以下指令可能会用到

```python
adb connect 127.0.0.1:7555 #  连接模拟器 连接上之后 Android Studio自动会识别到
adb shell # 进入shell
adb shell getprop ro.product.cpu.abi # 查看CPU类型（一般和主机CPU一样）
adb forward tcp:27042 tcp:27042 # 转发android TCP端口到本地
adb forward tcp:27043 tcp:27043 # 转发android TCP端口到本地
frida-ps -U # 查看当前正在运行的进程ID与进程名 如果有回显说明连接成功
```

## Frida  与  Frida-server

直接使用下面的命令行指令安装frida

```
pip install frida-tools #安装frida
frida --version # 查看frida的版本号
```

然后去[GitHub frida release](https://github.com/frida/frida/releases)里面查找对应版本、对应模拟器平台的frida-server 然后在该文件路径下打开新的终端 

```python
adb push frida-server /data/local/tmp/ #推送server给虚拟机的这个路径下
adb shell # 进入虚拟机的shell
cd /data/local/tmp/ #进入推送目录
chmod 777 frida-server #给执行权限
./frida-server #执行   执行后该终端要一直挂着
```

## Java层被动Hook

```java
// Android code
package com.example.demo1;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        while(true){
            try{
                Thread.sleep(1000);
            } catch (InterruptedException e){
                e.printStackTrace();
            }
            fun(50,30);
        }
    }
    void fun(int x,int y){
        Log.d("Demo1.sum",String.valueOf(x+y));
    }
}
```

```js
// frida code
function main(){
    Java.perform(function(){
        console.log('hello world');
        var MainActivity = Java.use('com.example.demo1.MainActivity');
        // var className = Java.use([className])  获取某个类的封装 
        // className.[funcName].implementation = function(arg1, ... ){}  以这种格式被动HOOK该函数 
        MainActivity.fun.implementation = function(x,y){
            console.log('x => ',x,'y => ',y);
            var ret_value = this.fun(1,2);
            return ret_value;
        }
    })
}
setTimeout(main);

```

### Overload问题

```java
// Android code
package com.example.demo1;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        while(true){
            try{
                Thread.sleep(1000);
            } catch (InterruptedException e){
                e.printStackTrace();
            }
            fun(50,30);
            fun("hello","world");
        }
    }
    void fun(int x,int y){
        Log.d("Demo1.sum",String.valueOf(x+y));
    }
    void fun(String a,String b){
        Log.d("Demo1.String",a+b);
    }
}
```

```js
// frida code
function main(){
    Java.perform(function(){
        console.log('hello world');
        var MainActivity = Java.use('com.example.demo1.MainActivity');
        MainActivity.fun.overload('int','int').implementation = function(x,y){
            // 在函数名后面加.overload([参数类型])  即可实现对于overload的函数的被动Hook
            console.log('x => ',x,'y => ',y);
            var ret_value = this.fun(1,2);
            return ret_value;
        }
        MainActivity.fun.overload('java.lang.String','java.lang.String').implementation = function(x,y){
            console.log('a => ',x,'b => ',y);
            var ret_value = this.fun('world',' hello');
            return ret_value;
        }
    })
}
setTimeout(main);

```

## Java层主动调用

Java层的函数分为两种：类函数与实例方法。类函数使用关键字static修饰过的静态函数，可以直接通过类去调用；而实例方法则没有关键字static修饰，需要先创建实例再去调用。对于frida来说，对于类函数使用Java.use()找到类直接调用即可，对于实例方法使用Java.choose()，这个函数可以在Java的堆中寻找指定类的实例，调用也稍微复杂但都是一个框架了。

```java
// Android code
package com.example.demo1;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
public class MainActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        while(true){
            try{
                Thread.sleep(1000);
            } catch (InterruptedException e){
                e.printStackTrace();
            }
            fun(50,30);
        }
    }
    void fun(int x,int y){
        Log.d("Demo1.sum",String.valueOf(x+y));
    }
    void fun(String a,String b){
        Log.d("Demo1.String",a+b);
    }
    void secret(){
        Log.d("Demo1.secret","this is secret func");
    }
    static void staticsecret(){
        Log.d("Demo1.staticsecret","this is static secret func");
    }

}
```

``` js
// frida code
function main(){
    Java.perform(function(){
        console.log('hello world')
        var MainActivity = Java.use('com.example.demo1.MainActivity')
        MainActivity.staticsecret() // Java.use 找到后直接调用

        Java.choose('com.example.demo1.MainActivity', {  // Java.choose 的固定框架
            onMatch: function(instance){
                console.log('instance found',instance)
                instance.secret()  // 这里调用
            },
            onComplete: function(){
                console.log('search Complete')
            }
        })
        
    })
}
setTimeout(main);

```

## native层Hook

这里使用的函数也是HOOK windwos平台下exe可执行文件的函数：Interceptor.attach()  HOOK模板如下

```js
Interceptor.attach(addr,{
    onEnter(args){
        // do something
    },
    onLeave(retval){
        // do something
    }
})
```

因为每次程序加载的地址都不一样，这里使用base+offset的方式定位要hook的函数

使用objection的功能

```python
frida-ps -U # 获取进程名
objection -g [进程名] explore  # objection注入
memory list modules # 查看所有的模块  找到libnative.so 并确定其偏移 （有些是lib+进程名+so)
			# libdemo2.so                     0x7fa4045f5000  241664 (236.0 KiB)    
memory list exports [模块名] # 查看此模块所有的符号
			# function  Java_com_example_demo2_MainActivity_stringFromJNI  0x7fa404604140
			# 计算出offset = 0x7fa404604140 - 0x7fa4045f5000
```



```java
// Android MainActivity code
package com.example.demo2;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import com.example.demo2.databinding.ActivityMainBinding;
public class MainActivity extends AppCompatActivity {
    // Used to load the 'demo2' library on application startup.
    static {
        System.loadLibrary("demo2");
    }
    private ActivityMainBinding binding;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
        while(true){
            try{
                Thread.sleep(1000);
            } catch (InterruptedException e){
                e.printStackTrace();
            }
            Log.d("Demo2.JNI ",stringFromJNI());
        }
    }
    /**
     * A native method that is implemented by the 'demo2' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
```

```c++
// Android native code
#include <jni.h>
#include <string>
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_demo2_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
```

```js
// frida code
function main(){
    var base = Module.findBaseAddress('libdemo2.so')
    console.log('libdemo2.so address is => ',base)
    var stringFromJNI = base.add(0x7fa404604140 - 0x7fa4045f5000)
    // 如果是导出函数  可以直接使用Module.getExportByName('libdemo2.so','Java_com_example_demo2_MainActivity_stringFromJNI') 来获取函数地址，但如果没有导出的话就只能用这种方法了
    Interceptor.attach(stringFromJNI,{
        onEnter: function(args){
            console.log('jnienc pointer => ',args[0])
            console.log('jobj pointer => ',args[1])
        },
        onLeave: function(retval){
            console.log('retval is ',Java.vm.getEnv().getStringUtfChars(retval,null).readCString())
            console.log('============')
        }
    })
}
setTimeout(main);

```

