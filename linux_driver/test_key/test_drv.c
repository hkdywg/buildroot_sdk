#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

#define GPIO_NUM 6

#define GPIO_BUFFER_LEN 3

#define GPIO6_4_IRQ  384  
#define GPIO6_6_IRQ  386
#define GPIO6_10_IRQ 390

#define TIMER_DELAY 20

#define GPIP6_07  1 
#define GPIP6_05  1
#define GPIP6_13  3

struct gpio_key {
	int gpio;
	int irq;
	enum of_gpio_flags flag;
};

struct device_node *g_node;

static bool Read_flag = true;

static char key_val[GPIO_BUFFER_LEN] = {0xFF,0xFF,0xFF};
static int key_tmp = -1;
static struct gpio_key *gpio_keys;
//1.  timer 
struct timer_list mytimer;

static struct fasync_struct * gpio_trigger_async;
static struct class * fasyncdrv_class;
static struct device * fasyncdrv_class_dev;

static int gpio_trigger_fasync(int fd,struct file *file,int on);
static ssize_t trigger_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos);

static struct file_operations gpio_drv_fops = {
	.owner   =  THIS_MODULE,    
	.read	 =  trigger_drv_read,
	.fasync	 =  gpio_trigger_fasync, 
};

#define GPIO_USED_NUM 3
static int gpio_index[GPIO_USED_NUM] = {384, 386, 390};

static ssize_t trigger_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
   
    if (size != GPIO_BUFFER_LEN)
        return -EINVAL;
	int ret = copy_to_user(buf, key_val, GPIO_BUFFER_LEN);
	if(!ret)
	{
		key_val[0] = 0xFF;
		key_val[1] = 0xFF;
		key_val[2] = 0xFF;
		return GPIO_BUFFER_LEN;
	}
	else 
	{
		printk("copy to  user failure\n");
		return -EFAULT;
	} 
}

static int gpio_trigger_fasync(int fd,struct file *file,int on)
{
	if(fasync_helper(fd,file,on,&gpio_trigger_async) >0)
	{
		return 0;
	}
	else 
	{
		return -EIO;
	}		
}
int major;
static int fasync_drv_init(void)
{
	major = register_chrdev(0, "fasync_drv", &gpio_drv_fops);
	fasyncdrv_class = class_create(THIS_MODULE, "fasync_drv");
	fasyncdrv_class_dev = device_create(fasyncdrv_class, NULL, MKDEV(major, 0), NULL, "gpio_trigger"); /* /dev/buttons */

	return 0;
} 
static void fasync_drv_exit(void)
{
	unregister_chrdev(major, "fasync_drv");
	device_destroy(fasyncdrv_class,MKDEV(major,0));
	class_destroy(fasyncdrv_class);
}

static const struct of_device_id ask100_gpios[] = {
    { .compatible = "100ask,gpio_key" },
    { },
};

//3 timer function
void mytimer_function(struct timer_list *timer)
{
    char i = 0;
#if 0
    for(i = 0; i < GPIO_USED_NUM; i++) {
        if(gpio_keys[i].irq == key_tmp) {
            if(gpio_keys[i].gpio == GPIO6_4_IRQ)
            {
                key_val[0] = gpio_keys[i].irq;
                key_val[1] = gpio_get_value(gpio_keys[i].gpio);
                key_val[2] = gpio_get_value(gpio_keys[i].gpio + GPIP6_05);//gpio6_14
            }
            else if(gpio_keys[i].gpio == GPIO6_6_IRQ)
            {
                key_val[0] = gpio_keys[i].irq;
                key_val[1] = gpio_get_value(gpio_keys[i].gpio);
                key_val[2] = gpio_get_value(gpio_keys[i].gpio + GPIP6_07);//gpio6_7
            }
            else if(gpio_keys[i].gpio == GPIO6_10_IRQ)
            {
                key_val[0] = gpio_keys[i].irq;
                key_val[1] = gpio_get_value(gpio_keys[i].gpio);
                key_val[2] = gpio_get_value(gpio_keys[i].gpio + GPIP6_13);//gpio6_10
            }
        }
    }
    key_tmp = -1;
    kill_fasync(&gpio_trigger_async,SIGIO,POLL_IN);
    //printk("irq key %d val %d\n", irq, gpio_get_value(gpio_key->gpio)); /*gpio_get_value the get value is logic value*/
    printk("key0 = %d, key1 = %d, key2 = %d\n", key_val[0], key_val[1], key_val[2]);
    Read_flag = false;
#endif
    Read_flag = true;
}

