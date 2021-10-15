#include <linux/kobject.h>/* Needed for kobject */
#include <linux/kernel.h>/* Needed by printk */
#include <linux/module.h>/* Needed by all modules */
#include <linux/init.h>/* Needed for __init */

int kset_filter(struct kset *kset, struct kobject *kobj)
{
	printk("filter: kobj %s\n", kobj->name);

	return 1;
}

const char *kset_name(struct kset *kset, struct kobject *kobj)
{
	static char buf[20];
	printk("name: kobj %s.\n", kobj->name);
	sprintf(buf, "%s", "kset_name");
	
	return buf;
}

int kset_uevent(struct kset *kset, struct kobject *kobj, struct kobj_uevent_env *env)
{
	int i = 0;
	printk("uevent: kobj %s.\n", kobj->name);

	while ( i < env->envp_idx ) {
		printk("%s.\n", env->envp[i]);
		i++;
	}
	
	return 0;
}

struct kset_uevent_ops uevent_ops = {
	.filter = kset_filter,
	.name = kset_name,
	.uevent = kset_uevent,
};

struct kset kset_p;
struct kset kset_c;

static int __init kset_test_init(void)
{
	printk(KERN_INFO "kset test init\n");

	kobject_set_name(&kset_p.kobj, "kset_p");
	kset_p.uevent_ops = &uevent_ops;
	kset_register(&kset_p);
	
	kobject_set_name(&kset_c.kobj, "kset_c");
	kset_c.kobj.kset = &kset_p;
	kset_register(&kset_c);
	
	return 0;
}


static void __exit kset_test_exit(void)
{
	kset_unregister(&kset_p);
	kset_unregister(&kset_c);

	printk(KERN_INFO "kset test exit\n");
}

module_init(kset_test_init);
module_exit(kset_test_exit);

MODULE_LICENSE( "GPL" );
