# vb2 callback

vb2 buffer偏向于芯片DMA操作，V4L2 buffer偏向于内核到用户空间数据转换；

## queue调用vb2回调

```
* static irqreturn_t mxc_isi_irq_handler(int irq, void *priv)
	* void mxc_isi_cap_frame_write_done(struct mxc_isi_dev *mxc_isi)
		* void mxc_isi_channel_set_outbuf(struct mxc_isi_dev *mxc_isi, struct mxc_isi_buffer *buf)
			* vb2_dma_contig_plane_dma_addr(struct vb2_buffer *vb, unsigned int plane_no)
				* void *vb2_plane_cookie(struct vb2_buffer *vb, unsigned int plane_no)
					* return call_ptr_memop(vb, cookie, vb->planes[plane_no].mem_priv);
						```
						#define call_ptr_memop(vb, op, args...)					\
							((vb)->vb2_queue->mem_ops->op ?					\
								(vb)->vb2_queue->mem_ops->op(args) : NULL)
						```
```


## queue注册vb2回调

```
	memset(q, 0, sizeof(*q));
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->drv_priv = mxc_isi;
	q->ops = &mxc_cap_vb2_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->buf_struct_size = sizeof(struct mxc_isi_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &mxc_isi->lock;

	ret = vb2_queue_init(q);
	if (ret)
		goto err_free_ctx;
```

## vb2操作回调

```
/*********************************************/
/*       DMA CONTIG exported functions       */
/*********************************************/

const struct vb2_mem_ops vb2_dma_contig_memops = {
	.alloc		= vb2_dc_alloc,
	.put		= vb2_dc_put,
	.get_dmabuf	= vb2_dc_get_dmabuf,
	.cookie		= vb2_dc_cookie,
	.vaddr		= vb2_dc_vaddr,
	.mmap		= vb2_dc_mmap,
	.get_userptr	= vb2_dc_get_userptr,
	.put_userptr	= vb2_dc_put_userptr,
	.prepare	= vb2_dc_prepare,
	.finish		= vb2_dc_finish,
	.map_dmabuf	= vb2_dc_map_dmabuf,
	.unmap_dmabuf	= vb2_dc_unmap_dmabuf,
	.attach_dmabuf	= vb2_dc_attach_dmabuf,
	.detach_dmabuf	= vb2_dc_detach_dmabuf,
	.num_users	= vb2_dc_num_users,
};
EXPORT_SYMBOL_GPL(vb2_dma_contig_memops);
```