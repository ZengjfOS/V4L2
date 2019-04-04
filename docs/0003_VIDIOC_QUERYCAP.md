# VIDIOC_QUERYCAP

## 参考文档

* [vidioc_querycap浅析](https://blog.csdn.net/leesagacious/article/details/50099473)


## 内核调度流程

用户层调用`ioctl()`，经过`v4l2_ioctl` —-> `video_ioctl2` ——> `__video_do_ioctl()`

## 设备节点映射函数

```C
static const struct v4l2_ioctl_ops mxc_isi_capture_ioctl_ops = {
	.vidioc_querycap		= mxc_isi_cap_querycap,

	.vidioc_enum_fmt_vid_cap_mplane	= mxc_isi_cap_enum_fmt_mplane,
	.vidioc_try_fmt_vid_cap_mplane	= mxc_isi_cap_try_fmt_mplane,
	.vidioc_s_fmt_vid_cap_mplane	= mxc_isi_cap_s_fmt_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= mxc_isi_cap_g_fmt_mplane,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,

	.vidioc_streamon		= mxc_isi_cap_streamon,
	.vidioc_streamoff		= mxc_isi_cap_streamoff,

	.vidioc_g_selection		= mxc_isi_cap_g_selection,
	.vidioc_s_selection		= mxc_isi_cap_s_selection,
	.vidioc_g_chip_ident	= mxc_isi_cap_g_chip_ident,

	.vidioc_g_parm			= mxc_isi_cap_g_parm,
	.vidioc_s_parm			= mxc_isi_cap_s_parm,

	.vidioc_enum_framesizes = mxc_isi_cap_enum_framesizes,
	.vidioc_enum_frameintervals = mxc_isi_cap_enum_frameintervals,
};
```