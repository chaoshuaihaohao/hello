#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */
#include <asm/bitops.h>

static volatile unsigned long word = 0;
static int __init test_init(void)
{
	set_bit(1, &word);
	printk("%lu\n", word);
	set_bit(0, &word);
	printk("%lu\n", word);

	return 0;
}

static void __exit test_exit(void)

{
	printk(KERN_ALERT"Goodbye world!\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("ko templete");
MODULE_LICENSE("GPL v2");
