# uvc_parse_streaming

* 最终VS被加载到`struct uvc_device`的streams链表上去了；

```C
static int uvc_parse_streaming(struct uvc_device *dev,
    struct usb_interface *intf)
{
    struct uvc_streaming *streaming = NULL;
    struct uvc_format *format;
    struct uvc_frame *frame;
    struct usb_host_interface *alts = &intf->altsetting[0];                         // 获取第0个多选接口描述
    unsigned char *_buffer, *buffer = alts->extra;                                  // 获取接口UVC定义的数据，主要是为了解析它
    int _buflen, buflen = alts->extralen;
    unsigned int nformats = 0, nframes = 0, nintervals = 0;
    unsigned int size, i, n, p;
    u32 *interval;
    u16 psize;
    int ret = -EINVAL;

    if (intf->cur_altsetting->desc.bInterfaceSubClass                               // 视频流
        != UVC_SC_VIDEOSTREAMING) {
        uvc_trace(UVC_TRACE_DESCR, "device %d interface %d isn't a "
            "video streaming interface\n", dev->udev->devnum,
            intf->altsetting[0].desc.bInterfaceNumber);
        return -EINVAL;
    }

    if (usb_driver_claim_interface(&uvc_driver.driver, intf, dev)) {
        uvc_trace(UVC_TRACE_DESCR, "device %d interface %d is already "
            "claimed\n", dev->udev->devnum,
            intf->altsetting[0].desc.bInterfaceNumber);
        return -EINVAL;
    }

    streaming = uvc_stream_new(dev, intf);                                          // 创建视频流
    if (streaming == NULL) {
        usb_driver_release_interface(&uvc_driver.driver, intf);
        return -ENOMEM;
    }

    /* The Pico iMage webcam has its class-specific interface descriptors
     * after the endpoint descriptors.
     */
    if (buflen == 0) {                                                      // 不等于0，暂不考虑
        for (i = 0; i < alts->desc.bNumEndpoints; ++i) {
            struct usb_host_endpoint *ep = &alts->endpoint[i];

            if (ep->extralen == 0)
                continue;

            if (ep->extralen > 2 &&
                ep->extra[1] == USB_DT_CS_INTERFACE) {
                uvc_trace(UVC_TRACE_DESCR, "trying extra data "
                    "from endpoint %u.\n", i);
                buffer = alts->endpoint[i].extra;
                buflen = alts->endpoint[i].extralen;
                break;
            }
        }
    }

    /* Skip the standard interface descriptors. */
    while (buflen > 2 && buffer[1] != USB_DT_CS_INTERFACE) {
        buflen -= buffer[0];
        buffer += buffer[0];
    }

    if (buflen <= 2) {
        uvc_trace(UVC_TRACE_DESCR, "no class-specific streaming "
            "interface descriptors found.\n");
        goto error;
    }

    /* Parse the header descriptor. */
    switch (buffer[2]) {
    case UVC_VS_OUTPUT_HEADER:
        streaming->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        size = 9;
        break;

    case UVC_VS_INPUT_HEADER:                                                   // 摄像头，当然是输入
        streaming->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                          // 这里指定了是V4L2类型的
        size = 13;
        break;

    default:
        uvc_trace(UVC_TRACE_DESCR, "device %d videostreaming interface "
            "%d HEADER descriptor not found.\n", dev->udev->devnum,
            alts->desc.bInterfaceNumber);
        goto error;
    }

    p = buflen >= 4 ? buffer[3] : 0;
    n = buflen >= size ? buffer[size-1] : 0;

    if (buflen < size + p*n) {
        uvc_trace(UVC_TRACE_DESCR, "device %d videostreaming "
            "interface %d HEADER descriptor is invalid.\n",
            dev->udev->devnum, alts->desc.bInterfaceNumber);
        goto error;
    }

    streaming->header.bNumFormats = p;
    streaming->header.bEndpointAddress = buffer[6];                             // 用来找后面的ep
    if (buffer[2] == UVC_VS_INPUT_HEADER) {
        streaming->header.bmInfo = buffer[7];
        streaming->header.bTerminalLink = buffer[8];
        streaming->header.bStillCaptureMethod = buffer[9];
        streaming->header.bTriggerSupport = buffer[10];
        streaming->header.bTriggerUsage = buffer[11];
    } else {
        streaming->header.bTerminalLink = buffer[7];
    }
    streaming->header.bControlSize = n;

    streaming->header.bmaControls = kmemdup(&buffer[size], p * n,
                        GFP_KERNEL);
    if (streaming->header.bmaControls == NULL) {
        ret = -ENOMEM;
        goto error;
    }

    buflen -= buffer[0];
    buffer += buffer[0];

    _buffer = buffer;
    _buflen = buflen;

    /* Count the format and frame descriptors. */
    while (_buflen > 2 && _buffer[1] == USB_DT_CS_INTERFACE) {                      // 统计支持的帧格式和帧尺寸
        switch (_buffer[2]) {
        case UVC_VS_FORMAT_UNCOMPRESSED:
        case UVC_VS_FORMAT_MJPEG:
        case UVC_VS_FORMAT_FRAME_BASED:
            nformats++;
            break;

        case UVC_VS_FORMAT_DV:
            /* DV format has no frame descriptor. We will create a
             * dummy frame descriptor with a dummy frame interval.
             */
            nformats++;
            nframes++;
            nintervals++;
            break;

        case UVC_VS_FORMAT_MPEG2TS:
        case UVC_VS_FORMAT_STREAM_BASED:
            uvc_trace(UVC_TRACE_DESCR, "device %d videostreaming "
                "interface %d FORMAT %u is not supported.\n",
                dev->udev->devnum,
                alts->desc.bInterfaceNumber, _buffer[2]);
            break;

        case UVC_VS_FRAME_UNCOMPRESSED:
        case UVC_VS_FRAME_MJPEG:
            nframes++;
            if (_buflen > 25)
                nintervals += _buffer[25] ? _buffer[25] : 3;
            break;

        case UVC_VS_FRAME_FRAME_BASED:
            nframes++;
            if (_buflen > 21)
                nintervals += _buffer[21] ? _buffer[21] : 3;
            break;
        }

        _buflen -= _buffer[0];
        _buffer += _buffer[0];
    }

    if (nformats == 0) {
        uvc_trace(UVC_TRACE_DESCR, "device %d videostreaming interface "
            "%d has no supported formats defined.\n",
            dev->udev->devnum, alts->desc.bInterfaceNumber);
        goto error;
    }

    /**
     * 计算需要的信息保存空间：
     *   [nformats size]|[nframes size]|[nintervals size]
     */
    size = nformats * sizeof(*format) + nframes * sizeof(*frame)
         + nintervals * sizeof(*interval);
    format = kzalloc(size, GFP_KERNEL);                                     // 开始即为format空间
    if (format == NULL) {
        ret = -ENOMEM;
        goto error;
    }

    frame = (struct uvc_frame *)&format[nformats];                          // frame地址开始的地方是format结束的地方
    interval = (u32 *)&frame[nframes];                                      // interval开始的地方是frame结束的地方

    streaming->format = format;                                             // 将format信息给streaming对象
    streaming->nformats = nformats;

    /* Parse the format descriptors. */
    while (buflen > 2 && buffer[1] == USB_DT_CS_INTERFACE) {                // 解析出format信息
        switch (buffer[2]) {
        case UVC_VS_FORMAT_UNCOMPRESSED:
        case UVC_VS_FORMAT_MJPEG:
        case UVC_VS_FORMAT_DV:
        case UVC_VS_FORMAT_FRAME_BASED:
            format->frame = frame;
            ret = uvc_parse_format(dev, streaming, format,
                &interval, buffer, buflen);
            if (ret < 0)
                goto error;

            frame += format->nframes;
            format++;

            buflen -= ret;
            buffer += ret;
            continue;

        default:
            break;
        }

        buflen -= buffer[0];
        buffer += buffer[0];
    }

    if (buflen)
        uvc_trace(UVC_TRACE_DESCR, "device %d videostreaming interface "
            "%d has %u bytes of trailing descriptor garbage.\n",
            dev->udev->devnum, alts->desc.bInterfaceNumber, buflen);

    /* Parse the alternate settings to find the maximum bandwidth. */
    for (i = 0; i < intf->num_altsetting; ++i) {
        struct usb_host_endpoint *ep;
        alts = &intf->altsetting[i];                                                    // 当前循环选中的接口
        ep = uvc_find_endpoint(alts,                                                    // 找到最大的端点传输包大小
                streaming->header.bEndpointAddress);
        if (ep == NULL)
            continue;

        psize = le16_to_cpu(ep->desc.wMaxPacketSize);
        psize = (psize & 0x07ff) * (1 + ((psize >> 11) & 3));
        if (psize > streaming->maxpsize)
            streaming->maxpsize = psize;
    }

    list_add_tail(&streaming->list, &dev->streams);                                     // 保存到streams链表中
    return 0;

error:
    usb_driver_release_interface(&uvc_driver.driver, intf);
    uvc_stream_delete(streaming);
    return ret;
}
```