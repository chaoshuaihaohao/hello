#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */

#include <linux/delay.h>
#include <linux/jiffies.h>

static int __init test_init(void)
{
	unsigned long start;
	unsigned long total_time;
	int i;

	start = jiffies;
	/* 执行一些任务 */
	for (i = 0; i < 10; i++) {
		mdelay(100);	//delay太长时间会造成系统死掉
		i++;
		i--;
	}
	total_time = jiffies - start;
	printk("That took %lu ticks\n", jiffies_to_clock_t(total_time));
	printk("That took %lu seconds\n", total_time/HZ);

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
