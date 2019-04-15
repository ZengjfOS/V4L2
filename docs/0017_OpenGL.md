# OpenGL

## 参考文档

* [OpenGL ES SDK for Android Documentation](https://arm-software.github.io/opengl-es-sdk-for-android/index.html)
* [Learn-OpenGLES-Tutorials](https://github.com/learnopengles/Learn-OpenGLES-Tutorials)
* [Android Lesson One: Getting Started](https://www.learnopengles.com/android-lesson-one-getting-started/)

## Tutorials指令集修改内容

```
ndk{
    abiFilters "arm64-v8a"
}

externalNativeBuild {
    cmake {
        cppFlags "-frtti -fexceptions"
        arguments "-DANDROID_PLATFORM_LEVEL=${platformVersion}",
                '-DANDROID_TOOLCHAIN=clang', '-DANDROID_STL=c++_shared'
    }
}
```