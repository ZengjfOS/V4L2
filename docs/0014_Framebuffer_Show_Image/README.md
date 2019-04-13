# Framebuffer Show Image


## 参考文档

* [How do I display images without starting X11?](https://raspberrypi.stackexchange.com/questions/8922/how-do-i-display-images-without-starting-x11)
* [Android Display System（5）：Android Display System 系统分析之Display Driver Architecture](http://zhoujinjian.cc/2018/08/30/Android%20Display%20System%EF%BC%885%EF%BC%89%EF%BC%9AAndroid%20Display%20System%20%E7%B3%BB%E7%BB%9F%E5%88%86%E6%9E%90%E4%B9%8BDisplay%20Driver%20Architecture/index.html)

## 源代码位置

[examples/fbshow.c](examples/fbshow.c)

## 注意事项

* `adb shell stop`杀掉**surfaceflinger**之后在测试，否则测试无效，程序运行出错；
* 测试完通过`adb shell start`重启**surfaceflinger**；
