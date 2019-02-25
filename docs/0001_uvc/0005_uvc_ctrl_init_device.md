# uvc_ctrl_init_device

* 主要工作是解析VC中的控制信息，生成对应的控制方法；
* 控制信息是从VC的描述符中提取出来的：

## 参考代码

* [uvc摄像头代码解析5](http://www.voidcn.com/article/p-sxqfydcg-gx.html)

## 代码解析

* uvc_ctrl_init_device
  ```C
  int uvc_ctrl_init_device(struct uvc_device *dev)
  {
      struct uvc_entity *entity;
      unsigned int i;
  
      INIT_WORK(&dev->async_ctrl.work, uvc_ctrl_status_event_work);
  
      /* Walk the entities list and instantiate controls */
      list_for_each_entry(entity, &dev->entities, list) {                                 // 迭代VC中检测到entity
          struct uvc_control *ctrl;
          unsigned int bControlSize = 0, ncontrols;
          u8 *bmControls = NULL;
  
          // 获取对应entity中的bitmap和位控制大小(位控所占的字节)
          if (UVC_ENTITY_TYPE(entity) == UVC_VC_EXTENSION_UNIT) {
              bmControls = entity->extension.bmControls;
              bControlSize = entity->extension.bControlSize;
          } else if (UVC_ENTITY_TYPE(entity) == UVC_VC_PROCESSING_UNIT) {
              bmControls = entity->processing.bmControls;
              bControlSize = entity->processing.bControlSize;
          } else if (UVC_ENTITY_TYPE(entity) == UVC_ITT_CAMERA) {
              bmControls = entity->camera.bmControls;
              bControlSize = entity->camera.bControlSize;
          }
  
          /* Remove bogus/blacklisted controls */
          uvc_ctrl_prune_entity(dev, entity);
  
          /* Count supported controls and allocate the controls array */
          ncontrols = memweight(bmControls, bControlSize);                        // 需要多少位控
          if (ncontrols == 0)
              continue;
  
          entity->controls = kcalloc(ncontrols, sizeof(*ctrl),                    // 申请位控空间
                         GFP_KERNEL);
          if (entity->controls == NULL)
              return -ENOMEM;
          entity->ncontrols = ncontrols;
  
          /* Initialize all supported controls */
          ctrl = entity->controls;
          for (i = 0; i < bControlSize * 8; ++i) {                                // 位控最大bit数
              if (uvc_test_bit(bmControls, i) == 0)                               // 位控空间是否存在
                  continue;
  
              ctrl->entity = entity;
              ctrl->index = i;
  
              uvc_ctrl_init_ctrl(dev, ctrl);
              ctrl++;
          }
      }
  
      return 0;
  }
  ```
* uvc_ctrl_init_ctrl
  ```C
  static void uvc_ctrl_init_ctrl(struct uvc_device *dev, struct uvc_control *ctrl)
  {
      const struct uvc_control_info *info = uvc_ctrls;                                    // 标准的位控
      const struct uvc_control_info *iend = info + ARRAY_SIZE(uvc_ctrls);
      const struct uvc_control_mapping *mapping = uvc_ctrl_mappings;                      // 位控映射
      const struct uvc_control_mapping *mend =
          mapping + ARRAY_SIZE(uvc_ctrl_mappings);
  
      /* XU controls initialization requires querying the device for control
       * information. As some buggy UVC devices will crash when queried
       * repeatedly in a tight loop, delay XU controls initialization until
       * first use.
       */
      if (UVC_ENTITY_TYPE(ctrl->entity) == UVC_VC_EXTENSION_UNIT)
          return;
  
      for (; info < iend; ++info) {
          if (uvc_entity_match_guid(ctrl->entity, info->entity) &&
              ctrl->index == info->index) {                                               // 根据索引进行匹配，说明这个是标准的，在UVC标准中已经定义好的
              uvc_ctrl_add_info(dev, ctrl, info);                                         // ctrl->info = *info;
              break;
           }
      }
  
      if (!ctrl->initialized)
          return;
  
      for (; mapping < mend; ++mapping) {
          if (uvc_entity_match_guid(ctrl->entity, mapping->entity) &&
              ctrl->info.selector == mapping->selector)                                   // 通过前面的info来帮顶mapping
              __uvc_ctrl_add_mapping(dev, ctrl, mapping);
      }
  }
  ```