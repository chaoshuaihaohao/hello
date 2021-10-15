#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/device/bus.h>

extern struct device my_bus;
extern struct bus_type my_bus_type;

static int my_probe(struct device *dev)
{
	printk("Driver found device which my driver can handle!\n");

	return 0;
}

static int my_remove(struct device *dev)
{
	printk("Driver found device unpluged!\n");

	return 0;
}

struct device_driver my_drv = {
	.name = "my_dev",
	.bus = &my_bus_type,
	.probe = &my_probe,
	.remove = &my_remove,
};

static ssize_t drv_show(struct device_driver *driver, char *buf)
{
	return sprintf(buf, "%s\n", "This is my driver");
}

static DRIVER_ATTR_RO(drv);

static int __init my_driver_init(void)
{
	int ret = 0;

	//注册设备
	driver_register(&my_drv);

	//创建属性文件
	driver_create_file(&my_drv, &driver_attr_drv);

	return ret;
}

static void __exit my_driver_exit(void)
{
	driver_unregister(&my_drv);
}
module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE( "GPL" );