static irqreturn_t gpio_key_irq_100ask(int irq, void *dev_id)
{
	struct gpio_key *gpio_key = dev_id;
	gpio_key = dev_id;

	if(!Read_flag)
	    return IRQ_HANDLED;

	//5 start timers
	mod_timer(&mytimer, jiffies + msecs_to_jiffies(TIMER_DELAY));//10ms
#if 1
	key_val[0] = irq;
	key_val[1] = gpio_get_value(gpio_key->gpio);// 385(5) 386(6)  393(13)
	if(gpio_key->gpio == GPIO6_4_IRQ)
	{
		key_val[2] = gpio_get_value((gpio_key->gpio) + GPIP6_05);//gpio6_14
	}
	else if(gpio_key->gpio == GPIO6_6_IRQ)
	{
		key_val[2] = gpio_get_value((gpio_key->gpio) + GPIP6_07);//gpio6_7
	}
	else if(gpio_key->gpio == GPIO6_10_IRQ)
	{
		key_val[2] = gpio_get_value((gpio_key->gpio) + GPIP6_13);//gpio6_10
	}
	else {}
	//printk("gpio value is %d\n", gpio_key->gpio);
	if(Read_flag)
	{
		kill_fasync(&gpio_trigger_async,SIGIO,POLL_IN);
		//printk("irq key %d val %d\n", irq, gpio_get_value(gpio_key->gpio)); /*gpio_get_value the get value is logic value*/
        //printk("key0 = %d, key1 = %d, key2 = %d\n", key_val[0], key_val[1], key_val[2]);
	}
	Read_flag = false;
#else
	key_tmp = irq;
#endif
	return IRQ_HANDLED;
}

#ifdef USE_DEVICE_TREE
static int chip_demo_gpio_probe(struct platform_device *pdev)
#else
static int chip_demo_gpio_probe(void)
#endif
{
	int count;
	int i;
	enum of_gpio_flags flags;
	int gpio;
	int irq;
	int err;
#if 1
	//2 init timer
 	mytimer.expires = jiffies + msecs_to_jiffies(TIMER_DELAY);  
    	timer_setup(&mytimer, mytimer_function, 0);
        //4 register timer
	add_timer(&mytimer);
#endif	
#if 0
	//1.获取设备树节点
	g_node = of_find_node_by_path("/gpio_keys_100ask"); 
	if(NULL == g_node){
		printk("of_find_node_path error\n");
		return -EINVAL;
	}
#endif

#ifdef USE_DEVICE_TREE
	struct device_node *node = pdev->dev.of_node;
	count = of_gpio_count(node);
#else
	count = GPIO_USED_NUM;
#endif
	//printk( "jh conunt is %d\n",count);
	gpio_keys = kzalloc(count * sizeof(struct gpio_key), GFP_KERNEL);
	if(!gpio_keys)
	{
		return  -ENOMEM;
	}
	for (i = 0; i < count; i++)
	{
#ifdef USE_DEVICE_TREE
		gpio = of_get_gpio_flags(node, i, &flags);
#else
		gpio = gpio_index[i];
        flags = OF_GPIO_SINGLE_ENDED;
#endif 
        gpio_direction_input(gpio);
		irq = gpio_to_irq(gpio);	
        printk("gpio num is %d, irq is %d\n", gpio, irq);
		gpio_keys[i].gpio = gpio;
		gpio_keys[i].irq  = irq;
		gpio_keys[i].flag = flags;
		//err = request_irq(irq, gpio_key_irq_100ask, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "100ask_gpio_key", &gpio_keys[i]);
		err = request_irq(irq, gpio_key_irq_100ask, IRQF_TRIGGER_FALLING, "100ask_gpio_key", &gpio_keys[i]);
		if(err)
		{
			printk(KERN_ERR "jhh failed to request GPIO IRQ\n");
			return err;
		}
	}

	return 0;
}
#ifdef USE_DEVICE_TREE
static int chip_demo_gpio_remove(struct platform_device *pdev)
#else
static int chip_demo_gpio_remove(void)
#endif
{
	int count;
	int i;
#ifdef USE_DEVICE_TREE
	struct device_node *node = pdev->dev.of_node;
	count = of_gpio_count(node);
#else
    count = GPIO_USED_NUM;
#endif
	for (i = 0; i < count; i++)
	{
		free_irq(gpio_keys[i].irq, &gpio_keys[i]);
	}
	//6 del timer
	del_timer(&mytimer);
    	return 0;
}

#ifdef USE_DEVICE_TREE
/* 1. 定义platform_driver */
static struct platform_driver test_gpio_drv = {
    .probe      = chip_demo_gpio_probe,
    .remove     = chip_demo_gpio_remove,
    .driver     = {
        .name   = "100ask_led",
        .of_match_table = ask100_gpios,
    },
};
#endif

static int test_gpio_drv_init(void)
{
	int ret;
#ifdef USE_DEVICE_TREE
	ret = platform_driver_register(&test_gpio_drv);
#else
    ret = chip_demo_gpio_probe();
#endif
	if (ret)
	{
		printk(KERN_ERR "init:jhh gpio_drv_init failure: %d\n", ret);
		return ret;
	}
        fasync_drv_init();
	
	return 0;
}

static void test_gpio_drv_exit(void)
{
#ifdef USE_DEVICE_TREE
	platform_driver_unregister(&test_gpio_drv);
#else
    chip_demo_gpio_remove();
#endif
	fasync_drv_exit();
}

module_init(test_gpio_drv_init);
module_exit(test_gpio_drv_exit);

MODULE_LICENSE("GPL");



