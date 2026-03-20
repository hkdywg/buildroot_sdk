/*
* drm_fbdev.c 
*   drm frame buffer device helper(old architecture)
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/
#include <drm/drm.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_modeset_helper.h>

#include "drm_debug.h"

struct drm_debug_fbdev {
    struct drm_gem_object *fbdev_bo;
    struct drm_fb_helper *fbdev_helper;
};

static const struct drm_framebuffer_funcs drm_debug_fb_funcs = {
    .create_handle  = drm_gem_fb_create_handle,
    .destroy = drm_gem_fb_destroy,
};

static struct drm_framebuffer *drm_debug_framebuffer_init(struct drm_device *dev,
                        struct drm_mode_fb_cmd2 *mode_cmd)
{
    struct drm_framebuffer *fb;
    struct drm_gem_object *obj;
    size_t size;
    int ret;

    fb = kzalloc(sizeof(*fb), GFP_KERNEL);
    if (!fb)
        return ERR_PTR(-ENOMEM);

    size = mode_cmd->pitches[0] * mode_cmd->height;
    obj = drm_debug_gem_obj_create(dev, size);
    if (IS_ERR(obj))
        return NULL;

    fb->obj[0] = obj;
    drm_helper_mode_fill_fb_struct(dev, fb, mode_cmd);

    ret = drm_framebuffer_init(dev, fb, &drm_debug_fb_funcs);
    if (ret) {
        DRM_DEV_ERROR(dev->dev, "Failed to initialize framebuffer: %d\n", ret);
        kfree(fb);
        return ERR_PTR(ret);
    }

    return fb;
}

static int drm_debug_fbdev_mmap(struct fb_info *info,
                    struct vm_area_struct *vma)
{
    struct drm_fb_helper *helper = info->par;
    struct drm_debug_fbdev *debug_fbdev = helper->dev->dev_private;

    return drm_gem_mmap_obj(debug_fbdev->fbdev_bo, debug_fbdev->fbdev_bo->size, vma);
}

static const struct fb_ops drm_debug_fbdev_ops = {
    .owner = THIS_MODULE,
    DRM_FB_HELPER_DEFAULT_OPS,
    .fb_mmap = drm_debug_fbdev_mmap,
    .fb_fillrect = drm_fb_helper_cfb_fillrect,
    .fb_copyarea  = drm_fb_helper_cfb_copyarea,
    .fb_imageblit = drm_fb_helper_cfb_imageblit,
};

static int drm_debug_fbdev_create(struct drm_fb_helper *helper,
                        struct drm_fb_helper_surface_size *sizes)
{
    struct drm_mode_fb_cmd2 mode_cmd = { 0 };
    struct drm_device *dev = helper->dev;
    struct drm_framebuffer *fb;
    unsigned int bytes_per_pixel;
    struct fb_info *fbi;
    int ret;

    bytes_per_pixel = DIV_ROUND_UP(sizes->surface_bpp, 8);

    mode_cmd.width = sizes->surface_width;
    mode_cmd.height = sizes->surface_height;
    mode_cmd.pitches[0] = sizes->surface_width * bytes_per_pixel;
    mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
                                                sizes->surface_depth);

    fbi = drm_fb_helper_alloc_fbi(helper);
    if (IS_ERR(fbi)) {
        DRM_DEV_ERROR(dev->dev, "Failed to create framebuffer info.\n");
        ret = PTR_ERR(fbi);
        goto out;
    }

    helper->fb = drm_debug_framebuffer_init(dev, &mode_cmd);
    if (IS_ERR(helper->fb)) {
        DRM_DEV_ERROR(dev->dev, "Failed to allocate DRM framebuffer.\n");
        ret = PTR_ERR(helper->fb);
        goto out;
    }

    fbi->fbops = &drm_debug_fbdev_ops; 
    fbi->screen_size  = fb->height * fb->pitches[0];
    fbi->fix.smem_len = fbi->screen_size;

    fb = helper->fb;
    drm_fb_helper_fill_info(fbi, helper, sizes);

    dev->mode_config.fb_base = 0;

    return 0;

out:
    return ret;
}

static const struct drm_fb_helper_funcs drm_fb_helper_funcs = {
    .fb_probe = drm_debug_fbdev_create,
};

int drm_debug_fbdev_init(struct drm_device *dev)
{
    struct drm_debug_fbdev *debug_fbdev = dev->dev_private;
    struct drm_fb_helper *helper;
    int ret;

    if (!dev->mode_config.num_crtc || !dev->mode_config.num_connector)
        return -EINVAL;

    helper = devm_kzalloc(dev->dev, sizeof(*helper), GFP_KERNEL);
    if (!helper)
        return -ENOMEM;
    debug_fbdev->fbdev_helper = helper;

    drm_fb_helper_prepare(dev, helper, &drm_fb_helper_funcs);

    ret = drm_fb_helper_init(dev, helper);
    if (ret < 0) {
        DRM_DEV_ERROR(dev->dev,
                "Failed to initialize drm fb helper - %d.\n", ret);
        return ret;
    }

    ret = drm_fb_helper_initial_config(helper, 32);
    if (ret < 0) {
        DRM_DEV_ERROR(dev->dev, 
                    "Failed to set initial hw config - %d\n", ret);
        drm_fb_helper_fini(helper);
        return ret;
    }

    return 0;
}

void  drm_debug_fbdev_fini(struct drm_device *dev)
{
    struct drm_debug_fbdev *debug_fbdev = dev->dev_private;
    struct drm_fb_helper *helper = debug_fbdev->fbdev_helper;

    if (!helper)
        return;

    drm_fb_helper_unregister_fbi(helper);

    if (helper->fb)
        drm_framebuffer_put(helper->fb);

    drm_fb_helper_fini(helper);
}

