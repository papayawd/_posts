---
title: IDA-Python常用函数
date: 2020-10-28 15:00:06
tags: re
toc: true
---

​	<!-- more -->

```python
import idc		#导入头文件
```

以下按功能分类吧

## 关于 光标/选择

```python
idc.ScreenEA()
here()	#两个函数都获取当前光标所在行的地址

idc.SelStart()
idc.SelEnd()	#获取选取部分代码的首尾地址
#注意SelEnd得到的是选择结束的下一个地址，可以用PrevHead调整一下end
```

## 反汇编

```python
idc.GetDisasm(ea)	#获取反汇编
idc.PrevHead(ea)	#获取该反汇编语句的 上一个反汇编语句的首地址
idc.NextHead(ea)	#获取该反汇编语句的 下一个反汇编语句的首地址
```

## 数据类型---获取/patch

```python
idc.Byte(ea)
idc.Word(ea)
idc.Dword(ea)
idc.Qword(ea)
idc.GetFloat(ea)
idc.GetDouble(ea)	#以上函数按照数据类型 获取数据(因为不同数据的储存方式不同)
idc.PatchByte(ea, value)
idc.PatchWord(ea, value)
idc.PatchDword(ea, value)	#以上函数按照数据类型 从ea开始 patch数据为value
```

## 查询Search/Find

```python
idc.FindBinary(ea, flag, searchstr)	#查询一个Binary串
#参数依次为：起始地址，查询模式，Binary串
#常用的flag属性有：SEARCH_DOWN，SEARCH_UP，SEARCH_NEXT
#不同类型的属性可以用｜表示共用，具体的flags可以查询IDAPython-Book

idc.FindText(ea, flag, 0, 0, searchstr)	#查询文本串，flag参数与FindBinary相同
#亲测，FindText无法使用SEARCH_NEXT，所以使用NextHead跳转到下一个，避免重复搜索

idc.FindCode(ea, flag)	#ea算作起始地址，开始寻找第一个code，flag与上相同
```

## Get套餐

```python
idc.SegName(ea)	#获取该地址所在的段名称
idc.GetDisasm(ea)	#获取反汇编
idc.GetMnem(ea)	#获取反汇编语句的类型  eg：mov
idc.GetOpnd(ea,0)	#获取该汇编指令的第一个参数
idc.GetOpnd(ea,1)	#获取该汇编指令的第二个参数
```

## 函数

```python
idautils.Functions([st],[ed])	#st与ed参数不存在时，获取所有函数的首地址，返回一个list
#st与ed参数存在时，获取该范围内所有函数的首地址

idc.GetFunctionName(ea)	#获取ea所在函数的函数名
idaapi.get_func(ea)	#返回ea所在的函数(class类型)，类似一个句柄
dir(class)	#用于获取上一个函数返回类型里面，可用的函数
idc.NextFunction(ea)	#获取ea所在函数的下一个函数的首地址
idc.PrevFunction(ea)	#获取ea所在函数的上一个函数的首地址
idc.GetFunctionFlags(ea)	#获取该函数的属性
#可以查询IDAPython-Book了解其属性
idautils.FuncItems(ea)	#返回值为某个生成器
#可以使用list(ret_val)得到ea所在函数内，所有反汇编语句的首地址

```

## 交叉引用

```python
idautils.CodeRefsTo(ea)	#交叉引用，返回调用ea的地方 相当于Ctrl+X
idautisl.CodeRefsFrom(ea)	#交叉引用，返回被ea调用的地方 也是双击跳转的位置
```

## API

```python
idc.LocByName(str)	#通过API函数名，获取函数首地址
Names()	#获取所有API的地址和Name (addr,str_name)
idautils.DataRefsTo(ea)	#数据的交叉引用 Ctrl+X
idautils.DataRefsFrom(ea)	#数据的交叉引用 ，双击跳转的位置
```

