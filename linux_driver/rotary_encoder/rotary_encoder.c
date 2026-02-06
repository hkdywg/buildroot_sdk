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
#define ENCODER_FIFO_DEPTH  16
#define DEBOUNCE_MS         20

struct rotary_encoder_drvdata;

struct rotary_encoder_channel {
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
	/* FIFO buffer to store events: [irq_num, phase_a_val, phase_b_val] */
	u8 fifo[ENCODER_FIFO_DEPTH][ENCODER_BUF_SIZE];
	int head;
	int tail;
	
	struct fasync_struct *fasync;
	wait_queue_head_t read_wait;
};

static void encoder_timer_callback(struct timer_list *t)
{
	struct rotary_encoder_channel *ch = from_timer(ch, t, timer);
	struct rotary_encoder_drvdata *drvdata = ch->drvdata;
	unsigned long flags;
	int next_head;

	spin_lock_irqsave(&drvdata->lock, flags);

	next_head = (drvdata->head + 1) % ENCODER_FIFO_DEPTH;
	if (next_head == drvdata->tail) {
		/* FIFO full, drop the oldest event to make room for new one */
		drvdata->tail = (drvdata->tail + 1) % ENCODER_FIFO_DEPTH;
		dev_warn(drvdata->dev, "Encoder FIFO full, dropping oldest event\n");
	}

	/* Store event in FIFO: IRQ number helps identify which knob moved */
	drvdata->fifo[drvdata->head][0] = ch->irq;
	drvdata->fifo[drvdata->head][1] = gpio_get_value(ch->phase_a_gpio);
	drvdata->fifo[drvdata->head][2] = gpio_get_value(ch->phase_b_gpio);
	
	drvdata->head = next_head;

	/* Re-enable sampling */
	ch->ready_to_sample = true;
	spin_unlock_irqrestore(&drvdata->lock, flags);

	/* Notify userspace */
	kill_fasync(&drvdata->fasync, SIGIO, POLL_IN);
	wake_up_interruptible(&drvdata->read_wait);
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

	/* Disable sampling and start debounce timer */
	ch->ready_to_sample = false;
	mod_timer(&ch->timer, jiffies + msecs_to_jiffies(DEBOUNCE_MS));

	spin_unlock_irqrestore(&drvdata->lock, flags);

	return IRQ_HANDLED;
}

static ssize_t rotary_encoder_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	struct rotary_encoder_drvdata *drvdata = container_of(file->private_data, struct rotary_encoder_drvdata, misc);
	u8 tmp_buf[ENCODER_BUF_SIZE];
	int ret;

	if (size != ENCODER_BUF_SIZE)
		return -EINVAL;

	/* Check if FIFO is empty */
	while (drvdata->head == drvdata->tail) {
		
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
			
		if (wait_event_interruptible(drvdata->read_wait, 
				drvdata->head != drvdata->tail))
			return -ERESTARTSYS;
	}

	/* Read from FIFO */
	memcpy(tmp_buf, drvdata->fifo[drvdata->tail], ENCODER_BUF_SIZE);
	drvdata->tail = (drvdata->tail + 1) % ENCODER_FIFO_DEPTH;
	
	ret = copy_to_user(buf, tmp_buf, ENCODER_BUF_SIZE);
	if (ret)
		return -EFAULT;

	return ENCODER_BUF_SIZE;
}

static __poll_t rotary_encoder_poll(struct file *file, poll_table *wait)
{
	struct rotary_encoder_drvdata *drvdata = container_of(file->private_data, struct rotary_encoder_drvdata, misc);
	__poll_t mask = 0;

	poll_wait(file, &drvdata->read_wait, wait);

	spin_lock_irq(&drvdata->lock);
	if (drvdata->head != drvdata->tail)
		mask |= POLLIN | POLLRDNORM;
	spin_unlock_irq(&drvdata->lock);

	return mask;
}

static int rotary_encoder_fasync(int fd, struct file *file, int on)
{
	struct rotary_encoder_drvdata *drvdata = container_of(file->private_data, struct rotary_encoder_drvdata, misc);
	return fasync_helper(fd, file, on, &drvdata->fasync);
}

static const struct file_operations rotary_encoder_fops = {
	.owner   = THIS_MODULE,
	.read    = rotary_encoder_read,
	.poll    = rotary_encoder_poll,
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
	
	/* Initialize FIFO */
	drvdata->head = 0;
	drvdata->tail = 0;

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

		/* Generate unique labels for each channel to avoid conflicts */
		label_a = devm_kasprintf(dev, GFP_KERNEL, "rotary_ch%d_a", i);
		label_b = devm_kasprintf(dev, GFP_KERNEL, "rotary_ch%d_b", i);
		label_irq = devm_kasprintf(dev, GFP_KERNEL, "rotary_ch%d_irq", i);
		
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
	dev_info(dev, "Rotary encoder driver initialized with %d channels\n", drvdata->num_channels);

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
