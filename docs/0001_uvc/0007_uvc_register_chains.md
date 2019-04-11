# uvc_register_chains

* uvc_register_chains
  ```C
  static int uvc_register_chains(struct uvc_device *dev)
  {
      struct uvc_video_chain *chain;
      int ret;
  
      list_for_each_entry(chain, &dev->chains, list) {                      // 遍历每一个链表
          ret = uvc_register_terms(dev, chain);
          if (ret < 0)
              return ret;
  
  #ifdef CONFIG_MEDIA_CONTROLLER
          ret = uvc_mc_register_entities(chain);
          if (ret < 0)
              uvc_printk(KERN_INFO,
                     "Failed to register entities (%d).\n", ret);
  #endif
      }
  
      return 0;
  }
  ```
* uvc_register_terms
  ```C
  static int uvc_register_terms(struct uvc_device *dev,
      struct uvc_video_chain *chain)
  {
      struct uvc_streaming *stream;
      struct uvc_entity *term;
      int ret;
  
      list_for_each_entry(term, &chain->entities, chain) {                  // 遍历uvc chain链表
          if (UVC_ENTITY_TYPE(term) != UVC_TT_STREAMING)                    // 不是输入Terminal streaming类型就下一个
              continue;
  
          stream = uvc_stream_by_id(dev, term->id);                         // 获取uvc视频流
          if (stream == NULL) {
              uvc_printk(KERN_INFO, "No streaming interface found "
                     "for terminal %u.", term->id);
              continue;
          }
  
          stream->chain = chain;                                            // 捆绑uvc视频流和uvc视频链
          ret = uvc_register_video(dev, stream);                            // 注册uvc视频流
          if (ret < 0)
              return ret;
  
          /* Register a metadata node, but ignore a possible failure,
           * complete registration of video nodes anyway.
           */
          uvc_meta_register(stream);
  
          term->vdev = &stream->vdev;
      }
  
      return 0;
  }
  ```