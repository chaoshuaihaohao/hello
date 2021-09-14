#include <linux/kobject.h>
#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */

struct kobject *hello_kobj;

static ssize_t hello_world_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%d\n", 0);
}

static ssize_t hello_world_store(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 const char *buf, size_t size)
{
        bool val;
        int ret;

        ret = strtobool(buf, &val);
        if (ret < 0)
                return ret;

        if (val) {
                printk("hello world!\n");
                return -EINVAL;
        }
        return size;
}

static const struct kobj_attribute hello_world_attr =
	__ATTR(hello_world, S_IRUGO | S_IWUSR, hello_world_show,
	       hello_world_store);

static int __init test_init(void)
{
	int result;

	hello_kobj = kobject_create_and_add("hello", NULL);
	if (!hello_kobj)
		return -ENOMEM;
	result = sysfs_create_file(hello_kobj, &hello_world_attr.attr);
	if (result)
		return result;


	kobject_uevent(hello_kobj, KOBJ_ADD);

	printk(KERN_ALERT"Hello world!\n");

	return 0;
}

static void __exit test_exit(void)
{
	kobject_del(hello_kobj);

	printk(KERN_ALERT"Goodbye world!\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE( "GPL" );
