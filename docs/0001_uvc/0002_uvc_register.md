# uvc register

* 主要是为了知道UVC Camera的驱动入口在哪里，如何知道支持的USB Camera设备有哪些；
* USB Camera的厂家/设备id通过`lsusb`即可获取到；

## 参考文档

* [linux uvc深入理解(二)](https://my.oschina.net/u/2007478/blog/968470)
* [uvc驱动入口](https://github.com/torvalds/linux/blob/master/drivers/media/usb/uvc/uvc_driver.c)

## 代码分析

* [uvc_init](https://elixir.bootlin.com/linux/v5.0-rc7/source/drivers/media/usb/uvc/uvc_driver.c#L2890)
  ```C
  [...省略]
  struct uvc_driver uvc_driver = {
      .driver = {
          .name        = "uvcvideo",
          .probe        = uvc_probe,
          .disconnect    = uvc_disconnect,
          .suspend    = uvc_suspend,
          .resume        = uvc_resume,
          .reset_resume    = uvc_reset_resume,
          .id_table    = uvc_ids,                                   // <------ 支持哪些设备
          .supports_autosuspend = 1,
      },
  };
  
  static int __init uvc_init(void)
  {
      int ret;
  
      uvc_debugfs_init();
  
      ret = usb_register(&uvc_driver.driver);
      if (ret < 0) {
          uvc_debugfs_cleanup();
          return ret;
      }
  
      printk(KERN_INFO DRIVER_DESC " (" DRIVER_VERSION ")\n");
      return 0;
  }
  
  static void __exit uvc_cleanup(void)
  {
      usb_deregister(&uvc_driver.driver);
      uvc_debugfs_cleanup();
  }
  ```
* [uvc_ids](https://elixir.bootlin.com/linux/v5.0-rc7/source/drivers/media/usb/uvc/uvc_driver.c#L2393)
  ```C
  static const struct usb_device_id uvc_ids[] = {
      /* LogiLink Wireless Webcam */
      { .match_flags        = USB_DEVICE_ID_MATCH_DEVICE
                  | USB_DEVICE_ID_MATCH_INT_INFO,
        .idVendor        = 0x0416,
        .idProduct        = 0xa91a,
        .bInterfaceClass    = USB_CLASS_VIDEO,
        .bInterfaceSubClass    = 1,
        .bInterfaceProtocol    = 0,
        .driver_info        = (kernel_ulong_t)&uvc_quirk_probe_minmax },
      [...省略]
  }
  ```