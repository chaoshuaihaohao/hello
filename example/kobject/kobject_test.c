#include <linux/kobject.h>/* Needed for kobject */
#include <linux/kernel.h>/* Needed by printk */
#include <linux/module.h>/* Needed by all modules */
#include <linux/init.h>/* Needed for __init */
#include <linux/sysfs.h>/* Needed for __init */

void kobj_test_release(struct kobject *kobject)
{
	printk("realse\n");
}

static ssize_t kobj_test_show(struct kobject *kobject, struct attribute *attr, char *buf)
{
	printk("show\n");
	printk("attrname:%s\n", attr->name);
	sprintf(buf, "%s\n", attr->name);
	
	return strlen(attr->name)+2;
}

static ssize_t kobj_test_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	printk("store\n");

	return count;
}

static struct attribute test_attr = {
	.name = "test_addr",
	.mode = S_IRWXUGO,
};

static struct attribute test_attr1 = {
	.name = "test_addr1",
	.mode = S_IRWXUGO,
};

static struct attribute *def_attrs[] = {
	&test_attr,
	&test_attr1,
	NULL,
};

static struct sysfs_ops kobj_test_sysfs_ops =
{
	.show = kobj_test_show,
	.store = kobj_test_store,
};

static struct kobj_type ktype =
{
	.release = kobj_test_release,
	.sysfs_ops = &kobj_test_sysfs_ops,
	.default_attrs = def_attrs,
};

static struct kobject kobj;
static int __init kobject_test_init(void)
{
	printk(KERN_INFO "kobject test init\n");
	
	kobject_init_and_add(&kobj, &ktype, NULL, "kobject_test");

	return 0;
}


static void __exit kobject_test_exit(void)
{
	kobject_del(&kobj);

	printk(KERN_INFO "kobject test exit\n");
}

module_init(kobject_test_init);
module_exit(kobject_test_exit);

MODULE_LICENSE( "GPL" );
