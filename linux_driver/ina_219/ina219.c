#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#define DEVICE_NAME "current_sensens"
#define INA219_PKG_SIZE	2

#define INA219_REG_CONFIG      0x00
#define INA219_REG_SHUNTVOLTAGE 0x01
#define INA219_REG_BUSVOLTAGE  0x02
#define INA219_REG_POWER       0x03
#define INA219_REG_CURRENT     0x04
#define INA219_REG_CALIBRATION 0x05

#define INA219_CONFIG_VALUE 0x3eef  


#define ina219_i2c_write(a,b) regmap_write(regmap_ina219, a, b);
#define ina219_i2c_read(a,b)  regmap_read(regmap_ina219, a, b);

struct ina219_i2c {
	struct i2c_client *client;
    unsigned short data[INA219_PKG_SIZE];
};

static const struct regmap_range ti_current_sensens_volatile_ranges[] = {
    { .range_min = 0, .range_max = 0xFF },
};

static const struct regmap_access_table ti_current_sensens_table = {
    .yes_ranges = ti_current_sensens_volatile_ranges,
    .n_yes_ranges = ARRAY_SIZE(ti_current_sensens_volatile_ranges),
};

static const struct regmap_config ina219_regmap_config = {
    .reg_bits = 8,
    .val_bits = 16,
    .volatile_table = &ti_current_sensens_table,
    .cache_type = REGCACHE_NONE,
};

static int major_num;
static struct cdev cdev;
static struct class *class;
static struct regmap *regmap_ina219;
static struct ina219_i2c ina219_instance;

static int ina219_open(struct inode *inode, struct file *file)
{
    int ret = 0;
    regmap_ina219 = devm_regmap_init_i2c(ina219_instance.client, &ina219_regmap_config);
    if(IS_ERR(regmap_ina219)) {
        ret = PTR_ERR(regmap_ina219);
        return ret;
    }

    ina219_i2c_write(INA219_REG_CONFIG, INA219_CONFIG_VALUE);

	return 0;
}

static ssize_t ina219_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    unsigned int shunt_voltage;
    unsigned int bus_voltage;
    unsigned int power_voltage;
    ssize_t data_len =  sizeof(ina219_instance.data);

    ina219_i2c_read(INA219_REG_BUSVOLTAGE, &bus_voltage);
    /* if convert is not complete, delay 20ms, and read */
    if(!(bus_voltage & 0x02)) 
    {
        msleep(20);
        ina219_i2c_read(INA219_REG_BUSVOLTAGE, &bus_voltage);
    }
    ina219_i2c_read(INA219_REG_SHUNTVOLTAGE, &shunt_voltage);
    /* reading eth power register to clear CNVR flag */
    ina219_i2c_read(INA219_REG_POWER, &power_voltage);  

    ina219_instance.data[0] = shunt_voltage; 
    ina219_instance.data[1] = (bus_voltage >> 3);

    if(copy_to_user(buf,((char *)ina219_instance.data), data_len)) {
        return -EFAULT;
    }

    return data_len;
}

static void ina219_i2c_close(struct input_dev *dev)
{

}

static struct file_operations fops = {
    .open = ina219_open,
    .read = ina219_read,
};

static int ina219_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
    dev_t dev;
    int ret = 0;

    if(ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
        printk(KERN_ERR "Failed to allocate character device number!\n");
        return ret;
    }

    major_num = MAJOR(dev);

    cdev_init(&cdev, &fops);
    cdev.owner = THIS_MODULE;
    if(ret = cdev_add(&cdev, dev, 1)) {
        unregister_chrdev_region(dev, 1);
        printk(KERN_ERR "Failed to create class!\n");
        return ret;
    }

    class = class_create(THIS_MODULE, DEVICE_NAME);
    if(IS_ERR(class)) {
        cdev_del(&cdev);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ERR "Failed to create class!\n");
        return PTR_ERR(class);
    }

    device_create(class, NULL, dev, NULL, DEVICE_NAME);

    memset(&ina219_instance, 0, sizeof(struct ina219_i2c));
    ina219_instance.client = client;

	return 0;
}

static int ina219_remove(struct i2c_client *client) 
{
    dev_t dev = MKDEV(major_num, 0);

    device_destroy(class, dev);
    class_destroy(class);

    cdev_del(&cdev);

    unregister_chrdev_region(dev, 1);

    return 0;
}

static int __maybe_unused ina219_i2c_suspend(struct device *dev)
{

	return 0;
}

static int __maybe_unused ina219_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	enable_irq(client->irq);

	return 0;
}

static SIMPLE_DEV_PM_OPS(ina219_i2c_pm, ina219_i2c_suspend, ina219_i2c_resume);

static const struct i2c_device_id ina219_i2c_id[] = {
	{ "ina219", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ina219_i2c_id);

static const struct of_device_id ina219_i2c_of_match[] = {
	{ .compatible = "Texas Instrunments,ina219-i2c", },
	{ }
};
MODULE_DEVICE_TABLE(of, ina219_i2c_of_match);

static struct i2c_driver ina219_i2c_driver = {
	.driver	= {
		.name	= "ina219_i2c",
		.pm	= &ina219_i2c_pm,
		.of_match_table = ina219_i2c_of_match,
	},

	.probe		= ina219_i2c_probe,
    .remove     = ina219_remove,
	.id_table	= ina219_i2c_id,
};
module_i2c_driver(ina219_i2c_driver);

MODULE_AUTHOR("yinwg");
MODULE_DESCRIPTION("Texas Instrunments ina219 I2C Driver");
MODULE_LICENSE("GPL");

