# README.md

## 参考文档

* [Linux UVC driver and tools](http://www.ideasonboard.org/uvc/)
* [28. The Linux USB Video Class (UVC) driver](https://www.kernel.org/doc/html/v4.9/media/v4l-drivers/uvcvideo.html)
* [Source Code](https://github.com/torvalds/linux/tree/master/drivers/media/usb/uvc)
* [Linux UVC分析](https://my.oschina.net/u/2007478?tab=newest&catalogId=5662494)
* [EZ-USB FX2LP CY7C68013A分析](https://github.com/ZengjfOS/USB)
* [UVC（USB Video Class）协议讲解](https://blog.csdn.net/LinuxWorking/article/details/78419631)
* [Universal Serial Bus Device Class Definition for Video Devices](https://www.spinelelectronics.com/pdf/UVC%201.5%20Class%20specification.pdf)

## 笔记文档

* [0006_uvc_scan_device.md](0006_uvc_scan_device.md)：将Unit/Terminal和Chain绑定起来；
* [0005_uvc_ctrl_init_device.md](0005_uvc_ctrl_init_device.md)：将VC中描述符中的控制位解析成系统控制控件；
* [0004_uvc_parse_streaming.md](0004_uvc_parse_streaming.md)：VS信息放在`struct uvc_device`的streams链表；
* [0003_uvc_probe_Parse_IAD_Interface.md](0003_uvc_probe_Parse_IAD_Interface.md)：IAD信息是如何被解析的，VC/VS信息如何被获知的，VC Terminal/Unit信息放在`struct uvc_device`的entities链表；
* [0002_uvc_register.md](0002_uvc_register.md)：uvc驱动在哪里，支持哪些设备？
* [0001_USB_Camera_Descriptor.md](0001_USB_Camera_Descriptor.md)：如何通过lsusb获取设备描述符；