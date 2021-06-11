package com.android.cpa.appfix;

import android.content.Intent;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import com.android.cpa.appfix.activity.NextActivity;
import com.swift.sandhook.SandHook;
import com.swift.sandhook.SandHookConfig;
import com.swift.sandhook.wrapper.HookErrorException;

public class MainActivity extends AppCompatActivity {

    public static String[] classes_methods_fields = new String[]{

            "com/swift/sandhook/SandHook",
            "getArtMethod",
            "(Ljava/lang/reflect/Member;)J",
            "getJavaMethod",
            "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;",
            "getThreadId",
            "testAccessFlag",

            "com/swift/sandhook/ClassNeverCall",
            "com.swift.sandhook.ClassNeverCall",
            "neverCall",
            "neverCall2",
            "neverCallNative",
            "neverCallNative2",
            "neverCallStatic",

            "com/swift/sandhook/PendingHookHandler",
            "onClassInit",
            "(J)V",

            "com/swift/sandhook/SandHookMethodResolver",
            "resolvedMethodsAddress",
            "entryPointFromInterpreter",
            "entryPointFromCompiledCode",
            "dexMethodIndex",

            "com/swift/sandhook/ArtMethodSizeTest",
            "com.swift.sandhook.ArtMethodSizeTest",
            "method1",
            "method2",

            "com/swift/sandhook/SandHookConfig",
            "compiler",

    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void init_hook_now(View view) {

        SandHookConfig.libLoader.loadLib();
        SandHook.__JNI_OnLoad__(getApplication().getClassLoader(), classes_methods_fields);
        SandHook.init();

        SandHookConfig.DEBUG = BuildConfig.DEBUG;
        SandHook.disableVMInline();
        SandHook.tryDisableProfile(getPackageName());
        SandHook.disableDex2oatInline(false);
        if (SandHookConfig.SDK_INT >= Build.VERSION_CODES.P) {
            SandHook.passApiCheck();
        }

    }

    public void do_hook_now(View view) {
        try {
            SandHook.addHookClass(
                    ActivityHooker.class
                    );
        } catch (HookErrorException e) {
            e.printStackTrace();
        }
    }

    public void start_new_activity(View view) {
        startActivity(new Intent(this, NextActivity.class));
    }

}