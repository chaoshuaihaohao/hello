#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/platform_device.h>

static struct platform_device *my_device;

static int __init my_device_init(void)
{
	int ret = 0;

	//分配结构
	my_device = platform_device_alloc("my_platform", -1);
	if (!my_device)
		return -ENOMEM;
	
	//注册platform设备
	ret = platform_device_add(my_device);
	//注册失败，释放相关内存
	if (ret)
		platform_device_put(my_device);

	return ret;
}

static void __exit my_device_exit(void)
{
	platform_device_unregister(my_device);
}
module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE( "GPL" );
