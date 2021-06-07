
CPP 方面:

    1. hide_api.c 里加入

        #include "../nativelib/sandhook_native.h"
        ...
        //init native hook lib
        hook_native = SandInlineHook;
        // 然后注释掉接下来对 hook_native 赋值的那几行
        ...


    2. sandhook.cpp 注释掉 JNI_OnLoad 方法, 然后加入 __JNI_OnLoad__ 方法

    3. 新建 Android.bp, 按照两个 CMakeLists.txt 写好

    4. 把两个CPP Module的所有代码 Hook/HOOK/hook 关键字都改成 Lock

    4. 复制 Nativehook 的 cpp 代码复制到 nativelib 下



JAVA 方面:

    1. SandHook.java 的 static 块去掉, 它的 init 方法改成 public

    2. SandHookConfig.java 把 import ...BuildConfig 一行去掉

    3. JAVA 方面的类名方法名随便改, 只要往 __JNI_OnLoad__ 传过去就行

