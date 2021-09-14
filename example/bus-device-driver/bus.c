#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/device/bus.h>

static void my_bus_release(struct device *dev)
{
}

struct device my_bus = {
	.init_name = "my_bus0",
	.release = my_bus_release,
};
EXPORT_SYMBOL(my_bus);

static int my_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev->kobj.name, driver->name, strlen(driver->name));
}

struct bus_type my_bus_type = {
	.name = "my_bus",
	.match = my_match,
};
EXPORT_SYMBOL(my_bus_type);

static char *Version = "1.10";
ssize_t version_show(struct bus_type *bus, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", Version);
}

static BUS_ATTR_RO(version);

static int __init my_bus_init(void)
{
	int ret;

	//注册总线
	ret = bus_register(&my_bus_type);
	if (ret)
		return ret;

	//创建属性文件
	if (bus_create_file(&my_bus_type, &bus_attr_version))
		printk(KERN_NOTICE "Failed to crate version attribute\n");

	//注册总线设备
	ret = device_register(&my_bus);
	if (ret)
		printk(KERN_NOTICE "Failed to register device: my_bus!\n");

	return ret;
}

static void __exit my_bus_exit(void)
{
	device_unregister(&my_bus);
	bus_unregister(&my_bus_type);
}
module_init(my_bus_init);
module_exit(my_bus_exit);

MODULE_LICENSE( "GPL" );
