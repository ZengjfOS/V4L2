# mplane bytesused hack

```
* static int mxc_isi_register_cap_device(struct mxc_isi_dev *mxc_isi, struct v4l2_device *v4l2_dev)
  * ret = vb2_queue_init(q);
    * q->buf_ops = &v4l2_buf_ops;
      ```C
      static const struct vb2_buf_ops v4l2_buf_ops = {
          .verify_planes_array   = __verify_planes_array_core,
          .fill_user_buffer      = __fill_v4l2_buffer,
          .fill_vb2_buffer       = __fill_vb2_buffer,
          .copy_timestamp         = __copy_timestamp,
      };
      ```
      * static void __fill_v4l2_buffer(struct vb2_buffer *vb, void *pb)
        * struct v4l2_plane *pdst = &b->m.planes[plane];
        * pdst->bytesused = psrc->bytesused;                      // <-- bytesusd是在planes中
```