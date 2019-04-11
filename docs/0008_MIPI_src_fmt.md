# MIPI src fmt

SOC MIPI CSI2一般是SINK端，其数据相当于是source，如果芯片内部有需要转换，那转出的数据就是dist；

```
* static int mxc_isi_source_fmt_init(struct mxc_isi_dev *mxc_isi)
  * ret = v4l2_subdev_call(src_sd, pad, get_fmt, NULL, &src_fmt);
    ```
    static struct v4l2_subdev_pad_ops mipi_csi2_pad_ops = {
      .enum_frame_size = mipi_csi2_enum_framesizes,
      .enum_frame_interval = mipi_csi2_enum_frame_interval,
      .get_fmt = mipi_csi2_get_fmt,
      .set_fmt = mipi_csi2_set_fmt,
    };
    ```
    * static int mipi_csi2_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *fmt)
      * static int mxc_csi2_get_sensor_fmt(struct mxc_mipi_csi2_dev *csi2dev)
        * ret = v4l2_subdev_call(sen_sd, pad, get_fmt, NULL, &src_fmt);
          ```
          static const struct v4l2_subdev_pad_ops max9286_pad_ops = {
          	.enum_mbus_code		= max9286_enum_mbus_code,
          	.enum_frame_size	= max9286_enum_framesizes,
          	.enum_frame_interval	= max9286_enum_frame_interval,
          	.get_fmt		= max9286_get_fmt,
          	.set_fmt		= max9286_set_fmt,
          	.get_frame_desc		= max9286_get_frame_desc,
          	.set_frame_desc		= max9286_set_frame_desc,
          };
          ```
          * static int max9286_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *fmt)
            * max9286_data->format.code = MEDIA_BUS_FMT_UYVY8_2X8;
```