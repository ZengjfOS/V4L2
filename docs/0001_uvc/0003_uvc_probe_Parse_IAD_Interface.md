# uvc probe Parse IAD Interface

UVC设备都是多Interface设备，这点同普通的u盘不同。UVC设备最起码有两个Interface，VideoControl（VC）Interface和VideoStream(VS) Interface； 这也是最常见的UVC设备。 Spec明确要求一个具有可用的，具有实际UVC功能的设备要有一个VC Interface，一个或多个VS Interface。

* `uvc_parse_control`解析IAD接口中的VC Interface，并从VC Interface获取VS Interface索引，通过`uvc_parse_streaming`解析出VS Interface；
* 最终VC的Terminal/Unit都被加载到`struct uvc_device`的entities链表上去了；

## 参考文档

* [Universal Serial Bus Device Class Definition for Video Devices](https://www.spinelelectronics.com/pdf/UVC%201.5%20Class%20specification.pdf)

## 代码分析

* uvc_probe
  ```C
  static int uvc_probe(struct usb_interface *intf,
               const struct usb_device_id *id)
  {
      struct usb_device *udev = interface_to_usbdev(intf);                          // 也就是说当前接口是VC控制接口
      struct uvc_device *dev;
      const struct uvc_device_info *info =
          (const struct uvc_device_info *)id->driver_info;
      int function;
      int ret;
  
      if (id->idVendor && id->idProduct)                                          // 匹配的厂家和产品id
          uvc_trace(UVC_TRACE_PROBE, "Probing known UVC device %s "
                  "(%04x:%04x)\n", udev->devpath, id->idVendor,
                  id->idProduct);
      else
          uvc_trace(UVC_TRACE_PROBE, "Probing generic UVC device %s\n",
                  udev->devpath);
  
      /* Allocate memory for the device and initialize it. */
      dev = kzalloc(sizeof(*dev), GFP_KERNEL);                                    // 分配一个uvc设备
      if (dev == NULL)
          return -ENOMEM;
  
      INIT_LIST_HEAD(&dev->entities);                                             // 初始化dev的list
      INIT_LIST_HEAD(&dev->chains);
      INIT_LIST_HEAD(&dev->streams);
      kref_init(&dev->ref);
      atomic_set(&dev->nmappings, 0);
      mutex_init(&dev->lock);
  
      dev->udev = usb_get_dev(udev);                                              // 将一些信息绑定到uvc设备中，udev是指usb设备，这里的dev是uvc设备，就是将usb普通设备升级为uvc设备
      dev->intf = usb_get_intf(intf);
      dev->intfnum = intf->cur_altsetting->desc.bInterfaceNumber;
      dev->info = info ? info : &uvc_quirk_none;                                  // ids中的信息
      dev->quirks = uvc_quirks_param == -1
              ? dev->info->quirks : uvc_quirks_param;
  
      if (udev->product != NULL)                                                  // 假装未NULL
          strscpy(dev->name, udev->product, sizeof(dev->name));
      else
          snprintf(dev->name, sizeof(dev->name),
               "UVC Camera (%04x:%04x)",
               le16_to_cpu(udev->descriptor.idVendor),
               le16_to_cpu(udev->descriptor.idProduct));
  
      /*
       * Add iFunction or iInterface to names when available as additional
       * distinguishers between interfaces. iFunction is prioritized over
       * iInterface which matches Windows behavior at the point of writing.
       */
      if (intf->intf_assoc && intf->intf_assoc->iFunction != 0)
          function = intf->intf_assoc->iFunction;
      else
          function = intf->cur_altsetting->desc.iInterface;
      if (function != 0) {
          size_t len;
  
          strlcat(dev->name, ": ", sizeof(dev->name));
          len = strlen(dev->name);
          usb_string(udev, function, dev->name + len,
                 sizeof(dev->name) - len);
      }
  
      /* Parse the Video Class control descriptor. */
      if (uvc_parse_control(dev) < 0) {                                       // <--- uvc解析usb视频类VC控制描述符
          uvc_trace(UVC_TRACE_PROBE, "Unable to parse UVC "
              "descriptors.\n");
          goto error;
      }
  
      uvc_printk(KERN_INFO, "Found UVC %u.%02x device %s (%04x:%04x)\n",
          dev->uvc_version >> 8, dev->uvc_version & 0xff,
          udev->product ? udev->product : "<unnamed>",
          le16_to_cpu(udev->descriptor.idVendor),
          le16_to_cpu(udev->descriptor.idProduct));
  
      if (dev->quirks != dev->info->quirks) {
          uvc_printk(KERN_INFO, "Forcing device quirks to 0x%x by module "
              "parameter for testing purpose.\n", dev->quirks);
          uvc_printk(KERN_INFO, "Please report required quirks to the "
              "linux-uvc-devel mailing list.\n");
      }
  
      /* Initialize the media device and register the V4L2 device. */
  #ifdef CONFIG_MEDIA_CONTROLLER
      dev->mdev.dev = &intf->dev;
      strscpy(dev->mdev.model, dev->name, sizeof(dev->mdev.model));
      if (udev->serial)
          strscpy(dev->mdev.serial, udev->serial,
              sizeof(dev->mdev.serial));
      strscpy(dev->mdev.bus_info, udev->devpath, sizeof(dev->mdev.bus_info));
      dev->mdev.hw_revision = le16_to_cpu(udev->descriptor.bcdDevice);
      media_device_init(&dev->mdev);
  
      dev->vdev.mdev = &dev->mdev;
  #endif
      if (v4l2_device_register(&intf->dev, &dev->vdev) < 0)                   // 注册为v4l2设备
          goto error;
  
      /* Initialize controls. */
      if (uvc_ctrl_init_device(dev) < 0)
          goto error;
  
      /* Scan the device for video chains. */
      if (uvc_scan_device(dev) < 0)
          goto error;
  
      /* Register video device nodes. */
      if (uvc_register_chains(dev) < 0)
          goto error;
  
  #ifdef CONFIG_MEDIA_CONTROLLER
      /* Register the media device node */
      if (media_device_register(&dev->mdev) < 0)
          goto error;
  #endif
      /* Save our data pointer in the interface data. */
      usb_set_intfdata(intf, dev);
  
      /* Initialize the interrupt URB. */
      if ((ret = uvc_status_init(dev)) < 0) {
          uvc_printk(KERN_INFO, "Unable to initialize the status "
              "endpoint (%d), status interrupt will not be "
              "supported.\n", ret);
      }
  
      uvc_trace(UVC_TRACE_PROBE, "UVC device initialized.\n");
      usb_enable_autosuspend(udev);
      return 0;
  
  error:
      uvc_unregister_video(dev);
      kref_put(&dev->ref, uvc_delete);
      return -ENODEV;
  }
  ```
* uvc_parse_control
  ```C
  static int uvc_parse_control(struct uvc_device *dev)
  {
      struct usb_host_interface *alts = dev->intf->cur_altsetting;
      unsigned char *buffer = alts->extra;                                          // UVC描述符藏在这里面，标准就是在这里面
      int buflen = alts->extralen;                                                  // UVC描述符的长度
      int ret;
  
      /* Parse the default alternate setting only, as the UVC specification
       * defines a single alternate setting, the default alternate setting
       * zero.
       */
  
      while (buflen > 2) {                                                          // 这里会循环解析，直到解析完VC
          if (uvc_parse_vendor_control(dev, buffer, buflen) ||                      // 解析厂商特殊控制，解析vendor-specific控制描述符,这里貌似只有Logitech厂商的.
              buffer[1] != USB_DT_CS_INTERFACE)
              goto next_descriptor;
  
          /**
           * 这个设备包含了一个Processing Unit、一个Input Terminal和Output Terminal，
           * VC Interface需要对它们下发不同的命令，为此VC Interface需要包含一个控制端点（强制性要求），
           * 它使用的就是每个USB设备中默认的端点0。
           */
          if ((ret = uvc_parse_standard_control(dev, buffer, buflen)) < 0)          // 解析uvc标准控制
              return ret;
  
  next_descriptor:
          buflen -= buffer[0];                                                      // 进行偏移
          buffer += buffer[0];                                                      // 进行偏移
      }
  
      /* Check if the optional status endpoint is present. Built-in iSight
       * webcams have an interrupt endpoint but spit proprietary data that
       * don't conform to the UVC status endpoint messages. Don't try to
       * handle the interrupt endpoint for those cameras.
       */
       /**
        * 另外一个Interrupt端点则是可选的，用来返回或通知Host端当前的UVC设备内部状态有变化。
        * 大部分情况下，一个UVC设备的VC Interface不需一定要实现此端点，但一旦UVC设备需要实现
        * 某些特定feature时，Spec会强制性要求实现该interrupt端点。
        */
      if (alts->desc.bNumEndpoints == 1 &&                                  // 判断描述符是否有1个端点
          !(dev->quirks & UVC_QUIRK_BUILTIN_ISIGHT)) {
          struct usb_host_endpoint *ep = &alts->endpoint[0];                // 获取主机端点
          struct usb_endpoint_descriptor *desc = &ep->desc;                 // 获取端点描述符
  
          if (usb_endpoint_is_int_in(desc) &&                               // 判断是否中断输入端点
              le16_to_cpu(desc->wMaxPacketSize) >= 8 &&
              desc->bInterval != 0) {
              uvc_trace(UVC_TRACE_DESCR, "Found a Status endpoint "
                  "(addr %02x).\n", desc->bEndpointAddress);
              dev->int_ep = ep;
          }
      }
  
      return 0;
  }
  ```
* uvc_parse_standard_control：UVC描述符解析，参考[Linux Dump Descriptor](0001_USB_Camera_Descriptor.md#Linux-lsusb-Dump-Camera-USB-Descriptor)
  ```C
  static int uvc_parse_standard_control(struct uvc_device *dev,
      const unsigned char *buffer, int buflen)
  {
      struct usb_device *udev = dev->udev;
      struct uvc_entity *unit, *term;
      struct usb_interface *intf;
      struct usb_host_interface *alts = dev->intf->cur_altsetting;
      unsigned int i, n, p, len;
      u16 type;
  
      switch (buffer[2]) {
      case UVC_VC_HEADER:
          /**
           * buffer[11]: bInCollection
           *
           * The number of VideoStreaming interfaces
           * in the Video Interface Collection to which
           * this VideoControl interface belongs: n
           */
          n = buflen >= 12 ? buffer[11] : 0;
  
          if (buflen < 12 + n) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d HEADER error\n", udev->devnum,
                  alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          dev->uvc_version = get_unaligned_le16(&buffer[3]);
          dev->clock_frequency = get_unaligned_le32(&buffer[7]);
  
          /* Parse all USB Video Streaming interfaces. */
          /**
           * buffer[12...n]: baInterfaceNr(1...n)
           *
           * Interface number of the first VideoStreaming interface in the Collection
           */
          for (i = 0; i < n; ++i) {
              /**
               * 主要判断：if (config->interface[i]->altsetting[0].desc.bInterfaceNumber == ifnum)
               * 用所有的接口的多项选择中的第0个的接口序号来匹配查找
               */
              intf = usb_ifnum_to_if(udev, buffer[12+i]);                       // 通过bInterfaceNumber来找到VC接口对应的VS接口
              if (intf == NULL) {
                  uvc_trace(UVC_TRACE_DESCR, "device %d "
                      "interface %d doesn't exists\n",
                      udev->devnum, i);
                  continue;
              }
  
              uvc_parse_streaming(dev, intf);                                   // 解析出VideoStream(VS) Interface
          }
          break;
  
      case UVC_VC_INPUT_TERMINAL:
          if (buflen < 8) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d INPUT_TERMINAL error\n",
                  udev->devnum, alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          /* Make sure the terminal type MSB is not null, otherwise it
           * could be confused with a unit.
           */
          type = get_unaligned_le16(&buffer[4]);
          if ((type & 0xff00) == 0) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d INPUT_TERMINAL %d has invalid "
                  "type 0x%04x, skipping\n", udev->devnum,
                  alts->desc.bInterfaceNumber,
                  buffer[3], type);
              return 0;
          }
  
          n = 0;
          p = 0;
          len = 8;
  
          if (type == UVC_ITT_CAMERA) {
              n = buflen >= 15 ? buffer[14] : 0;
              len = 15;
  
          } else if (type == UVC_ITT_MEDIA_TRANSPORT_INPUT) {
              n = buflen >= 9 ? buffer[8] : 0;
              p = buflen >= 10 + n ? buffer[9+n] : 0;
              len = 10;
          }
  
          if (buflen < len + n + p) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d INPUT_TERMINAL error\n",
                  udev->devnum, alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          term = uvc_alloc_entity(type | UVC_TERM_INPUT, buffer[3],             // 申请一个entity
                      1, n + p);
          if (term == NULL)
              return -ENOMEM;
  
          if (UVC_ENTITY_TYPE(term) == UVC_ITT_CAMERA) {
              term->camera.bControlSize = n;
              term->camera.bmControls = (u8 *)term + sizeof(*term);
              term->camera.wObjectiveFocalLengthMin =
                  get_unaligned_le16(&buffer[8]);
              term->camera.wObjectiveFocalLengthMax =
                  get_unaligned_le16(&buffer[10]);
              term->camera.wOcularFocalLength =
                  get_unaligned_le16(&buffer[12]);
              memcpy(term->camera.bmControls, &buffer[15], n);
          } else if (UVC_ENTITY_TYPE(term) ==
                 UVC_ITT_MEDIA_TRANSPORT_INPUT) {
              term->media.bControlSize = n;
              term->media.bmControls = (u8 *)term + sizeof(*term);
              term->media.bTransportModeSize = p;
              term->media.bmTransportModes = (u8 *)term
                               + sizeof(*term) + n;
              memcpy(term->media.bmControls, &buffer[9], n);
              memcpy(term->media.bmTransportModes, &buffer[10+n], p);
          }
  
          if (buffer[7] != 0)
              usb_string(udev, buffer[7], term->name,
                     sizeof(term->name));
          else if (UVC_ENTITY_TYPE(term) == UVC_ITT_CAMERA)
              sprintf(term->name, "Camera %u", buffer[3]);
          else if (UVC_ENTITY_TYPE(term) == UVC_ITT_MEDIA_TRANSPORT_INPUT)
              sprintf(term->name, "Media %u", buffer[3]);
          else
              sprintf(term->name, "Input %u", buffer[3]);
  
          list_add_tail(&term->list, &dev->entities);                               // 放入entities链表中
          break;
  
      case UVC_VC_OUTPUT_TERMINAL:
          if (buflen < 9) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d OUTPUT_TERMINAL error\n",
                  udev->devnum, alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          /* Make sure the terminal type MSB is not null, otherwise it
           * could be confused with a unit.
           */
          type = get_unaligned_le16(&buffer[4]);
          if ((type & 0xff00) == 0) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d OUTPUT_TERMINAL %d has invalid "
                  "type 0x%04x, skipping\n", udev->devnum,
                  alts->desc.bInterfaceNumber, buffer[3], type);
              return 0;
          }
  
          term = uvc_alloc_entity(type | UVC_TERM_OUTPUT, buffer[3],                // 申请一个entity
                      1, 0);
          if (term == NULL)
              return -ENOMEM;
  
          memcpy(term->baSourceID, &buffer[7], 1);
  
          if (buffer[8] != 0)
              usb_string(udev, buffer[8], term->name,
                     sizeof(term->name));
          else
              sprintf(term->name, "Output %u", buffer[3]);
  
          list_add_tail(&term->list, &dev->entities);                               // 放入entities中
          break;
  
      case UVC_VC_SELECTOR_UNIT:
          p = buflen >= 5 ? buffer[4] : 0;
  
          if (buflen < 5 || buflen < 6 + p) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d SELECTOR_UNIT error\n",
                  udev->devnum, alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          unit = uvc_alloc_entity(buffer[2], buffer[3], p + 1, 0);                  // 申请entity
          if (unit == NULL)
              return -ENOMEM;
  
          memcpy(unit->baSourceID, &buffer[5], p);
  
          if (buffer[5+p] != 0)
              usb_string(udev, buffer[5+p], unit->name,
                     sizeof(unit->name));
          else
              sprintf(unit->name, "Selector %u", buffer[3]);
  
          list_add_tail(&unit->list, &dev->entities);                               // 放入entities中
          break;
  
      case UVC_VC_PROCESSING_UNIT:
          n = buflen >= 8 ? buffer[7] : 0;
          p = dev->uvc_version >= 0x0110 ? 10 : 9;
  
          if (buflen < p + n) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d PROCESSING_UNIT error\n",
                  udev->devnum, alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          unit = uvc_alloc_entity(buffer[2], buffer[3], 2, n);                      // 申请entity
          if (unit == NULL)
              return -ENOMEM;
  
          memcpy(unit->baSourceID, &buffer[4], 1);
          unit->processing.wMaxMultiplier =
              get_unaligned_le16(&buffer[5]);
          unit->processing.bControlSize = buffer[7];
          unit->processing.bmControls = (u8 *)unit + sizeof(*unit);
          memcpy(unit->processing.bmControls, &buffer[8], n);
          if (dev->uvc_version >= 0x0110)
              unit->processing.bmVideoStandards = buffer[9+n];
  
          if (buffer[8+n] != 0)
              usb_string(udev, buffer[8+n], unit->name,
                     sizeof(unit->name));
          else
              sprintf(unit->name, "Processing %u", buffer[3]);
  
          list_add_tail(&unit->list, &dev->entities);                               // 放入entity链表中
          break;
  
      case UVC_VC_EXTENSION_UNIT:
          p = buflen >= 22 ? buffer[21] : 0;
          n = buflen >= 24 + p ? buffer[22+p] : 0;
  
          if (buflen < 24 + p + n) {
              uvc_trace(UVC_TRACE_DESCR, "device %d videocontrol "
                  "interface %d EXTENSION_UNIT error\n",
                  udev->devnum, alts->desc.bInterfaceNumber);
              return -EINVAL;
          }
  
          unit = uvc_alloc_entity(buffer[2], buffer[3], p + 1, n);                  // 申请entity
          if (unit == NULL)
              return -ENOMEM;
  
          memcpy(unit->extension.guidExtensionCode, &buffer[4], 16);
          unit->extension.bNumControls = buffer[20];
          memcpy(unit->baSourceID, &buffer[22], p);
          unit->extension.bControlSize = buffer[22+p];
          unit->extension.bmControls = (u8 *)unit + sizeof(*unit);
          memcpy(unit->extension.bmControls, &buffer[23+p], n);
  
          if (buffer[23+p+n] != 0)
              usb_string(udev, buffer[23+p+n], unit->name,
                     sizeof(unit->name));
          else
              sprintf(unit->name, "Extension %u", buffer[3]);
  
          list_add_tail(&unit->list, &dev->entities);                               // 放入entity链表中
          break;
  
      default:
          uvc_trace(UVC_TRACE_DESCR, "Found an unknown CS_INTERFACE "
              "descriptor (%u)\n", buffer[2]);
          break;
      }
  
      return 0;
  }
  ```