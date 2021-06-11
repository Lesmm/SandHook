
CPP 方面:

    1. hide_api.c 里加入

        #include "../nativelib/sandhook_native.h"
        ...
        //init native hook lib
        hook_native = SandInlineHook;   // 注意后面改成 SandInlineLock
        // 然后注释掉接下来对 hook_native 赋值的那几行
        ...


    2. sandhook.cpp 注释掉 JNI_OnLoad 方法, 然后加入 __JNI_OnLoad__ 方法

    3. 新建 Android.bp, 按照两个 CMakeLists.txt 写好

    4. 把两个CPP Module的所有代码 Hook/HOOK 关键字都改成 Lock/LOCK
        "setLockMode" 及 "initForPendingLock" 两个Native方法注册时, 对应JAVA那边也要改了
        关键字 hook 换成 lock 时: 因为只改代码, 文件名这些不用改, 基本上把 _sandhook_ 换成 _sandlock_ 就可以了
        而且 nativehook Module 没有需要替换 hook 的代码,只换了 hooklib Module 就行了
        注意: bool isSandHooker(char *const args[]) 那里写死了个 SandHooker, 需要注意在JAVA层的两个XposedCompat Module的
        CLASS_NAME_PREFIX 属性 及 hooklib 的 SandHookerStubClass_ 也跟着改了

    5. 复制 Nativehook 的 cpp 代码复制到 nativelib 下



JAVA 方面:

    1. SandHook.java 的 static 块去掉, 它的 init 方法改成 public

    2. SandHookConfig.java 把 import ...BuildConfig 一行去掉

    3. JAVA 方面的类名方法名随便改, 只要往 __JNI_OnLoad__ 传过去就行

    4. TODO ... 注册时 Native Method 时的方法名以后再改了 ~~~

