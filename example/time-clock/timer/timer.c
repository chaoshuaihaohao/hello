#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */

#include <linux/timer.h>
#include <linux/jiffies.h>


/*
 * 入参timer:指向定时器my_timer.
 *	在设备驱动中，定时器放在dev私有数据结构中，可通过from_timer()从timer获取dev私有数据结构.
 *	#define from_timer(var, callback_timer, timer_fieldname)
 *		var是dev私有数据结构类型;
 *		callback_timer是struct timer_list *timer;
 *		timer_fieldname是dev私有数据结构中定时器变量名my_timer.
 * */
void my_timer_function(struct timer_list *timer)
{
	printk("do time function!\n");
	timer->expires = jiffies + 500 * HZ/1000;	// 500ms 运行一次.HZ看作1s.
	mod_timer(timer, timer->expires);		//定时器运行一次就自己删除了。如果需要周期性执行任务，在定时器回调函数中添加 mod_timer.
}

static int __init test_init(void)
{
	//创建定时器时需要先定义它
	struct timer_list my_timer;

	//接着需要通过一个辅助函数来初始化定时器数据结构的内部值，
	//初始化必须在使用其他定时器管理函数对定时器进行操作前完成。
	
	//init_timer(&my_timer);
	//填充定时器结构中需要的值
//	unsigned long delay = 100;
//	my_timer.expires = jiffies + delay; /* 定时器超时时的节拍数 */
//	my_timer.function = my_timer_function;	/* 定时器超时时调用的函数 */
	timer_setup(&my_timer, my_timer_function, 0);

	my_timer.expires = jiffies + 500 * HZ/1000; /* 定时器超时时的节拍数 */

	//激活定时器
	add_timer(&my_timer);
	printk(KERN_ALERT "add timer succeed!\n");

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
