/*
 * rotary_encoder.c
 *
 * Rotary Encoder Driver
 *
 * @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co., Ltd. All rights reserved.
 *
 * Optimized and Refactored Version
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define ENCODER_BUF_SIZE    3

struct rotary_encoder_channel {
	// default map
	//{ 384, 385 }, /* GPIO6_4   GPIO6_5  */
	//{ 386, 387 }, /* GPIO6_6   GPIO6_7  */
	//{ 390, 393 }, /* GPIO6_10  GPIO6_13 */
	int phase_a_gpio;
	int phase_b_gpio;
	int irq;
	bool ready_to_sample;
	struct timer_list timer;
	struct rotary_encoder_drvdata *drvdata;
};

struct rotary_encoder_drvdata {
	struct device *dev;
	struct miscdevice misc;
	
	struct rotary_encoder_channel *channels;
	int num_channels;

	spinlock_t lock;
	u8 report_buf[ENCODER_BUF_SIZE]; /* [irq_num, phase_a_gpio_val, phase_b_gpio_val] */
	
	struct fasync_struct *fasync;
	wait_queue_head_t read_wait;
};

static void encoder_timer_callback(struct timer_list *t)
{
	struct rotary_encoder_channel *ch = from_timer(ch, t, timer);
	unsigned long flags;

	spin_lock_irqsave(&ch->drvdata->lock, flags);
	ch->ready_to_sample = true;
	spin_unlock_irqrestore(&ch->drvdata->lock, flags);
}

static irqreturn_t rotary_encoder_irq_handler(int irq, void *dev_id)
{
	struct rotary_encoder_channel *ch = dev_id;
	struct rotary_encoder_drvdata *drvdata = ch->drvdata;
	unsigned long flags;

	spin_lock_irqsave(&drvdata->lock, flags);

	if (!ch->ready_to_sample) {
		spin_unlock_irqrestore(&drvdata->lock, flags);
		return IRQ_HANDLED;
	}

	/* Disable sampling until timer expires */
	ch->ready_to_sample = false;

	/* Update report buffer */
	drvdata->report_buf[0] = irq;
	drvdata->report_buf[1] = gpio_get_value(ch->phase_a_gpio);
	drvdata->report_buf[2] = gpio_get_value(ch->phase_b_gpio);

	spin_unlock_irqrestore(&drvdata->lock, flags);

	/* Restart debounce timer */
	mod_timer(&ch->timer, jiffies + msecs_to_jiffies(20));

	/* Notify userspace */
	kill_fasync(&drvdata->fasync, SIGIO, POLL_IN);
	wake_up_interruptible(&drvdata->read_wait);

	return IRQ_HANDLED;
}

static ssize_t rotary_encoder_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	struct rotary_encoder_drvdata *drvdata = container_of(file->private_data, struct rotary_encoder_drvdata, misc);
	unsigned long flags;
	u8 tmp_buf[ENCODER_BUF_SIZE];
	int ret;

	if (size != ENCODER_BUF_SIZE)
		return -EINVAL;

	spin_lock_irqsave(&drvdata->lock, flags);
	memcpy(tmp_buf, drvdata->report_buf, ENCODER_BUF_SIZE);
	/* Reset buffer after read */
	drvdata->report_buf[0] = 0xFF;
	drvdata->report_buf[1] = 0xFF;
	drvdata->report_buf[2] = 0xFF;
	spin_unlock_irqrestore(&drvdata->lock, flags);

	ret = copy_to_user(buf, tmp_buf, ENCODER_BUF_SIZE);
	if (ret)
		return -EFAULT;

	return ENCODER_BUF_SIZE;
}

static int rotary_encoder_fasync(int fd, struct file *file, int on)
{
	struct rotary_encoder_drvdata *drvdata = container_of(file->private_data, struct rotary_encoder_drvdata, misc);
	return fasync_helper(fd, file, on, &drvdata->fasync);
}

static const struct file_operations rotary_encoder_fops = {
	.owner   = THIS_MODULE,
	.read    = rotary_encoder_read,
	.fasync  = rotary_encoder_fasync,
	.llseek  = no_llseek,
};

