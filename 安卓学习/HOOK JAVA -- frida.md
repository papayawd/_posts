# HOOK JAVA -- frida

**hook基础之前的笔记中已经写有，这写一些新学到的东西**

使用npm为vscode安装frida代码编写提示工具

```js
npm i @types/frida-gum
```

往adb连接到的设备中 正在运行的app的输入框中传入字符串（粘贴功能）

```js
adb shell // 先进入shell
input text string // string 需要加上双引号
```

修改某个变量的值

```js
var Activity = Java.use(className) // className类名
Activity.v1.value = true // 静态变量 类.变量名.value 即可修改

Java.choose(xxx,{
    onMatch: function (instance){
        instance.v2.value = true //  非静态变量和非静态方法一样 需要使用choose
        instance._v3.value = true // 若此时该类中存在一个与变量同名的方法v3 则需要在名前加下划线
    },
    onComplete: function (){
    }
});

```

内部类的hook

```js
var Class2 = Java.use("xxx.xxx.Class1$Class2") // 使用$符号便可以访问到内部类 嵌套类无非是多几个$符号
```

Object的类名获取

```js
object.$className // $符号
```

Java类枚举，找到动态加载的dex中的类，load类

```js
Java.enumerateClassLoaders({
	onMatch: function (loader) {
		try { // 正常情况下不会报错
			if (loader.findClass(ClassName)) { // ClassName为要找的class
				console.log(loader);
				Java.classFactory.loader = loader; // 如此load之后才能使用Java.use
				}
		} catch (error) {
			}
	}, 
    onComplete: function () {
	}
});
```

hook一个类的构造函数

```js
var class = Java.use(className) // 
a.$init.implementation = function(){   // .$init 即为构造函数名
    
};
```

动态加载dex

```js
var dex = Java.openClassFile(dexPath); // dexPath为dex在android机中的路径
dex.load() // 加载dex
var class = Java.use(classPath) // 之后可以正常使用dex中的clas
```

调用没有实例的方法

```js
var Exception = Java.use("java.lang.Exception");
var instance = Exception.$new("print_stack"); // 使用.$new 创建实例
var stack = instance.getStackTrace(); // 使用实例中的方法 
instance.$dispose(); //  使用 .$dispose 析构函数释放实例  
```

打印某地址的char*字符串  ptr的加法和作为地址读写

```js
var p = ptr(0x123456)
console.log(p.readCString()) // 打印
p = p.add(0x321) // 加法 
var v = p.readPointer() // 把p作为地址读值 int大小 相当于 [p]
p.writePointer(ptr) // 把p作为地址写值
```

使用`NativeFunction`调用c的函数（so内的函数）

```js
var addr_fopen = Module.findExportByName("libc.so", "fopen"); // 先找到函数地址 
var fopen = new NativeFunction(addr_fopen, "pointer", ["pointer", "pointer"]); // 注册Native函数对象
var filename = Memory.allocUtf8String("/sdcard/reg.dat"); // js的字符串和c的字符串不同 需要这样转换
var open_mode = Memory.allocUtf8String("w+");
var file = fopen(filename, open_mode); // 正常使用函数 
```

修改Interceptor.attach - onEnter 中参数的值的时候需要使用`ptr`类型赋值 

```js
Interceptor.attach(addr,{
    onEnter: function(args){
        args[0] = ptr(0x123456) // ptr类型赋值 
    }
})
```

hook `RegisterNatives`

```js
function hook_libart() {
    var module_libart = Process.findModuleByName("libart.so");
    var symbols = module_libart.enumerateSymbols();     //枚举模块的符号
    var addr_GetStringUTFChars = null;
    var addr_FindClass = null;
    var addr_GetStaticFieldID = null;
    var addr_SetStaticIntField = null;
    var addr_RegisterNatives = null;        // 怎么hook RegisterNatives
    for (var i = 0; i < symbols.length; i++) {
        var name = symbols[i].name;
        if (name.indexOf("art") >= 0) {  // 函数名 前缀
            if ((name.indexOf("CheckJNI") == -1) && (name.indexOf("JNI") >= 0)) { // 不要checkjni
                if (name.indexOf("GetStringUTFChars") >= 0) {
                    console.log(name);
                    addr_GetStringUTFChars = symbols[i].address;
                } else if (name.indexOf("FindClass") >= 0) {
                    console.log(name);
                    addr_FindClass = symbols[i].address;
                } else if (name.indexOf("GetStaticFieldID") >= 0) {
                    console.log(name);
                    addr_GetStaticFieldID = symbols[i].address;
                } else if (name.indexOf("SetStaticIntField") >= 0) {
                    console.log(name);
                    addr_SetStaticIntField = symbols[i].address;
                } else if (name.indexOf("RegisterNatives") >= 0) {   // 寻找 RegisterNatives
                    console.log(name);
                    addr_RegisterNatives = symbols[i].address;
                }
            }
        }
    }
    if (addr_RegisterNatives) { // 存在 RegisterNatives
        Interceptor.attach(addr_RegisterNatives, {
            onEnter: function (args) {
                console.log("addr_RegisterNatives:", hexdump(args[2]));
                console.log("addr_RegisterNatives name:", ptr(args[2]).readPointer().readCString())
                console.log("addr_RegisterNatives sig:", ptr(args[2]).add(Process.pointerSize).readPointer().readCString());
            }, onLeave: function (retval) {
            }
        });
    }
}

```

inline hook可能存在问题，因为`Interceptor.attach`的`onEnter`参数访问可能存在问题，这个时候可以修改字节为断点，使用异常处理来解决

```js
var base = Module.getBaseAddress(xxx)
Process.setExceptionHandler(function(details){ // 异常监控
   if(details.type == 'breakpoint')  // 判断异常为断点
   {
		// 1.做任何想做的
       // 2.还原0xcc处 原来的值
       return true // 3. 返回true表示异常正常处理 
   }
});
Memory.protect(base.add(xxx),0x1,'rxw')
Memory.writeU8(base.add(xxx),0xcc) // exe举例  断点为0xcc
```





