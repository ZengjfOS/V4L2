# USB Camera Descriptor

## 工具

* `lsusb`： Linux工具
* [Thesycon USB Descriptor Dumper](https://www.thesycon.de/eng/usb_descriptordumper.shtml)：Windows工具

## 简述

* 基本上UVC跟CDC-ACM類似，它一定要支援兩個Interface，一個用來控制用，一個用來傳資料。控制用的Interface叫做Video Control Interface，另一個叫Video Streaming Interface。這兩個Interface得用IAD把他們接合在一起。
* UVC支援兩種資料傳輸模式，第一個是Isochronous模式，第二個是bulk模式。這邊不講bulk模式，因為市面上99.9999%的UVC裝置都是用Isochronous模式。
* Isochronous模式有個特點，就是必須使用Alternate Setting，如果你忘了Alternate Setting的話，記得再去瞄一下USB2.0規格。基本上只要宣告Alternate Setting 0跟1就可以了。其中Alternate Setting 0是不包含任何Endpoint宣告的，而Alternate Setting 1則會包含一個Isochronous Endpoint宣告。當HOST想要接收影像資料時，會用Set Interface去選擇Alternate Setting 1，而這時DEVICE就得開始輸出資料。相同地，Alternate Setting 0就是不要傳資料。
* 資料的格式部分，因為UVC目的是支援大部分的裝置，所以裝置端必須提供它支援的格式。資料格式分兩種，第一種是Video Format，其下又分Video Frame。這很好理解，基本上你得先讓HOST選擇影像格式，才能再選擇它的解析度、bitrate跟frame rate。Video Format對應的就是影像格式，而Video Frame則是對應到解析度、bitrate跟frame rate。這樣做的好處是每個裝置可以支援多種影像格式，像是RGB或 MJPEG。
* UVC要支援MJPEG或H264的原因很簡單，因為傳統的RGB格式真的佔用太多USB頻寬了，加上現在沒有HD賣不動，用RGB格式根本沒辦法達到720p30。1280*720*30*3 = 82,944,000 bytes，直接爆掉USB頻寬，所以你會看到有些Webcam號稱支援720p或1080p RGB格式，但是它的frame rate是可憐的1或5，這有意義嗎？
* 从后面Linux代码解读来看，其实Widnows工具获取的数据是更贴切的，因为UVC部分的描述符是在驱动中单独分析出来的；
* 一个VC控制接口描述符；
* 一个VS流接口描述符，多选一模式的；
* IDA（usb_interface_assoc_descriptor）在usb_host_config描述符中，UVC设备值关心usb_interface_assoc_descriptor数组的第一个，也就是VC接口描述符，所以在Linux驱动中probe传入的interface就是VC接口描述符；
* VC和VS的描述符信息在各自的usb_interface->cur_altsetting->extra中，长度是extralen，所有的UVC自定义的描述符都在这，Linux驱动解析VC就是解析extra数据；
* 设备描述符
  * 配置描述符
    * IAD接口描述符(VC和VS接口的个数等信息在IAD描述符中的bInterfaceCount表示)
      * VC接口描述符
        * HEADER描述符
          * OUTPUT_TERMINAL描述符
          * EXTENSION_UNIT描述符
          * INPUT_TERMINAL描述符
          * PROCESSING_UNIT描述符
        * 端点描述符
      * VS接口描述符(bAlternateSetting = 0)
        * INPUT_HEADER描述符(bNumFormats表示包含多少种帧格式)
          * FORMAT_MJPEG描述符(bFormatIndex表示当前是第几种帧格式，bNumFrameDescriptors当前这种帧格式有多少种尺寸)
          * FRAME描述符(bFrameIndex表示当前帧格式是第几个)
          * STILL_IMAGE_FRAME描述符
          * COLORFORMAT描述符
        * Video Streaming接口描述符(bAlternateSetting > 0)

## 参考文档

* [UVC（USB Video Class）协议讲解](https://blog.csdn.net/LinuxWorking/article/details/78419631)
* [从零写UVC驱动之分析描述符](https://www.cnblogs.com/liusiluandzhangkun/p/8729528.html)
* [USB Video Class 簡介](http://pollos-blog.blogspot.com/2014/10/usb-video-class.html)

## Linux lsusb Dump Camera USB Descriptor

```
pi@raspberrypi:~ $ lsusb
Bus 001 Device 002: ID 057e:030a Nintendo Co., Ltd
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
pi@raspberrypi:~ $ lsusb -vd 057e:030a

Bus 001 Device 002: ID 057e:030a Nintendo Co., Ltd
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               2.00
  bDeviceClass          239 Miscellaneous Device
  bDeviceSubClass         2 ?
  bDeviceProtocol         1 Interface Association
  bMaxPacketSize0        64
  idVendor           0x057e Nintendo Co., Ltd
  idProduct          0x030a
  bcdDevice            9.24
  iManufacturer          48
  iProduct               96
  iSerial                 0
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength          777
    bNumInterfaces          2
    bConfigurationValue     1
    iConfiguration         48
    bmAttributes         0x80
      (Bus Powered)
    MaxPower              500mA
    Interface Association:
      bLength                 8
      bDescriptorType        11
      bFirstInterface         0
      bInterfaceCount         2
      bFunctionClass         14 Video
      bFunctionSubClass       3 Video Interface Collection
      bFunctionProtocol       0
      iFunction              96
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      1 Video Control
      bInterfaceProtocol      0
      iInterface             96
      VideoControl Interface Descriptor:
        bLength                13
        bDescriptorType        36
        bDescriptorSubtype      1 (HEADER)
        bcdUVC               1.00
        wTotalLength           77
        dwClockFrequency       15.000000MHz
        bInCollection           1
        baInterfaceNr( 0)       1
      VideoControl Interface Descriptor:
        bLength                 9
        bDescriptorType        36
        bDescriptorSubtype      3 (OUTPUT_TERMINAL)
        bTerminalID             2
        wTerminalType      0x0101 USB Streaming
        bAssocTerminal          0
        bSourceID               4
        iTerminal               0
      VideoControl Interface Descriptor:
        bLength                26
        bDescriptorType        36
        bDescriptorSubtype      6 (EXTENSION_UNIT)
        bUnitID                 4
        guidExtensionCode         {f07735d1-898d-0047-812e-7dd5e2fdb898}
        bNumControl             8
        bNrPins                 1
        baSourceID( 0)          3
        bControlSize            1
        bmControls( 0)       0xff
        iExtension              0
      VideoControl Interface Descriptor:
        bLength                18
        bDescriptorType        36
        bDescriptorSubtype      2 (INPUT_TERMINAL)
        bTerminalID             1
        wTerminalType      0x0201 Camera Sensor
        bAssocTerminal          0
        iTerminal               0
        wObjectiveFocalLengthMin      0
        wObjectiveFocalLengthMax      0
        wOcularFocalLength            0
        bControlSize                  3
        bmControls           0x0000020a
          Auto-Exposure Mode
          Exposure Time (Absolute)
          Zoom (Absolute)
      VideoControl Interface Descriptor:
        bLength                11
        bDescriptorType        36
        bDescriptorSubtype      5 (PROCESSING_UNIT)
      Warning: Descriptor too short
        bUnitID                 3
        bSourceID               1
        wMaxMultiplier          0
        bControlSize            2
        bmControls     0x0000157f
          Brightness
          Contrast
          Hue
          Saturation
          Sharpness
          Gamma
          White Balance Temperature
          Backlight Compensation
          Power Line Frequency
          White Balance Temperature, Auto
        iProcessing             0
        bmVideoStandards     0x 0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x82  EP 2 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0010  1x 16 bytes
        bInterval               6
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       0
      bNumEndpoints           0
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      VideoStreaming Interface Descriptor:
        bLength                            15
        bDescriptorType                    36
        bDescriptorSubtype                  1 (INPUT_HEADER)
        bNumFormats                         2
        wTotalLength                      557
        bEndPointAddress                  129
        bmInfo                              0
        bTerminalLink                       2
        bStillCaptureMethod                 2
        bTriggerSupport                     1
        bTriggerUsage                       0
        bControlSize                        1
        bmaControls( 0)                    11
        bmaControls( 1)                    11
      VideoStreaming Interface Descriptor:
        bLength                            11
        bDescriptorType                    36
        bDescriptorSubtype                  6 (FORMAT_MJPEG)
        bFormatIndex                        1
        bNumFrameDescriptors                5
        bFlags                              1
          Fixed-size samples: Yes
        bDefaultFrameIndex                  1
        bAspectRatioX                       0
        bAspectRatioY                       0
        bmInterlaceFlags                 0x00
          Interlaced stream or variable: No
          Fields per frame: 1 fields
          Field 1 first: No
          Field pattern: Field 1 only
          bCopyProtect                      0
      VideoStreaming Interface Descriptor:
        bLength                            38
        bDescriptorType                    36
        bDescriptorSubtype                  7 (FRAME_MJPEG)
        bFrameIndex                         1
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            640
        wHeight                           480
        dwMinBitRate                   128000
        dwMaxBitRate                 11059200
        dwMaxVideoFrameBufferSize      460800
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  0
        dwMinFrameInterval             333333
        dwMaxFrameInterval            9999990
        dwFrameIntervalStep            333333
      VideoStreaming Interface Descriptor:
        bLength                            38
        bDescriptorType                    36
        bDescriptorSubtype                  7 (FRAME_MJPEG)
        bFrameIndex                         2
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            320
        wHeight                           240
        dwMinBitRate                   128000
        dwMaxBitRate                  2764800
        dwMaxVideoFrameBufferSize      115200
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  0
        dwMinFrameInterval             333333
        dwMaxFrameInterval            9999990
        dwFrameIntervalStep            333333
      VideoStreaming Interface Descriptor:
        bLength                            38
        bDescriptorType                    36
        bDescriptorSubtype                  7 (FRAME_MJPEG)
        bFrameIndex                         3
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            160
        wHeight                           120
        dwMinBitRate                   128000
        dwMaxBitRate                   691200
        dwMaxVideoFrameBufferSize       28800
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  0
        dwMinFrameInterval             333333
        dwMaxFrameInterval            9999990
        dwFrameIntervalStep            333333
      VideoStreaming Interface Descriptor:
        bLength                            38
        bDescriptorType                    36
        bDescriptorSubtype                  7 (FRAME_MJPEG)
        bFrameIndex                         4
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            176
        wHeight                           144
        dwMinBitRate                   128000
        dwMaxBitRate                   912384
        dwMaxVideoFrameBufferSize       38016
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  0
        dwMinFrameInterval             333333
        dwMaxFrameInterval            9999990
        dwFrameIntervalStep            333333
      VideoStreaming Interface Descriptor:
        bLength                            38
        bDescriptorType                    36
        bDescriptorSubtype                  7 (FRAME_MJPEG)
        bFrameIndex                         5
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            352
        wHeight                           288
        dwMinBitRate                   128000
        dwMaxBitRate                  3649536
        dwMaxVideoFrameBufferSize      152064
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  0
        dwMinFrameInterval             333333
        dwMaxFrameInterval            9999990
        dwFrameIntervalStep            333333
      VideoStreaming Interface Descriptor:
        bLength                            26
        bDescriptorType                    36
        bDescriptorSubtype                  3 (STILL_IMAGE_FRAME)
        bEndpointAddress                    0
        bNumImageSizePatterns               5
        wWidth( 0)                        640
        wHeight( 0)                       480
        wWidth( 1)                        320
        wHeight( 1)                       240
        wWidth( 2)                        160
        wHeight( 2)                       120
        wWidth( 3)                        176
        wHeight( 3)                       144
        wWidth( 4)                        352
        wHeight( 4)                       288
        bNumCompressionPatterns             5
      VideoStreaming Interface Descriptor:
        bLength                             6
        bDescriptorType                    36
        bDescriptorSubtype                 13 (COLORFORMAT)
        bColorPrimaries                     1 (BT.709,sRGB)
        bTransferCharacteristics            1 (BT.709)
        bMatrixCoefficients                 4 (SMPTE 170M (BT.601))
      VideoStreaming Interface Descriptor:
        bLength                            27
        bDescriptorType                    36
        bDescriptorSubtype                  4 (FORMAT_UNCOMPRESSED)
        bFormatIndex                        2
        bNumFrameDescriptors                5
        guidFormat                            {59555932-0000-1000-8000-00aa00389b71}
        bBitsPerPixel                      16
        bDefaultFrameIndex                  1
        bAspectRatioX                       0
        bAspectRatioY                       0
        bmInterlaceFlags                 0x00
          Interlaced stream or variable: No
          Fields per frame: 2 fields
          Field 1 first: No
          Field pattern: Field 1 only
          bCopyProtect                      0
      VideoStreaming Interface Descriptor:
        bLength                            50
        bDescriptorType                    36
        bDescriptorSubtype                  5 (FRAME_UNCOMPRESSED)
        bFrameIndex                         1
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            640
        wHeight                           480
        dwMinBitRate                   614400
        dwMaxBitRate                 18432000
        dwMaxVideoFrameBufferSize      614400
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  6
        dwFrameInterval( 0)            333333
        dwFrameInterval( 1)            500000
        dwFrameInterval( 2)            666666
        dwFrameInterval( 3)           1000000
        dwFrameInterval( 4)           2000000
        dwFrameInterval( 5)          10000000
      VideoStreaming Interface Descriptor:
        bLength                            50
        bDescriptorType                    36
        bDescriptorSubtype                  5 (FRAME_UNCOMPRESSED)
        bFrameIndex                         2
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            320
        wHeight                           240
        dwMinBitRate                   153600
        dwMaxBitRate                  4608000
        dwMaxVideoFrameBufferSize      153600
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  6
        dwFrameInterval( 0)            333333
        dwFrameInterval( 1)            500000
        dwFrameInterval( 2)            666666
        dwFrameInterval( 3)           1000000
        dwFrameInterval( 4)           1016960
        dwFrameInterval( 5)          10000000
      VideoStreaming Interface Descriptor:
        bLength                            50
        bDescriptorType                    36
        bDescriptorSubtype                  5 (FRAME_UNCOMPRESSED)
        bFrameIndex                         3
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            160
        wHeight                           120
        dwMinBitRate                    38400
        dwMaxBitRate                  1152000
        dwMaxVideoFrameBufferSize       38400
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  6
        dwFrameInterval( 0)            333333
        dwFrameInterval( 1)            500000
        dwFrameInterval( 2)            666666
        dwFrameInterval( 3)           1000000
        dwFrameInterval( 4)           1016960
        dwFrameInterval( 5)          10000000
      VideoStreaming Interface Descriptor:
        bLength                            50
        bDescriptorType                    36
        bDescriptorSubtype                  5 (FRAME_UNCOMPRESSED)
        bFrameIndex                         4
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            176
        wHeight                           144
        dwMinBitRate                    50688
        dwMaxBitRate                  1520640
        dwMaxVideoFrameBufferSize       50688
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  6
        dwFrameInterval( 0)            333333
        dwFrameInterval( 1)            500000
        dwFrameInterval( 2)            666666
        dwFrameInterval( 3)           1000000
        dwFrameInterval( 4)           1016960
        dwFrameInterval( 5)          10000000
      VideoStreaming Interface Descriptor:
        bLength                            50
        bDescriptorType                    36
        bDescriptorSubtype                  5 (FRAME_UNCOMPRESSED)
        bFrameIndex                         5
        bmCapabilities                   0x00
          Still image unsupported
        wWidth                            352
        wHeight                           288
        dwMinBitRate                   202752
        dwMaxBitRate                  6082560
        dwMaxVideoFrameBufferSize      202752
        dwDefaultFrameInterval         333333
        bFrameIntervalType                  6
        dwFrameInterval( 0)            333333
        dwFrameInterval( 1)            500000
        dwFrameInterval( 2)            666666
        dwFrameInterval( 3)           1000000
        dwFrameInterval( 4)           1016960
        dwFrameInterval( 5)          10000000
      VideoStreaming Interface Descriptor:
        bLength                            26
        bDescriptorType                    36
        bDescriptorSubtype                  3 (STILL_IMAGE_FRAME)
        bEndpointAddress                    0
        bNumImageSizePatterns               5
        wWidth( 0)                        640
        wHeight( 0)                       480
        wWidth( 1)                        320
        wHeight( 1)                       240
        wWidth( 2)                        160
        wHeight( 2)                       120
        wWidth( 3)                        176
        wHeight( 3)                       144
        wWidth( 4)                        352
        wHeight( 4)                       288
        bNumCompressionPatterns             5
      VideoStreaming Interface Descriptor:
        bLength                             6
        bDescriptorType                    36
        bDescriptorSubtype                 13 (COLORFORMAT)
        bColorPrimaries                     1 (BT.709,sRGB)
        bTransferCharacteristics            1 (BT.709)
        bMatrixCoefficients                 4 (SMPTE 170M (BT.601))
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       1
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            5
          Transfer Type            Isochronous
          Synch Type               Asynchronous
          Usage Type               Data
        wMaxPacketSize     0x0a60  2x 608 bytes
        bInterval               1
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       2
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            5
          Transfer Type            Isochronous
          Synch Type               Asynchronous
          Usage Type               Data
        wMaxPacketSize     0x0b00  2x 768 bytes
        bInterval               1
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       3
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            5
          Transfer Type            Isochronous
          Synch Type               Asynchronous
          Usage Type               Data
        wMaxPacketSize     0x0b20  2x 800 bytes
        bInterval               1
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       4
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            5
          Transfer Type            Isochronous
          Synch Type               Asynchronous
          Usage Type               Data
        wMaxPacketSize     0x1300  3x 768 bytes
        bInterval               1
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       5
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            5
          Transfer Type            Isochronous
          Synch Type               Asynchronous
          Usage Type               Data
        wMaxPacketSize     0x1320  3x 800 bytes
        bInterval               1
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       6
      bNumEndpoints           1
      bInterfaceClass        14 Video
      bInterfaceSubClass      2 Video Streaming
      bInterfaceProtocol      0
      iInterface              0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            5
          Transfer Type            Isochronous
          Synch Type               Asynchronous
          Usage Type               Data
        wMaxPacketSize     0x13fc  3x 1020 bytes
        bInterval               1
```

## Windows Dump Camera USB Descriptor

```
Information for device USB Camera (VID=0x057E PID=0x030A):

Connection Information:
------------------------------
Device current bus speed: HighSpeed
Device supports USB 1.1 specification
Device supports USB 2.0 specification
Device address: 0x0020
Current configuration value: 0x00
Number of open pipes: 0

Device Descriptor:
------------------------------
0x12	bLength
0x01	bDescriptorType
0x0200	bcdUSB
0xEF	bDeviceClass      (Miscellaneous device)
0x02	bDeviceSubClass   
0x01	bDeviceProtocol   
0x40	bMaxPacketSize0   (64 bytes)
0x057E	idVendor
0x030A	idProduct
0x0924	bcdDevice
0x30	iManufacturer   "Guillemot Corporation"
0x60	iProduct   "USB Camera"
0x00	iSerialNumber
0x01	bNumConfigurations

Device Qualifier Descriptor:
------------------------------
0x0A	bLength
0x06	bDescriptorType
0x0200	bcdUSB
0xEF	bDeviceClass      (Miscellaneous device)
0x02	bDeviceSubClass   
0x01	bDeviceProtocol   
0x40	bMaxPacketSize0   (64 bytes)
0x01	bNumConfigurations 
0x00	bReserved 

Configuration Descriptor:
------------------------------
0x09	bLength
0x02	bDescriptorType
0x0309	wTotalLength   (777 bytes)
0x02	bNumInterfaces
0x01	bConfigurationValue
0x30	iConfiguration   "Guillemot Corporation"
0x80	bmAttributes   (Bus-powered Device)
0xFA	bMaxPower      (500 mA)

Interface Association Descriptor:
------------------------------
0x08	bLength
0x0B	bDescriptorType
0x00	bFirstInterface
0x02	bInterfaceCount
0x0E	bFunctionClass      (Video Device Class)
0x03	bFunctionSubClass   
0x00	bFunctionProtocol   
0x60	iFunction   "USB Camera"

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x00	bInterfaceNumber
0x00	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x01	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x60	iInterface   "USB Camera"

Unknown Descriptor:
------------------------------
0x0D	bLength
0x24	bDescriptorType
Hex dump: 
0x0D 0x24 0x01 0x00 0x01 0x4D 0x00 0xC0 0xE1 0xE4 
0x00 0x01 0x01 

Unknown Descriptor:
------------------------------
0x09	bLength
0x24	bDescriptorType
Hex dump: 
0x09 0x24 0x03 0x02 0x01 0x01 0x00 0x04 0x00 

Unknown Descriptor:
------------------------------
0x1A	bLength
0x24	bDescriptorType
Hex dump: 
0x1A 0x24 0x06 0x04 0xF0 0x77 0x35 0xD1 0x89 0x8D 
0x00 0x47 0x81 0x2E 0x7D 0xD5 0xE2 0xFD 0xB8 0x98 
0x08 0x01 0x03 0x01 0xFF 0x00 

Unknown Descriptor:
------------------------------
0x12	bLength
0x24	bDescriptorType
Hex dump: 
0x12 0x24 0x02 0x01 0x01 0x02 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x03 0x0A 0x02 0x00 

Unknown Descriptor:
------------------------------
0x0B	bLength
0x24	bDescriptorType
Hex dump: 
0x0B 0x24 0x05 0x03 0x01 0x00 0x00 0x02 0x7F 0x15 
0x00 

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x82	bEndpointAddress  (IN endpoint 2)
0x03	bmAttributes      (Transfer: Interrupt / Synch: None / Usage: Data)
0x0010	wMaxPacketSize    (1 x 16 bytes)
0x06	bInterval         (32 microframes)

Unknown Descriptor:
------------------------------
0x05	bLength
0x25	bDescriptorType
Hex dump: 
0x05 0x25 0x03 0x10 0x00 

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x00	bAlternateSetting
0x00	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Unknown Descriptor:
------------------------------
0x0F	bLength
0x24	bDescriptorType
Hex dump: 
0x0F 0x24 0x01 0x02 0x2D 0x02 0x81 0x00 0x02 0x02 
0x01 0x00 0x01 0x00 0x00 

Unknown Descriptor:
------------------------------
0x0B	bLength
0x24	bDescriptorType
Hex dump: 
0x0B 0x24 0x06 0x01 0x05 0x01 0x01 0x00 0x00 0x00 
0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x01 0x00 0x80 0x02 0xE0 0x01 0x00 
0xF4 0x01 0x00 0x00 0xC0 0xA8 0x00 0x00 0x08 0x07 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x02 0x00 0x40 0x01 0xF0 0x00 0x00 
0xF4 0x01 0x00 0x00 0x30 0x2A 0x00 0x00 0xC2 0x01 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x03 0x00 0xA0 0x00 0x78 0x00 0x00 
0xF4 0x01 0x00 0x00 0x8C 0x0A 0x00 0x80 0x70 0x00 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x04 0x00 0xB0 0x00 0x90 0x00 0x00 
0xF4 0x01 0x00 0x00 0xEC 0x0D 0x00 0x80 0x94 0x00 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x05 0x00 0x60 0x01 0x20 0x01 0x00 
0xF4 0x01 0x00 0x00 0xB0 0x37 0x00 0x00 0x52 0x02 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x1A	bLength
0x24	bDescriptorType
Hex dump: 
0x1A 0x24 0x03 0x00 0x05 0x80 0x02 0xE0 0x01 0x40 
0x01 0xF0 0x00 0xA0 0x00 0x78 0x00 0xB0 0x00 0x90 
0x00 0x60 0x01 0x20 0x01 0x00 

Unknown Descriptor:
------------------------------
0x06	bLength
0x24	bDescriptorType
Hex dump: 
0x06 0x24 0x0D 0x01 0x01 0x04 

Unknown Descriptor:
------------------------------
0x1B	bLength
0x24	bDescriptorType
Hex dump: 
0x1B 0x24 0x04 0x02 0x05 0x59 0x55 0x59 0x32 0x00 
0x00 0x10 0x00 0x80 0x00 0x00 0xAA 0x00 0x38 0x9B 
0x71 0x10 0x01 0x00 0x00 0x00 0x00 

Unknown Descriptor:
------------------------------
0x32	bLength
0x24	bDescriptorType
Hex dump: 
0x32 0x24 0x05 0x01 0x00 0x80 0x02 0xE0 0x01 0x00 
0x60 0x09 0x00 0x00 0x40 0x19 0x01 0x00 0x60 0x09 
0x00 0x15 0x16 0x05 0x00 0x06 0x15 0x16 0x05 0x00 
0x20 0xA1 0x07 0x00 0x2A 0x2C 0x0A 0x00 0x40 0x42 
0x0F 0x00 0x80 0x84 0x1E 0x00 0x80 0x96 0x98 0x00 

Unknown Descriptor:
------------------------------
0x32	bLength
0x24	bDescriptorType
Hex dump: 
0x32 0x24 0x05 0x02 0x00 0x40 0x01 0xF0 0x00 0x00 
0x58 0x02 0x00 0x00 0x50 0x46 0x00 0x00 0x58 0x02 
0x00 0x15 0x16 0x05 0x00 0x06 0x15 0x16 0x05 0x00 
0x20 0xA1 0x07 0x00 0x2A 0x2C 0x0A 0x00 0x40 0x42 
0x0F 0x00 0x80 0x84 0x0F 0x00 0x80 0x96 0x98 0x00 

Unknown Descriptor:
------------------------------
0x32	bLength
0x24	bDescriptorType
Hex dump: 
0x32 0x24 0x05 0x03 0x00 0xA0 0x00 0x78 0x00 0x00 
0x96 0x00 0x00 0x00 0x94 0x11 0x00 0x00 0x96 0x00 
0x00 0x15 0x16 0x05 0x00 0x06 0x15 0x16 0x05 0x00 
0x20 0xA1 0x07 0x00 0x2A 0x2C 0x0A 0x00 0x40 0x42 
0x0F 0x00 0x80 0x84 0x0F 0x00 0x80 0x96 0x98 0x00 

Unknown Descriptor:
------------------------------
0x32	bLength
0x24	bDescriptorType
Hex dump: 
0x32 0x24 0x05 0x04 0x00 0xB0 0x00 0x90 0x00 0x00 
0xC6 0x00 0x00 0x00 0x34 0x17 0x00 0x00 0xC6 0x00 
0x00 0x15 0x16 0x05 0x00 0x06 0x15 0x16 0x05 0x00 
0x20 0xA1 0x07 0x00 0x2A 0x2C 0x0A 0x00 0x40 0x42 
0x0F 0x00 0x80 0x84 0x0F 0x00 0x80 0x96 0x98 0x00 

Unknown Descriptor:
------------------------------
0x32	bLength
0x24	bDescriptorType
Hex dump: 
0x32 0x24 0x05 0x05 0x00 0x60 0x01 0x20 0x01 0x00 
0x18 0x03 0x00 0x00 0xD0 0x5C 0x00 0x00 0x18 0x03 
0x00 0x15 0x16 0x05 0x00 0x06 0x15 0x16 0x05 0x00 
0x20 0xA1 0x07 0x00 0x2A 0x2C 0x0A 0x00 0x40 0x42 
0x0F 0x00 0x80 0x84 0x0F 0x00 0x80 0x96 0x98 0x00 

Unknown Descriptor:
------------------------------
0x1A	bLength
0x24	bDescriptorType
Hex dump: 
0x1A 0x24 0x03 0x00 0x05 0x80 0x02 0xE0 0x01 0x40 
0x01 0xF0 0x00 0xA0 0x00 0x78 0x00 0xB0 0x00 0x90 
0x00 0x60 0x01 0x20 0x01 0x00 

Unknown Descriptor:
------------------------------
0x06	bLength
0x24	bDescriptorType
Hex dump: 
0x06 0x24 0x0D 0x01 0x01 0x04 

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x01	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0A60	wMaxPacketSize    (2 x 608 bytes)
0x01	bInterval         (1 microframes)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x02	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0B00	wMaxPacketSize    (2 x 768 bytes)
0x01	bInterval         (1 microframes)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x03	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0B20	wMaxPacketSize    (2 x 800 bytes)
0x01	bInterval         (1 microframes)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x04	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x1300	wMaxPacketSize    (3 x 768 bytes)
0x01	bInterval         (1 microframes)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x05	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x1320	wMaxPacketSize    (3 x 800 bytes)
0x01	bInterval         (1 microframes)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x06	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x13FC	wMaxPacketSize    (3 x 1020 bytes)
0x01	bInterval         (1 microframes)

Other Speed Configuration Descriptor:
------------------------------
0x09	bLength
0x07	bDescriptorType
0x026A	wTotalLength   (618 bytes)
0x04	bNumInterfaces
0x01	bConfigurationValue
0x30	iConfiguration   "Guillemot Corporation"
0x80	bmAttributes   (Bus-powered Device)
0xFA	bMaxPower      (500 mA)

Interface Association Descriptor:
------------------------------
0x08	bLength
0x0B	bDescriptorType
0x00	bFirstInterface
0x02	bInterfaceCount
0x0E	bFunctionClass      (Video Device Class)
0x03	bFunctionSubClass   
0x00	bFunctionProtocol   
0x60	iFunction   "USB Camera"

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x00	bInterfaceNumber
0x00	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x01	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x60	iInterface   "USB Camera"

Unknown Descriptor:
------------------------------
0x0D	bLength
0x24	bDescriptorType
Hex dump: 
0x0D 0x24 0x01 0x00 0x01 0x4D 0x00 0x38 0x9C 0x1C 
0x00 0x01 0x01 

Unknown Descriptor:
------------------------------
0x09	bLength
0x24	bDescriptorType
Hex dump: 
0x09 0x24 0x03 0x02 0x01 0x01 0x00 0x04 0x00 

Unknown Descriptor:
------------------------------
0x1A	bLength
0x24	bDescriptorType
Hex dump: 
0x1A 0x24 0x06 0x04 0x70 0x33 0xF0 0x28 0x11 0x63 
0x2E 0x4A 0xBA 0x2C 0x68 0x90 0xEB 0x33 0x40 0x16 
0x01 0x01 0x03 0x01 0x01 0x00 

Unknown Descriptor:
------------------------------
0x12	bLength
0x24	bDescriptorType
Hex dump: 
0x12 0x24 0x02 0x01 0x01 0x02 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x03 0x00 0x02 0x00 

Unknown Descriptor:
------------------------------
0x0B	bLength
0x24	bDescriptorType
Hex dump: 
0x0B 0x24 0x05 0x03 0x01 0x00 0x00 0x02 0x7F 0x05 
0x00 

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x82	bEndpointAddress  (IN endpoint 2)
0x03	bmAttributes      (Transfer: Interrupt / Synch: None / Usage: Data)
0x0010	wMaxPacketSize    (1 x 16 bytes)
0x06	bInterval         (6 frames)

Unknown Descriptor:
------------------------------
0x05	bLength
0x25	bDescriptorType
Hex dump: 
0x05 0x25 0x03 0x10 0x00 

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x00	bAlternateSetting
0x00	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Unknown Descriptor:
------------------------------
0x0E	bLength
0x24	bDescriptorType
Hex dump: 
0x0E 0x24 0x01 0x01 0xF8 0x00 0x81 0x00 0x02 0x02 
0x01 0x00 0x01 0x00 

Unknown Descriptor:
------------------------------
0x0B	bLength
0x24	bDescriptorType
Hex dump: 
0x0B 0x24 0x06 0x01 0x05 0x01 0x01 0x00 0x00 0x00 
0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x01 0x00 0x80 0x02 0xE0 0x01 0x00 
0xF4 0x01 0x00 0x00 0xC0 0xA8 0x00 0x00 0x08 0x07 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x02 0x00 0x40 0x01 0xF0 0x00 0x00 
0xF4 0x01 0x00 0x00 0x30 0x2A 0x00 0x00 0xC2 0x01 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x03 0x00 0xA0 0x00 0x78 0x00 0x00 
0xF4 0x01 0x00 0x00 0x8C 0x0A 0x00 0x80 0x70 0x00 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x04 0x00 0xB0 0x00 0x90 0x00 0x00 
0xF4 0x01 0x00 0x00 0xEC 0x0D 0x00 0x80 0x94 0x00 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x26	bLength
0x24	bDescriptorType
Hex dump: 
0x26 0x24 0x07 0x05 0x00 0x60 0x01 0x20 0x01 0x00 
0xF4 0x01 0x00 0x00 0xB0 0x37 0x00 0x00 0x52 0x02 
0x00 0x15 0x16 0x05 0x00 0x00 0x15 0x16 0x05 0x00 
0x76 0x96 0x98 0x00 0x15 0x16 0x05 0x00 

Unknown Descriptor:
------------------------------
0x1B	bLength
0x24	bDescriptorType
Hex dump: 
0x1B 0x24 0x03 0x00 0x05 0x80 0x02 0xE0 0x01 0xA0 
0x00 0x78 0x00 0xB0 0x00 0x90 0x00 0x40 0x01 0xF0 
0x00 0x60 0x01 0x20 0x01 0x01 0x64 

Unknown Descriptor:
------------------------------
0x06	bLength
0x24	bDescriptorType
Hex dump: 
0x06 0x24 0x0D 0x01 0x01 0x04 

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x01	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0300	wMaxPacketSize    (1 x 768 bytes)
0x01	bInterval         (1 frames)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x02	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0320	wMaxPacketSize    (1 x 800 bytes)
0x01	bInterval         (1 frames)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x03	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0350	wMaxPacketSize    (1 x 848 bytes)
0x01	bInterval         (1 frames)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x04	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0380	wMaxPacketSize    (1 x 896 bytes)
0x01	bInterval         (1 frames)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x05	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x039F	wMaxPacketSize    (1 x 927 bytes)
0x01	bInterval         (1 frames)

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x01	bInterfaceNumber
0x06	bAlternateSetting
0x01	bNumEndPoints
0x0E	bInterfaceClass      (Video Device Class)
0x02	bInterfaceSubClass   
0x00	bInterfaceProtocol   
0x00	iInterface

Endpoint Descriptor:
------------------------------
0x07	bLength
0x05	bDescriptorType
0x81	bEndpointAddress  (IN endpoint 1)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x03FF	wMaxPacketSize    (1 x 1023 bytes)
0x01	bInterval         (1 frames)

Interface Association Descriptor:
------------------------------
0x08	bLength
0x0B	bDescriptorType
0x02	bFirstInterface
0x02	bInterfaceCount
0x01	bFunctionClass      (Audio Device Class)
0x00	bFunctionSubClass   
0x00	bFunctionProtocol   
0x40	iFunction   "Audio_Device"

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x02	bInterfaceNumber
0x00	bAlternateSetting
0x00	bNumEndPoints
0x01	bInterfaceClass      (Audio Device Class)
0x01	bInterfaceSubClass   (Audio Control Interface)
0x00	bInterfaceProtocol   
0x40	iInterface   "Audio_Device"

AC Interface Header Descriptor:
------------------------------
0x09	bLength
0x24	bDescriptorType
0x01	bDescriptorSubtype
0x0100	bcdADC
0x0026	wTotalLength   (38 bytes)
0x01	bInCollection
0x03	baInterfaceNr(1)

AC Input Terminal Descriptor:
------------------------------
0x0C	bLength
0x24	bDescriptorType
0x02	bDescriptorSubtype
0x01	bTerminalID
0x0201	wTerminalType   (Microphone)
0x00	bAssocTerminal
0x01	bNrChannels   (1 channels)
0x0000	wChannelConfig
0x00	iChannelNames
0x00	iTerminal

AC Output Terminal Descriptor:
------------------------------
0x09	bLength
0x24	bDescriptorType
0x03	bDescriptorSubtype
0x03	bTerminalID
0x0101	wTerminalType   (USB Streaming)
0x00	bAssocTerminal
0x05	bSourceID
0x00	iTerminal

AC Feature Unit Descriptor:
------------------------------
0x08	bLength
0x24	bDescriptorType
0x06	bDescriptorSubtype
0x05	bUnitID
0x01	bSourceID
0x01	bControlSize
bmaControls: 
 0x03	Channel(0)
0x00	iFeature


Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x03	bInterfaceNumber
0x00	bAlternateSetting
0x00	bNumEndPoints
0x01	bInterfaceClass      (Audio Device Class)
0x02	bInterfaceSubClass   (Audio Streaming Interface)
0x00	bInterfaceProtocol   
0x00	iInterface

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x03	bInterfaceNumber
0x01	bAlternateSetting
0x01	bNumEndPoints
0x01	bInterfaceClass      (Audio Device Class)
0x02	bInterfaceSubClass   (Audio Streaming Interface)
0x00	bInterfaceProtocol   
0x00	iInterface

AS Interface Descriptor:
------------------------------
0x07	bLength
0x24	bDescriptorType
0x01	bDescriptorSubtype
0x03	bTerminalLink
0x01	bDelay
0x0001	wFormatTag   (PCM)

AS Format Type 1 Descriptor:
------------------------------
0x0B	bLength
0x24	bDescriptorType
0x02	bDescriptorSubtype
0x01	bFormatType   (FORMAT_TYPE_1)
0x01	bNrChannels   (1 channels)
0x02	bSubframeSize
0x10	bBitResolution   (16 bits per sample)
0x01	bSamFreqType   (Discrete sampling frequencies)
0x003E80 	tSamFreq(1)   (16000 Hz)

Endpoint Descriptor (Audio/MIDI 1.0):
------------------------------
0x09	bLength
0x05	bDescriptorType
0x83	bEndpointAddress  (IN endpoint 3)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0020	wMaxPacketSize    (1 x 32 bytes)
0x04	bInterval         (8 frames)
0x00	bRefresh
0x00	bSynchAddress

AS Isochronous Data Endpoint Descriptor:
------------------------------
0x07	bLength
0x25	bDescriptorType
0x01	bDescriptorSubtype
0x01	bmAttributes   (Sampling Frequency)
0x00	bLockDelayUnits   (undefined)
0x0000	wLockDelay

Interface Descriptor:
------------------------------
0x09	bLength
0x04	bDescriptorType
0x03	bInterfaceNumber
0x02	bAlternateSetting
0x01	bNumEndPoints
0x01	bInterfaceClass      (Audio Device Class)
0x02	bInterfaceSubClass   (Audio Streaming Interface)
0x00	bInterfaceProtocol   
0x00	iInterface

AS Interface Descriptor:
------------------------------
0x07	bLength
0x24	bDescriptorType
0x01	bDescriptorSubtype
0x03	bTerminalLink
0x01	bDelay
0x0001	wFormatTag   (PCM)

AS Format Type 1 Descriptor:
------------------------------
0x0B	bLength
0x24	bDescriptorType
0x02	bDescriptorSubtype
0x01	bFormatType   (FORMAT_TYPE_1)
0x01	bNrChannels   (1 channels)
0x02	bSubframeSize
0x10	bBitResolution   (16 bits per sample)
0x01	bSamFreqType   (Discrete sampling frequencies)
0x00BB80 	tSamFreq(1)   (48000 Hz)

Endpoint Descriptor (Audio/MIDI 1.0):
------------------------------
0x09	bLength
0x05	bDescriptorType
0x83	bEndpointAddress  (IN endpoint 3)
0x05	bmAttributes      (Transfer: Isochronous / Synch: Asynchronous / Usage: Data)
0x0060	wMaxPacketSize    (1 x 96 bytes)
0x04	bInterval         (8 frames)
0x00	bRefresh
0x00	bSynchAddress

AS Isochronous Data Endpoint Descriptor:
------------------------------
0x07	bLength
0x25	bDescriptorType
0x01	bDescriptorSubtype
0x01	bmAttributes   (Sampling Frequency)
0x00	bLockDelayUnits   (undefined)
0x0000	wLockDelay

Microsoft OS Descriptor is not available. Error code: 0x0000001F

String Descriptor Table
--------------------------------
Index  LANGID  String
0x00   0x0000  0x0409 
0x30   0x0409  "Guillemot Corporation"
0x60   0x0409  "USB Camera"
0x40   0x0409  "Audio_Device"

------------------------------

Connection path for device: 
符合 USB xHCI 的主机控制器
Root Hub
USB Camera (VID=0x057E PID=0x030A) Port: 3

Running on: Windows 10 or greater

Brought to you by TDD v2.11.0, Mar 26 2018, 09:54:50
```

## VC 架构

OUTPUT_TERMINAL(bSourceID = 4) <-- EXTENSION_UNIT(bUnitID = 4, baSourceID = 3) <-- PROCESSING_UNIT(bUnitID = 3, bSourceID = 1) <-- INPUT_TERMINAL(bTerminalID = 1)
