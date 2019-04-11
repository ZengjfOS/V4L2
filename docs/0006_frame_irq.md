# frame irq

正常来说，完成一帧图片芯片是会引发一次中断，进行DMA帧初始地址进行切换；

```
* static int mxc_isi_probe(struct platform_device *pdev)
  * ret = devm_request_irq(dev, res->start, mxc_isi_irq_handler, 0, dev_name(dev), mxc_isi);
    * static irqreturn_t mxc_isi_irq_handler(int irq, void *priv)
      * status = mxc_isi_get_irq_status(mxc_isi);
      * mxc_isi_cap_frame_write_done(mxc_isi);
```