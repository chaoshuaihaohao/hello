#include <linux/kobject.h>
#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */

struct kobject *hello_kobj;

static int __init test_init(void)
{
	hello_kobj = kobject_create_and_add("hello", NULL);
	if (!hello_kobj)
		return -ENOMEM;

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
