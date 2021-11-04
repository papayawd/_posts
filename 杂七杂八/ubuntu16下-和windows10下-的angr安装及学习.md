---
title: ubuntu16下 和windows10下 的angr安装及学习
date: 2020-09-25 00:32:26
tags: re
---

​	<!-- more -->

## ubuntu16

首先是需要python3.6以上的版本 用以下方法更新 （摘抄于https://zhuanlan.zhihu.com/p/51340766）

1.安装编译环境

```
sudo apt-get install zlib1g-dev libbz2-dev libssl-dev libncurses5-dev libsqlite3-dev libreadline-dev tk-dev libgdbm-dev libdb-dev libpcap-dev xz-utils libexpat1-dev liblzma-dev libffi-dev libc6-dev
```

2.下载Python-3.6.3.tar.xz  [国内镜像](http://mirrors.sohu.com/python/3.6.3/)

3.解压

```
tar xvJf Python-3.7.1.tar.xz
```

4.配置安装位置

```
./configure --prefix=/usr/bin/python3.7
```

5.编译及安装

```
sudo make && sudo make install
```

 弄完之后直接

```
pip3 install angr
```

 

今天打比赛 空余时间捣鼓 弄了一整天都有成功把去平坦化弄好呜呜呜 再见angr

## windows10

直接使用python2.7即可

依次执行以下命令（首先需要安装pip

```
pip install --upgrade pip
pip install GitPython
pip install pyvex
pip install unicorn
pip install simuvex
pip install angr  
```

import angr之后会有warning 说明安装成功

###  

## 学习

这里使用了Null战队编写的《从0到1》内容

自己总结到一般来说angr跑题到步骤

1.创建一个project对象 

```
p = Project('r100',auto_load_libs = False)
```

　这里后面追加的 auto_load_libs 是否自动载入依赖库 一般设置为False 以减少angr的工作量

2.设置一个启动状态state

```
p.factory.blank_state(addr=xxx)
#可自定义入口地址  一般我解题使用这个
p.factory.entry_state()
#从程序入口点开始 默认会使用这个
p.factory.full_init_state()
#和entry_一样 不过这之前会调用每个库的初始化函数 会给angr增加工作量
```

3.创建虚拟执行

```
sm = p.factory.simulation_manager(state)
#其中的state就是步骤2里面设置的启动状态
```

4.执行！ 去往想去的地方 规避不想去的地方

```
sm.explore(find = 0x400844,avoid = 0x400855)
#find 和 avoid 可以传数组进来的 如果有多个想去的或者规避的
```

5.输出结果

```
if sm.found[0]:
    print (sm.found[0].posix.dumps(0).replace(b'\x00',b''))
#成功的结果放在sm.found里面  这是个数组 一般我们想要的结果都是唯一解 然后用posix.dumps(0)输出  这里的0是标准输入的意思 replace让结果清晰
```

这是最简单的一个操作 接下来补充一些花里胡哨的操作

- 一些函数对结果没有影响，比如printf 我们可以直接让它返回，代码如下

```
p.hook_symbol('printf',SIM_PROCEDURES['stubs']['ReturnUnconstrained'](),replace = True)
#这里吧需要处理对函数名替换printf就可以了
```

- 既然可以hook我们认为对结果无影响对函数，那么也可以自己写一个函数去hook原本对函数，从而达到给angr减少工作量对目的

```
class my_fgets(SimProcedure): # 固有格式
    def run(self,s): # 参数为 (self + 该函数实际参数)
        simfd = self.state.posix.get_fd(0) # 创建一个标准输入对对象
        data,real_size = simfd.read_data(12) # 注意该函数返回两个值 第一个是读到的数据内容 第二个数内容长度
        self.state.memory.store(s,data) # 将数据保存到相应参数内
        return 12 # 返回原本函数该返回的东西
p.hook_symbol('fgets',my_fgets(),replace = True)
```

如果是写scanf的%d如下

```
class my_sacnf(SimProcedure): # 固有格式
    def run(self,fmt,n): # 参数为 (self + 该函数实际参数)
        simfd = self.state.posix.get_fd(0) # 创建一个标准输入对对象
        data,real_size = simfd.read_data(4) # 注意该函数返回两个值 第一个是读到的数据内容 第二个数内容长度
        self.state.memory.store(n,data) # 将数据保存到相应参数内
        return 1 # 返回原本函数该返回的东西
p.hook_symbol('__isoc99_scanf',my_scanf(),replace = True)
# 这里%d对应int 是4个字节 但是读取到一个int所以返回1  所以这完全是模拟的原来的函数
```

- 一个优化开关，无法避免无解的情况产生，但是能大大提高脚本的运行效率

```
sm.one_active.options.add(options.LAZY_SOLVES)
```

- 自己构造输入

因为使用标准输入经常无法推测输入字符串的长度，会浪费大量时间去尝试不同长度，所以我们可以自定义输入 然后作为参数传入一个函数，这个时候state要设置为call的地址

```
flag_chars = [BVS('flag_%d'%i,32) for i in range(13)] 
# BVS类似于z3中的BitVec，第一个参数为变量名，第二个参数为位数(bit) 这里我们知道输入了13个int 所以申请13个约束变量
for i in range(13):
    state.mem[state.regs.rsp + i * 4].dword = flag_chars[i]
#这里为了方便 先把内容储存在rsp指向的内存 注意一个int是4字节
state.regs.rdi = state.regs.rsp # 然后传参给rdi
```

因为是手动设置的输入，不能通过dump(0) dump标准输入来得到输入，这里使用angr求解器提供的eval函数

```
flag = ''.join(chr(sm.one_found.solver.eval(c)) for c in flag_chars)
# sm.one_found.solver.eval(flag_char[i]) 得到一个int 然后转为char即可
```

-  对于开启了PIE的可执行文件，angr会默认其基地址为0x400000，此时所有操作只需要在原本地址上加上offset即可
- 对内存对储存鱼读取

```
state.memory.store(addr,data)
# 这里对data可以是一串数据 data🉑️来源于simfd.read_data(标准输入)
text = sm.one_found.solver.eval(sm.one_found.memory.load(addr,len),cast_to = bytes)
# sm.one_found.memory.load 加载内存     cast_to = bytes  转为char
```

 