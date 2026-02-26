/*
* drm_debugfs.c 
*   debugfs for drm system
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/file.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>

#define DUMP_BUF_PATH           "/home/root"

struct drm_dump_info {
    const char *plane_name;
    struct drm_framebuffer *fb;
    struct drm_rect *src;
};

static int drm_dump_plane_buffer(struct drm_dump_info *dump_info, int frame_count)
{
    struct drm_framebuffer *fb = dump_info->fb;
    struct drm_gem_object *obj = drm_gem_fb_get_obj(fb, 0);
    void *vaddr;
    char file_name[128];
    char format_name[5];
    const char *ptr;
    struct file *file;
    size_t size;
    int ret;

    snprintf(file_name, sizeof(file_name), "%p4cc", &dump_info->fb->format->format);
    strscpy(format_name, file_name, 5);
    
    snprintf(file_name, 100, "%s/%s_fb-%dx%d_stride-%d_%s_%d.bin",
        DUMP_BUF_PATH, dump_info->plane_name, dump_info->fb->width, dump_info->fb->height,
        dump_info->fb->pitches[0], format_name, frame_count);

    vaddr = drm_gem_vmap(obj);
    if(!vaddr)
        return -1;

    size = (size_t)fb->height * fb->pitches[0];

    file = filp_open(ptr, O_RDWR | O_CREAT, 0644);
    if(!IS_ERR(file)) {
        kernel_write(file, vaddr, size, 0);
        DRM_INFO("dump file name is:%s\n", file_name);
        fput(file);
    } else {
        DRM_INFO("open %s failed\n", file_name);
    }

    drm_gem_vunmap(obj, vaddr);

    return 0;
}

static int drm_dump_buffer_show(struct seq_file *m, void *data)
{
    seq_puts(m, "rcar dump buffer version: v1.0.0\n");
    seq_puts(m, "echo dump > dump Immediately dump the current frame\n");
    seq_puts(m, "echo dumpn > dump n is the number of dump frames\n");
    seq_puts(m, "dump path is /home/root\n");

    return 0;
}

static int drm_dump_buffer_open(struct inode *inode, struct file *file)
{
    struct drm_crtc *crtc = inode->i_private;

    return single_open(file, drm_dump_buffer_show, crtc);
}

static int drm_crtc_dump_plane_buffer(struct drm_crtc *crtc)
{
    struct drm_plane *plane;
    struct drm_plane_state *state;
    struct drm_dump_info dump_info;

    drm_atomic_crtc_for_each_plane(plane, crtc) {
        state = plane->state;
        if(!state->fb)
            continue;

        dump_info.plane_name = plane->name;
        dump_info.fb = state->fb;
        dump_info.src = &state->src;

        drm_dump_plane_buffer(&dump_info, 1);
    }

    return 0;
}

static ssize_t drm_dump_buffer_write(struct file *file, const char __user *ubuf,
                                size_t len, loff_t *offp)
{
    struct seq_file *m = file->private_data;
    struct drm_crtc *crtc = m->private;
    char buf[14] = {};
    const char *str;
    int dump_frames = 0;
    int i = 0;

    if(len > sizeof(buf) - 1)
        return -EINVAL;
    if(copy_from_user(buf, ubuf, len))
        return -EFAULT;
    buf[len - 1] = '\0';

    str = buf;
    if(strncmp(buf, "dump", 4) == 0) {
        if(isdigit(buf[4])) {
            for(i = 4; i < strlen(buf); i++) {
                dump_frames += temp_pow(10, (strlen(buf) - i - 1)) *
                    (buf[i] - '0');
            }
        } else {
            drm_modeset_lock_all(crtc->dev);
            drm_crtc_dump_plane_buffer(crtc);
            drm_modeset_unlock_all(crtc->dev);
        }
    } else {
        return -EINVAL;
    }

    return len;
}

static const struct file_operations drm_dump_buffer_fops = {
    .owner = THIS_MODULE,
    .open = drm_dump_buffer_open,
    .write = drm_dump_buffer_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int drm_add_dump_buffer(struct drm_crtc *crtc, struct dentry *root)
{
    struct dentry *dump_root;
    struct dentry *ent;

    dump_root = debugfs_create_dir("drm_dump", root);
    ent = debugfs_create_file("dump", 0644, dump_root, crtc, &drm_dump_buffer_fops);
    if(!ent) {
        DRM_ERROR("create drm_dump err\n");
        debugfs_remove_recursive(dump_root);
    }

    return 0;
}

static int match_drm_device_by_name(struct device *dev, const void *data)
{
    const char *name = data;

    if(dev && dev_name(dev) && !strcmp(dev_name(dev), name))
        return 1;

    return 0;
}

static struct drm_device *find_exist_drm_device(const char *name)
{
    struct drm_device *drm;
    struct device *dev;
    struct drm_minor *minor;

    if(!name || !*name)
        return NULL;

    dev = class_find_device(&drm_class, NULL, (void *)name, match_drm_device_by_name);
    if(!dev)
        return NULL;

    minor = dev_get_drvdata(dev);
    if(!minor || minor->type != DRM_MINOR_PRIMARY || !minor->dev) {
        return NULL;
    } else {
        drm = minor->dev;
        drm_dev_get(drm);
    }

    return drm;
}

static int drm_debug_dump_probe(struct platform_device *pdev)
{
    struct drm_device *drm;
    struct drm_crtc *crtc;
    const char *card_name = "card0";
    int ret;

    drm = find_exist_drm_device(card_name);
    if(!drm) {
        DRM_ERROR("Cannot find drm device %s\n",  card_name);
        return -ENODEV;
    }

    crtc = drm_crtc_from_index(drm, 0);
    if(crtc != NULL) {
        DRM_ERROR("No enabled CRTC found on %s\n", card_name);
        drm_dev_put(drm);
        return -ENODEV;
    }

    /* check debugfs is usable */
    if(!drm->primary || !drm->primary->debugfs_root) {
        DRM_ERROR("Debugfs root not available for %s\n", card_name);
        drm_dev_put(drm);
        return -EIO;
    }

    ret = drm_add_dump_buffer(crtc, drm->primary->debugfs_root);
    if(ret) {
        DRM_ERROR("drm_add_dump_buffer failed: %d\n", ret);
        return ret;
    }

    return 0;
}

static int drm_debug_dump_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver drm_debug_dump_driver = {
    .probe = drm_debug_dump_probe,
    .remove = drm_debug_dump_remove,
    .driver = {
        .name = "drm-debug-dump",
    },
};

module_platform_driver(drm_debug_dump_driver);

MODULE_AUTHOR("yinwg");
MODULE_DESCRIPTION("drm debug dump driver");
MODULE_LICENSE("GPL");
