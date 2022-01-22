# Android shell

## 类加载器

### 一、类加载过程

加载指的是将类的class文件读入到内存，并为之创建一个Class对象。加载的过程有由加载器`ClassLoader`来完成。

当类被加载之后，系统为之生成一个对应的Class对象，接着将会进入连接阶段，连接阶段负责把类的二进制数据合并到 JRE 环境中。分为以下三个部分：

**验证**：文件格式验证、元数据验证、字节码验证、符号引用验证

**准备**：类准备阶段负责为类的静态变量分配内存，并设置默认初始值。

**解析**：将类的二进制数据中的符号引用替换成直接引用。

之后为初始化，是为类的静态变量赋予正确的初始值。

### 二、类加载时机

主要分为两大类：

**一**是在启动的时候，默认会加载一些核心类；

**二**是在主动需求的时候：第一次使用该类的方法、变量、接口，或者是子类被加载的时候

### 三、类加载器的种类

#### 1.BootClassLoader

用来预加载常用类、单例模式，与Java中的`bootstrap class loader`不同，后者是用原生代码来实现的，而`BootClassLoader`是用Java实现的。

#### 2.BaseDexClassLoader

`BaseDexClassLoader`是`PathClassLoader`、`DexClassLoader`、`InMemoryDexClassLoader`三者的父类，其三的主要逻辑都是在`BaseDexClassLoader`中实现的。

#### 3.SecureClassLoader

`SecureClassLoader`继承了抽象类`ClassLoader`，扩展了`ClassLoader`类加入了权限方面的功能，即有了安全性。其子类`URLClassLoader`可以直接从网络 URL 路径获取 .jar 文件来加载类和资源。

#### 4.PathClassLoader

`PathClassLoader`是 Android 默认的类加载器，一个apk中的 Activity 等类就是使用这个加载器加载。

#### 5.DexClassLoader

`DexClassLoader`可以动态加载任意目录下的 `dex / jar / apk / zip` 文件，比`PathClassLoader`更灵活，是实现插件化、热修复、dex加壳的重点。

#### 6.InMemoryDexClassLoader

Android 8.0 新引入了`InMemoryDexClassLoader`，即可以直接从内存中加载dex。

## APP运行流程

**一：BootClassLoader加载系统核心库**

**二：PathClassLoader加载APP自身的dex**

**三：进入APP自身组件开始执行**

**四：调用声明 Application 的 attachBaseContext        （shell重点） **

**五：调用声明 Application 的 onCreate **

## 壳的分类

### 1.第一代壳   dex加密

#### 原理

一共有三个程序： **一** 是被加密的 app ；**二** 是壳程序 shell ，用于解密 dex 并且动态加载被揭密的 dex ；**三** 是整合程序，用于加密 dex 并且整合 shell 与被加密的 app 到一起，打包并且签名。

壳程序shell需要重写`attachBaseContext`函数，`attachBaseContext`中需要解压 apk ，找到被加密的 dex 文件，解密并且插入到`dexElements`数组中去，这样就能和其他 dex 一起被加载。

按道理来说应该还需要把 Activity 修改为解密后的 dex 中的 Activity ，以及环境的修改，但是不清楚怎么操作的，待补。。。

#### 解决方法 

hook 关键函数，dump 内存，修复文件头即可（也可以直接修改关键函数的源码进行dump）

Dalvik 下的关键函数为：`dexFileParse`、`dvmDexFileOpenPartial`。

ART 下的关键函数为：

​		`InMemoryDexClassLoader`中的：`CreateSingleDexFileCookie`、`CreateDexFile`、`DexFile::open`、`OpenCommon`、`DexFile::DexFile`

​		`DexClassLoader`中的：`OpenAndReadMagic`、`DexFile::Common`、`DexFile::DexFile`

### 2.第二代壳   dex抽取与so加固

#### 原理

用于对抗第一代的脱壳技术，将dex的method代码抽取到外部并且加密，在调用其method的时候才使用so解密还原回来。

#### 解决方法

第二代壳在调用了一次method之后，代码就是完整的了。所以直接遍历调用dex中的所有method然后dump内存即可。

### 3.第三代壳   Dex动态解密与so混淆

#### 原理

动态解密就是在第二代壳代码抽取的基础上，动态解密。即用前解密，用后加密。并且对解密文件的so进行了混淆。不论是定位到关键位置，或是直接去解密抽取出来的method，都比较棘手。对抗了之前出现的所有脱壳法。

#### 解决方法

待补。。。大概要么是定位 dump  要么直接硬刚解密so

### 4.arm vmp壳（未来）



# 参考

https://blog.csdn.net/m0_38075425/article/details/81627349