static int rotary_encoder_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct rotary_encoder_drvdata *drvdata;
	int count, i, ret;
	enum of_gpio_flags flags;

	if (!node)
		return -ENODEV;

	count = of_gpio_count(node);
	if (count <= 0 || count % 2 != 0) {
		dev_err(dev, "Invalid GPIO count: %d. Must be even (pairs of A/B)\n", count);
		return -EINVAL;
	}

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = dev;
	drvdata->num_channels = count / 2;
	drvdata->channels = devm_kcalloc(dev, drvdata->num_channels, sizeof(struct rotary_encoder_channel), GFP_KERNEL);
	if (!drvdata->channels)
		return -ENOMEM;

	spin_lock_init(&drvdata->lock);
	init_waitqueue_head(&drvdata->read_wait);
	
	/* Initialize report buffer to default state */
	memset(drvdata->report_buf, 0xFF, ENCODER_BUF_SIZE);

	for (i = 0; i < drvdata->num_channels; i++) {
		struct rotary_encoder_channel *ch = &drvdata->channels[i];
		int phase_a_gpio, phase_b_gpio;
		int idx_a = i * 2;
		int idx_b = i * 2 + 1;

		char *label_a, *label_b, *label_irq;

		phase_a_gpio = of_get_gpio_flags(node, idx_a, &flags);
		phase_b_gpio = of_get_gpio_flags(node, idx_b, &flags);

		if (!gpio_is_valid(phase_a_gpio) || !gpio_is_valid(phase_b_gpio)) {
			dev_err(dev, "Invalid GPIO pair at index %d\n", i);
			return -EINVAL;
		}

		label_a = devm_kasprintf(dev, GFP_KERNEL, "encoder_ch%d_a", i);
		label_b = devm_kasprintf(dev, GFP_KERNEL, "encoder_ch%d_b", i);
		label_irq = devm_kasprintf(dev, GFP_KERNEL, "encoder_ch%d_irq", i);
		
		if (!label_a || !label_b || !label_irq)
			return -ENOMEM;

		ret = devm_gpio_request_one(dev, phase_a_gpio, GPIOF_DIR_IN, label_a);
		if (ret) {
			dev_err(dev, "Failed to request GPIO A %d\n", phase_a_gpio);
			return ret;
		}
		
		ret = devm_gpio_request_one(dev, phase_b_gpio, GPIOF_DIR_IN, label_b);
		if (ret) {
			dev_err(dev, "Failed to request GPIO B %d\n", phase_b_gpio);
			return ret;
		}

		ch->phase_a_gpio = phase_a_gpio;
		ch->phase_b_gpio = phase_b_gpio;
		ch->drvdata = drvdata;
		ch->ready_to_sample = true;
		timer_setup(&ch->timer, encoder_timer_callback, 0);

		ch->irq = gpio_to_irq(phase_a_gpio);
		if (ch->irq < 0) {
			dev_err(dev, "Unable to map GPIO A %d to IRQ\n", phase_a_gpio);
			return ch->irq;
		}

		ret = devm_request_irq(dev, ch->irq, rotary_encoder_irq_handler,
				       IRQF_TRIGGER_FALLING, label_irq, ch);
		if (ret) {
			dev_err(dev, "Failed to request IRQ %d\n", ch->irq);
			return ret;
		}
	}

	/* Register misc device */
	drvdata->misc.minor = MISC_DYNAMIC_MINOR;
	drvdata->misc.name = "rotary_encoder_device";
	drvdata->misc.fops = &rotary_encoder_fops;
	
	ret = misc_register(&drvdata->misc);
	if (ret) {
		dev_err(dev, "Failed to register misc device\n");
		return ret;
	}

	platform_set_drvdata(pdev, drvdata);
	dev_info(dev, "Rotary encoder driver initialized with %d channels\n", count);

	return 0;
}

static int rotary_encoder_remove(struct platform_device *pdev)
{
	struct rotary_encoder_drvdata *drvdata = platform_get_drvdata(pdev);
	int i;

	misc_deregister(&drvdata->misc);

	for (i = 0; i < drvdata->num_channels; i++) {
		del_timer_sync(&drvdata->channels[i].timer);
	}

	return 0;
}

static const struct of_device_id rotary_encoder_of_match[] = {
	{ .compatible = "newversion,rotary_encoder" },
	{ },
};
MODULE_DEVICE_TABLE(of, rotary_encoder_of_match);

static struct platform_driver rotary_encoder_driver = {
	.probe  = rotary_encoder_probe,
	.remove = rotary_encoder_remove,
	.driver = {
		.name = "rotary_encoder",
		.of_match_table = rotary_encoder_of_match,
	},
};

module_platform_driver(rotary_encoder_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("weigenyin <weigenyin@zjautomotive.com>");
MODULE_DESCRIPTION("Rotary Encoder Driver");

