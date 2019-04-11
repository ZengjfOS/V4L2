# multi-planar bytesused

Camera Type: `V4L2_CAP_VIDEO_CAPTURE_MPLANE`

## 参考文档

* [3.2. Streaming I/O (Memory Mapping)](https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/mmap.html#example-mapping-buffers-in-the-multi-planar-api)
* http://www.infradead.org/~mchehab/kernel_docs_pdf/media.pdf

## 获取图像大小

主要是要注意获取图片大小是保存在了`struct v4l2_plane`中了；

* `struct v4l2_plane planes;`
* `planes.bytesused`