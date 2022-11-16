# LLVM学习

## LLVM编译以及CLion调试

### 编译

下载project.src源码，使用Clion打开llvm目录下的CMakeLists.txt，然后cmake选项里面的参数如下填写

![](./picture/截屏2022-02-1321.17.19.png)

```shell
-G Ninja -DLLVM_ENABLE_PROJECTS="clang;lldb;libcxx;libcxxabi" # 工具之间用 ; 隔开
```

**NOTE:  编译lldb需要libcxx，而libcxx需要libcxxabi**

这里使用Ninja编译，因为用的Clion，里面已经制定了编译版本，否则默认编译Debug版本，需要用`-DMAKE_BUILD_TYPE=Release`来制定编译Release版本。`-DLLVM_ENABLE_PROJECTS`则是制定编译那些工具，工具之间用分号隔开即可。

一切修改好保存之后，Clion回自动重新生成cmake-build-degub等文件夹，这时候就不用Clion，直接终端用Ninja编译即可。

ubuntu下编译需要有[足够的swap space](https://my.oschina.net/u/4266515/blog/3329968)，或者线程弄少一点（最好是1）

不清楚为什么生成的clang没有SDK，需要手动指定，这里使用xcode的，路径一般固定

```shell
-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
```

llvm编译过程中也有中间代码，分别是ll与bc文件

```shell
clang hello.c -emit-llvm -S -o hello.ll # 仍然要加-isysroot 
lli ./hello.ll             # lli 为ll文件的解释器
# ll相当于是中间语言 与平台无关 
```

```shell
llvm-as hello.ll -o hello.bc   # 仍然要加-isysroot 
lli ./hello.bc              # lli 也是bc文件的解释器
```

```shell
clang hello.bc -o hello     # 最后用clang就得到最终的可执行文件  仍然要加-isysroot 
```

### 调试

需要在文件`/llvm/clang/tool/driver/driver.cpp`中main下断点，选择degub版本下的clang进行调试，参数则是填写正常编译需要的参数，比如

```shell
-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk /Users/liuxingyu/Desktop/vscode/android/a.c -o /Users/liuxingyu/Desktop/vscode/android/a_clion # 最前面的clang需要省略
```

![](./picture/截屏2022-02-1412.58.54.png)

之后点击调试就可以断下来了，效果如下

![](./picture/MacPic2022-02-1413.04.25.png)



## LLVM PASS

### 内部编译pass

这是llvm12.0.0写pass的官方[教程](https://releases.llvm.org/12.0.0/docs/WritingAnLLVMPass.html#quick-start-writing-hello-world)

这里新创建一个叫做EncoderFunctionName的模块，文件内容为

```cmake
add_llvm_library( LLVMEncoderFunctionName MODULE EncoderFunctionName.cpp PLUGIN_TOOL opt)
#模块中的CmakeLists.txt    LLVMEncoderFunctionName为模块名称
```

```cmake
add_subdirectory(EncoderFunctionName) # Transforms目录下的CmakeLists.txt 添加的
```

```c++
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
namespace {
    struct EncoderFunctionName : public FunctionPass {
        static char ID;
      
        EncoderFunctionName() : FunctionPass(ID) {}
      
        bool runOnFunction(Function &F) override {
            errs() << "EncoderFunctionName: " << F.getName() << " -> ";
            if (F.getName().compare("main") != 0) {
                llvm::MD5 Hasher;
                llvm::MD5::MD5Result Hash;
                Hasher.update("llvm_encode_");
                Hasher.update(F.getName());
                Hasher.final(Hash);

                SmallString<32> HexString;
                llvm::MD5::stringifyResult(Hash, HexString);
                F.setName(HexString);
            }
            errs() << F.getName() << "\n";
            return false;
        }
    }; // end of struct Hello
} // end of anonymous namespace
char EncoderFunctionName::ID = 0;
static RegisterPass<EncoderFunctionName> X("encode", "EncoderFunctionName Pass",
                                           false /* Only looks at CFG */,
                                           false /* Analysis Pass */);
static llvm::RegisterStandardPasses
        Y(llvm::PassManagerBuilder::EP_EarlyAsPossible,
          [](const llvm::PassManagerBuilder &Builder,
             llvm::legacy::PassManagerBase &PM) {
              PM.add(new EncoderFunctionName());
          });
```

````shell
ninja LLVMEncoderFunctionName # 单独编译模块   会得到一个动态链接库
opt -load /lib/LLVMEncoderFunctionName.dylib -encode hello.ll -o hello.bc
# 用opt工具加载模块 使用-encode参数 将中间ll文件编译为二进制bc文件
clang hello.bc -o hello # 用得到的bc文件继续编译出的可执行文件  可以拖进ida查看被修改后的函数名
````

![](./picture/MacPic2.png)



### 外部编译pass

官方[教程](https://llvm.org/docs/CMake.html#cmake-out-of-source-pass)

mac下需要修改clang使用c++14，参数为`-std=c++14`,最后编译出来找不到符号很无语。

解决了符号问题！！！！！！！      

报错如下

```shell
Undefined symbols for architecture x86_64:
  "llvm::FunctionPass::assignPassManager(llvm::PMStack&, llvm::PassManagerType)", referenced from:
      vtable for (anonymous namespace)::Hello in Hello.o
 # .....   相同的找不到符号错误
```

谷歌出来的原因是没有找到llvm的lib库，这个解答是正确的，但是缺少了正对cmake的解决方案（网络上都是MakeFile的语法），既然找不到库那就自己链接库。

首先使用`llvm-config --libs`找出所有可以的库，名字都是以`-lLLVM`起头，需要删除才是库的libname。然后通过`llvm_map_components_to_libnames`把所有库都链接到一个符号表内，用法是

```shell
llvm_map_components_to_libnames(mylibname libname libname ...)   # mylibname是自己的符号 
```

然后用target_link_libraries链接符号

```shell
target_link_libraries(LLVMObfuscationPass ${llvm_libs}) # LLVMObfuscationPass就是工具名
```

这两句放进Obfuscation中的CMakeLists.txt就行了！！！

## OLLVM

### 编译Obfuscation

llvm12.0.0编译出问题了，换成llvm9.0.1编译通过，其中有这么几个地方需要注意

1.分别在lib和include/llvm里面添加obfuscation文件夹，include/llvm里面还有一个CryptoUtils.h文件添加

2.lib中以及llvm文件夹中的LLVMBuild.txt文件修改，CMakeLists.txt的文件修改

3.PassManagerBuilder.cpp中不仅要添加头文件，还有几行代码

```cpp
// Flags for obfuscation
static cl::opt<bool> Flattening("fla", cl::init(false),
                                cl::desc("Enable the flattening pass"));

static cl::opt<bool> BogusControlFlow("bcf", cl::init(false),
                                      cl::desc("Enable bogus control flow"));

static cl::opt<bool> Substitution("sub", cl::init(false),
                                  cl::desc("Enable instruction substitutions"));

static cl::opt<std::string> AesSeed("aesSeed", cl::init(""),
                                    cl::desc("seed for the AES-CTR PRNG"));

static cl::opt<bool> Split("split", cl::init(false),
                           cl::desc("Enable basic block splitting"));

PassManagerBuilder::PassManagerBuilder() {} /// 这行之前添加以上部分
```

```cpp
  MPM.add(createSplitBasicBlock(Split));
  MPM.add(createBogus(BogusControlFlow));
  MPM.add(createFlattening(Flattening));
  MPM.add(createSubstitution(Substitution));

  if (OptLevel == 0) {} /// 这行之前添加以上部分
```

```cpp
  MPM.add(createSubstitution(Substitution));

  addExtensionsToPM(EP_OptimizerLast, MPM); /// 这行之前添加以上部分
```

4.Flattening.cpp需要添加头文件

```cpp
#include "llvm/Transforms/Utils.h"  // 需要使用createLowerSwitchPass函数
```

5.BogusControlFlow.cpp需要修改变量类型

```cpp
// TerminatorInst * tbb= fi->getTerminator();
Instruction * tbb= fi->getTerminator(); // 上面这个的返回类型改为Instruction
```



之后使用`ninja LLVMObfuscation`编译ollvm，再`ninja clang`重新编译clang使得clang可以支持Obfuscation

ollvm的[特性手册](https://github.com/obfuscator-llvm/obfuscator/wiki/Features) 

### 字符串加密PASS

**前置知识**：全局变量的符号都是以`@`开头，字符串作为全局变量一般是以`@.str`开头 

```cpp
// Entry.cpp 添加如下 注册参数
static cl::opt<bool> StrObf("strobf", cl::init(false),
                           cl::desc("Enable basic block splitting"));
```



```h
// StringObf.h
// 模仿其他功能的.h头文件即可 
#ifndef OUTPASS_STRINGOBF_H
#define OUTPASS_STRINGOBF_H
// LLVM include
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/CryptoUtils.h"
// Namespace
using namespace llvm;
using namespace std;
namespace llvm {
    Pass *createStringObf (bool flag);
}
#endif //OUTPASS_STRINGOBF_H

```



```cpp
// StringObf.cpp
#include "llvm/Transforms/Obfuscation/StringObf.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
using namespace llvm;
namespace {
    struct StringObf : public FunctionPass {
        static char ID; // Pass identification, replacement for typeid
        bool flag;
        StringObf() : FunctionPass(ID) {}
        StringObf(bool flag) : FunctionPass(ID) {
            this->flag = flag;
        }
        bool runOnFunction(Function &F) {  // 所有操作写在这个函数里面
            if (toObfuscate(flag, &F, "strobf")) { // 当加入了-strobf参数
                for (BasicBlock &basicBlock: F) { // block 块
//                    errs() << basicBlock.getName() << "\n";
                    for (Instruction &instruction: basicBlock) { // Ins 指令 
//                        errs() << instruction << "\n";
                        for (Value *op: instruction.operands()) { // OP 指令中的每个部分 
                            Value *stripPtr = op->stripPointerCasts();
                            if (stripPtr->getName().contains(".str")) { // find op which use "@.str" 
                                // NOTE: 这里使用"@.str"匹配不到
                                errs() << *op << "\n";
                                GlobalVariable *GV = dyn_cast<GlobalVariable>(stripPtr);
                                if (GV) {
                                    ConstantDataSequential *CDS =
                                            dyn_cast<ConstantDataSequential>(GV->getInitializer());
                                    if (CDS) {
                                        std::string str = CDS->getRawDataValues().str();
                                        errs() << "str: " << str << "\n";

                                        // encrypt 加密
                                        uint8_t xor_key = llvm::cryptoutils->get_uint8_t();
                                        for (int i = 0; i < str.size(); i++) {
                                            str[i] = str[i] ^ xor_key;
                                        }


                                        AllocaInst *allocaInst_str = new AllocaInst(
                                                ArrayType::get(Type::getInt8Ty(F.getContext()), str.size()), 0,
                                                nullptr, 1, Twine(stripPtr->getName() + ".array"), &instruction);
									// 申请数组指令 
                                        // ** 函数中有 &instruction 都会生成指令并且默认插入在 &instruction 之前 **
                                        // %.str.array = alloca [7 x i8], align 1

                                        Twine *twine_bitcast = new Twine(
                                                stripPtr->getName() + ".bitcast"); // make twine
                                        BitCastInst *bitCastInst_str = new BitCastInst(allocaInst_str,
                                                                                       Type::getInt8PtrTy(
                                                                                               F.getParent()->getContext()),
                                                                                       twine_bitcast->str(),
                                                                                       &instruction);
                                        // bitcast 才是可以直接使用的item
                                        // %.str.bitcast = bitcast [7 x i8]* %.str.array to i8*

                                        ConstantInt *constantInt_xor_key = ConstantInt::get(
                                                Type::getInt8Ty(F.getContext()), xor_key); // make const -- xor_key
                                        // 申请 xor_key 常量


                                        AllocaInst *allocaInst_xor_key = new AllocaInst(Type::getInt8Ty(F.getContext()),
                                                                                        0,
                                                                                        nullptr, 1,
                                                                                        Twine(stripPtr->getName() +
                                                                                              ".key"), &instruction);
                                        // %.str.key = alloca i8, align 1
                                        // 为xor_key常量申请空间
                                        StoreInst *storeInst_xor_key = new StoreInst(constantInt_xor_key,
                                                                                     allocaInst_xor_key);
                                        storeInst_xor_key->insertAfter(allocaInst_xor_key);
                                        // StoreInst中没有 &instruction 不会插入指令 需要主动调用函数来插入指令
                                        // store i8 -116, i8* %.str.key
                                        // 储存常量到空间中

                                        LoadInst *loadInst_xor_key = new LoadInst(allocaInst_xor_key, ""); 
                                        // name = null  name为空则从0开始命名 %0 %1 %2 %3 ...
                                        loadInst_xor_key->insertAfter(storeInst_xor_key);
                                        // %0 = load i8, i8* %.str.key
                                        // 加载xor_key  其实可以直接使用常量 这样做是为了不被IDA优化  

                                        for (int i = 0; i < str.size(); i++) {

                                            ConstantInt *index = ConstantInt::get(Type::getInt8Ty(F.getContext()), i); 
                                            // 申请index为常量
                                            GetElementPtrInst *getElementPtrInst = GetElementPtrInst::CreateInBounds(
                                                    bitCastInst_str, index);
                                            getElementPtrInst->insertBefore(&instruction);
                                            //  取出数组中当前index的元素 
                                            // foreach array element
                                            // %1 = getelementptr inbounds i8, i8* %.str.bitcast, i8 0
                                            // %3 = getelementptr inbounds i8, i8* %.str.bitcast, i8 1
                                            // %5 = getelementptr inbounds i8, i8* %.str.bitcast, i8 2   # 0,1,2 is index
                                            // ....

                                            ConstantInt *enc_ch = ConstantInt::get(Type::getInt8Ty(F.getContext()),
                                                                                   str[i]);
                                            // 申请被加密的str[i]作为常量
                                            BinaryOperator *xor_inst = BinaryOperator::CreateXor(enc_ch,
                                                                                                 loadInst_xor_key);
                                            // 这里就是直接使用常量了
                                            xor_inst->insertAfter(getElementPtrInst);
                                            // %2 = xor i8 -4, %0

                                            StoreInst *storeInst = new StoreInst(xor_inst, getElementPtrInst);
                                            storeInst->insertAfter(xor_inst);
                                            // 保存解密后的字符
                                            // store i8 %2, i8* %1
                                            // until now decrypt over
                                        }
                                        op->replaceAllUsesWith(bitCastInst_str); // use bitcast replace all place which use whis op
                                        // 不需要引用全局变量的op了 直接使用bitcast作为新的op
                                        GV->eraseFromParent(); // remove global values

                                    }
                                }
                            }
                        }
                        errs() << "\n";
                    }
                }
            }
            return false;
        }
    };
}

char StringObf::ID = 0;
static RegisterPass<StringObf> X("stringobf", "string obf");

Pass *llvm::createStringObf(bool flag) {
    return new StringObf(flag);
}
```



