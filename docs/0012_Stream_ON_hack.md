# Stream_ON hack

## 回调函数
```
static const struct v4l2_ioctl_ops mxc_isi_capture_ioctl_ops = {
    .vidioc_querycap        = mxc_isi_cap_querycap,

    .vidioc_enum_fmt_vid_cap_mplane    = mxc_isi_cap_enum_fmt_mplane,
    .vidioc_try_fmt_vid_cap_mplane    = mxc_isi_cap_try_fmt_mplane,
    .vidioc_s_fmt_vid_cap_mplane    = mxc_isi_cap_s_fmt_mplane,
    .vidioc_g_fmt_vid_cap_mplane    = mxc_isi_cap_g_fmt_mplane,

    .vidioc_reqbufs            = vb2_ioctl_reqbufs,
    .vidioc_querybuf        = vb2_ioctl_querybuf,
    .vidioc_qbuf            = vb2_ioctl_qbuf,
    .vidioc_dqbuf            = vb2_ioctl_dqbuf,
    .vidioc_expbuf            = vb2_ioctl_expbuf,
    .vidioc_prepare_buf        = vb2_ioctl_prepare_buf,
    .vidioc_create_bufs        = vb2_ioctl_create_bufs,

    .vidioc_streamon        = mxc_isi_cap_streamon,
    .vidioc_streamoff        = mxc_isi_cap_streamoff,

    .vidioc_g_selection        = mxc_isi_cap_g_selection,
    .vidioc_s_selection        = mxc_isi_cap_s_selection,
    .vidioc_g_chip_ident    = mxc_isi_cap_g_chip_ident,

    .vidioc_g_parm            = mxc_isi_cap_g_parm,
    .vidioc_s_parm            = mxc_isi_cap_s_parm,

    .vidioc_enum_framesizes = mxc_isi_cap_enum_framesizes,
    .vidioc_enum_frameintervals = mxc_isi_cap_enum_frameintervals,
};
```

## 调用路径

* static int mxc_isi_cap_streamon(struct file *file, void *priv, enum v4l2_buf_type type)
  * ret = v4l2_subdev_call(subdev, video, s_stream, enable);
    ```
    static const struct v4l2_subdev_core_ops max9286_core_ops = {
        .s_power    = max9286_set_power,
    };

    static const struct v4l2_subdev_video_ops max9286_video_ops = {
        .s_parm =    max9286_s_parm,
        .g_parm =    max9286_g_parm,
        .s_stream        = max9286_s_stream,
    };

    static const struct v4l2_subdev_ops max9286_subdev_ops = {
        .core    = &max9286_core_ops,
        .pad    = &max9286_pad_ops,
        .video    = &max9286_video_ops,
    };
    ```
    * static int max9286_s_stream(struct v4l2_subdev *sd, int enable)