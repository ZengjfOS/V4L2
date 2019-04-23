# OpenCV

## 参考文档

* https://opencv.org/
* https://opencv.org/releases.html
* https://docs.opencv.org/3.4.2/
* [OpenCV On Android最佳环境配置指南(Android Studio篇)](https://www.jianshu.com/p/6e16c0429044)

## YOCTO查找OpenCV版本

* 查看下载文件
  ```
  zengjf@zengjf:~/yocto/imx-yocto-bsp/downloads$ ls -1 | grep opencv
  git2_github.com.opencv.opencv_3rdparty.git.tar.gz
  git2_github.com.opencv.opencv_3rdparty.git.tar.gz.done
  git2_github.com.opencv.opencv_contrib.git.tar.gz
  git2_github.com.opencv.opencv_contrib.git.tar.gz.done
  git2_github.com.opencv.opencv.git.tar.gz
  git2_github.com.opencv.opencv.git.tar.gz.done
  ```
* 查找解压文件
  ```
  zengjf@zengjf:~/yocto/imx-yocto-bsp/build_xwayland/tmp/work$ find * -iname opencv
  aarch64-mx8-poky-linux/imx-gpu-sdk/5.2.0-r0/git/DemoApps/OpenCV
  aarch64-mx8-poky-linux/imx-gpu-sdk/5.2.0-r0/git/DemoApps/Shared/Camera/Adapter/OpenCV
  aarch64-mx8-poky-linux/imx-gpu-sdk/5.2.0-r0/git/DemoApps/Shared/Camera/Adapter/OpenCV/source/Shared/Camera/Adapter/OpenCV
  aarch64-mx8-poky-linux/imx-gpu-sdk/5.2.0-r0/git/DemoApps/Shared/Camera/Adapter/OpenCV/include/Shared/Camera/Adapter/OpenCV
  aarch64-mx8-poky-linux/imx-gpu-sdk/5.2.0-r0/git/ThirdParty/OpenCV
  aarch64-mx8-poky-linux/opencv
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv-samples/usr/share/OpenCV
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv-dev/usr/include/opencv
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv-dev/usr/share/OpenCV
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv-dbg/usr/src/debug/opencv
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv-dbg/usr/share/OpenCV
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv-apps/usr/share/OpenCV
  aarch64-mx8-poky-linux/opencv/3.4.2+gitAUTOINC+9e1b1e5389_d4e0286945_a62e20676a_34e4206aef_fccf7cd6a4-r0/packages-split/opencv
  [...省略]
  ```
* 版本：[3.4.2 下载](https://sourceforge.net/projects/opencvlibrary/files/opencv-android/3.4.2/opencv-3.4.2-android-sdk.zip/download)
* [build-opencv-for-android](https://github.com/tzutalin/build-opencv-for-android)