# ART定制

## LoadMethod的相关hook

### 前置知识

C++中，对于结构体`struct`或者类`class`都有的性质：

1、成员函数不会占空间，编译后会成为一个独立的全局的函数，会在参数列表的头部新增一个`参数：this指针`来与相关的类或是结构体建立联系。

2、如果有虚函数，那么其内部首地址会有一个指向虚函数表的指针，之后才是其他成员变量之类的东西。

3、静态变量不占空间。空的结构体或者类占1个空间，因为要保证每一个结构体有不同的地址。

### hook得到Dexfile思路

首先看[DexFile](http://aospxref.com/android-8.0.0_r36/xref/art/runtime/dex_file.h)的结构

```c++
class DexFile{
    //  一些static变量、函数、枚举、结构体     都不占空间 但是存在虚函数
    
	// The base address of the memory mapping.
	const uint8_t* const begin_;  // 指针类型  大小取决于arch
	// The size of the underlying memory allocation in bytes.
	const size_t size_;  // size_t 大小取决于arch
    
    // 其他成员变量
}
```

然后是8.0下的`LoadMethod`函数原型 （不同版本下`LoadMethod`原型可能存在差异）

```c++
void ClassLinker::LoadMethod(const DexFile& dex_file,
						const ClassDataItemIterator& it,
                             Handle<mirror::Class> klass,
                             ArtMethod* dst)
```

可见原型中一个参数即为`DexFile`的指针，该`LoadMethod`是一个类的成员函数，所以第一个参数为this指针，第二个参数才是DexFile指针。

hook函数的思路可以先拖出apk中的`libart.so`文件，找到对应的函数名进行hook。也可以枚举所有的导出符号，通过匹配字符串得到函数地址进行hook。

得到指针之后，因为存在虚函数，所以`base+pointersize`为`begin_`，`base+pointersize*2`为`size_`。

### hook得到ArtMethod思路

首先看[ArtMethod](http://aospxref.com/android-8.0.0_r36/xref/art/runtime/art_method.h)的结构

```c++
class ArtMethod FINAL {
    //  一些static变量、函数、枚举、结构体     都不占空间 并且没有虚函数
    
     GcRoot<mirror::Class> declaring_class_;  // 没有虚函数，只存在一个4字节的变量
    
	std::atomic<std::uint32_t> access_flags_; // 4 字节
	/* Dex file fields. The defining dex file is available via declaring_class_->dex_cache_ */
	// Offset to the CodeItem.
	uint32_t dex_code_item_offset_;  // 4字节
	// Index into method_ids of the dex file associated with this method.
	uint32_t dex_method_index_;     // 4字节
    
    // 其他成员变量
}
```

由结构知、类`ArtMethod`中不存在虚函数表，并且所有成员变量的大小都是固定的

所以`base+8`为`dex_code_item_offset_`，`base+12`为`dex_method_index_`。

## Android源码

AOSP源码下载：http://wuxiaolong.me/2018/07/07/AOSP1/

**下载速度很慢，并且到最后99%的时候会卡住很久 ，一直等就行了**

Android Studio导入AOSP源码：http://wuxiaolong.me/2018/08/15/AOSP3/

**使用repo的时候需要使用python3.6以上，但是生成gen的时候需要使用python2，否则报错SyntaxError无法正常生成**

java层的类的native的实现文件是类引用中 点换成下划线，比如

```
目录/libcore/dalvik/src/main/java/dalvik/system下的DexFile的package 为： package dalvik.system
则目录/art/runtime/native下的dalvik_system_DexFile就是其native实现
```

可以合理修改源码去实现我们想要实现的功能，并且只要make了一次，下一次make只会重新编译修改过的模块，效率还是很客观的。

## so文件加载流程

```java
System.loadLibrary()
Runtime.java.loadlibrary()
Runtime.java.loadlibrary0()
Runtime.java.doload()
Runtime.java.nativeLoad()  --对应-- >  native Runtime_nativeLoad() //从java层进入native层
JVM_NativeLoad()
vm->LoadNativeLibrary() :  OpenNativeLibrary()    library->FindSymbol()
```

可以通过hook函数`LoadNativeLibrary`来了解加载了哪些so，并且hook函数`FindSymbol`来了解加载了哪些符号

这两个函数在`/system/lib/libart.so`里面都有导出 ，拖进IDA就可以找到对应符号

```js
function readStdString(str) { // string 类型 储存方式特别 
    const isTiny = (str.readU8() & 1) == 0
    if (isTiny) {
        return str.add(1).readUtf8String()
    }
    return str.add(2 * Process.pointerSize).readPointer().readUtf8String()
}

function hook_art() {
    var LoadNativeLibraryaddr = Module.findExportByName('libart.so', '_ZN3art9JavaVMExt17LoadNativeLibraryEP7_JNIEnvRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEP8_jobjectPS9_')
    console.log('LoadNativeLibrary -> ', LoadNativeLibraryaddr)
    // art::JavaVMExt::LoadNativeLibrary(_JNIEnv *, std::string const&, _jobject *, std::string*)
    // _ZN3art9JavaVMExt17LoadNativeLibraryEP7_JNIEnvRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEP8_jobjectPS9_
    // art::JavaVMExt::LoadNativeLibrary(_JNIEnv *, std::string const&, _jobject *, std::string*)::$_16::operator()(_jobject *)const
    // _ZZN3art9JavaVMExt17LoadNativeLibraryEP7_JNIEnvRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEP8_jobjectPS9_ENK4$_16clESD
    Interceptor.attach(LoadNativeLibraryaddr, {
        onEnter: function (args) {
            console.log('goto in LoadNativeLibrary')
            var pathptr = args[2]
            console.log('LoadNativeLibrary load so -> ' + readStdString(ptr(pathptr)))
        },
        onLeave: function (retval) {

        }
    });

    //_ZN3art13SharedLibrary10FindSymbolERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEPKc
    // findExportByName有些时候找不到 可以直接枚举某个mudule的所有符号查找 
    var libart = Process.findModuleByName('libart.so')
    var symbols = libart.enumerateSymbols()
    var FindSymboladdr = null
    symbols.forEach(function (symbol) {
        if (symbol.name == '_ZN3art13SharedLibrary10FindSymbolERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEPKc') {
            FindSymboladdr = symbol.address
        }
    });
    Interceptor.attach(FindSymboladdr, {
        onEnter: function (args) {
            console.log('enter FindSymbol')
            this.pathptr = args[1]
            //console.log('FindSymbol : ' + readStdString(ptr(this.pathptr)))
        },
        onLeave: function (retval) {
            console.log('leave FindSymbol')
            console.log('FindSymbol : ' + readStdString(ptr(this.pathptr)) + '----address : ' + ptr(retval))
        }
    });

}
setImmediate(hook_art)
```

## JNI函数

每个Java层函数在Native层都对应一个ArtMethod。JNI函数执行之前必须完成JNI函数对应类的加载和初始化，以及该JNI函数的ArtMethod对象中属性和so中具体函数地址绑定。

JNI函数的绑定分为：静态注册和动态注册，并且两种注册都有两次绑定

第一次绑定的函数为`UnregisterNative`，绑定的函数地址为“桥函数”，hook没有很大的作用。

第二次绑定函数为`RegisterNative`，参数就是函数实际地址。可以尝试hook一下

```js
function hook_register() {
    // _ZN3art9ArtMethod14RegisterNativeEPKv
    var libart = Process.findModuleByName('libart.so')
    var RegisterNativeaddr = libart.findExportByName('_ZN3art9ArtMethod14RegisterNativeEPKv')
    console.log('RegisterNativeaddr: ' + RegisterNativeaddr)
    Interceptor.attach(RegisterNativeaddr, {
        onEnter: function (args) {
            var artmethodaddr = args[0]
            var dex_method_index_ = Memory.readU32(ptr(artmethodaddr).add(12)) // 分析artmethod结构得到的固定偏移
            var functionaddr = args[1]
            console.log('enter RegisterNative')
            console.log('RegisterNative -> ' + dex_method_index_ + '------addr: ' + ptr(functionaddr))

        },
        onLeave: function (retval) {
            console.log('leave RegisterNative')
        }
    });
}
setImmediate(hook_register)
```



静态注册的JNI函数：有两次绑定，第一次是在JNI函数对应类加载的时候，会绑定到ART中的“桥函数”，第二次是在JNI第一次真正被调用的时候由“桥函数”完成JNI函数在so中的地址查询以及绑定。

“桥函数”为`art_jni_dlsym_lookup_stub`，之后依次调用`artFindNativeMethod`、`FindCodeForNativeMethod`。`FindCodeForNativeMethod`中调用了`FindNativeMethod`，`FindNativeMethod`调用了`FindSymbol`，所以之前hook掉`FindSymbol`也可以看到静态注册的JNI函数符号

动态注册的JNI函数：也有两次绑定。不同之处是第一次绑定的“桥函数”需要开发者自己实现，就比如JNIOnLoad函数。

两者最后都通过`SetEntryPointFromJni`函数绑定最后准确的函数地址



