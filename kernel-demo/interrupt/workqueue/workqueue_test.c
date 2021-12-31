/* 工作队列是Linux操作系统中，进行中断下半部分处理的重要方式！ */
/* 从名称上可以猜到：一个工作队列就好像业务层常用的消息队列一样， */
/* 里面存放着很多的工作项等待着被处理。 */
/* 工作队列中有两个重要的结构体： */
/* 	工作队列(workqueue_struct) 和 工作项(work_struct) */

/* 在内核中，工作队列中的所有工作项，是通过链表串在一起的， */
/* 并且等待着操作系统中的某个线程挨个取出来处理。 */
/* 这些线程，可以是由驱动程序通过 kthread_create 创建的线程， */
/* 也可以是由操作系统预先就创建好的线程。 */
/* 这里就涉及到一个取舍的问题了。 */
/* 如果我们的处理函数很简单，那么就没有必要创建一个单独的线程来处理了。 */

/* 原因有二： */
/* 创建一个内核线程是很耗费资源的，如果函数很简单， */
/* 很快执行结束之后再关闭线程，太划不来了，得不偿失; */
/* 如果每一个驱动程序编写者都毫无节制地创建内核线程， */
/* 那么内核中将会存在大量不必要的线程， */
/* 当然了本质上还是系统资源消耗和执行效率的问题; */
/* 为了避免这种情况，于是操作系统就为我们预先创建好一些工作队列和内核线程。 */
/* 我们只需要把需要处理的工作项， */
/* 直接添加到这些预先创建好的工作队列中就可以了， */
/* 它们就会被相应的内核线程取出来处理。 */
/* 例如下面这些工作队列，就是内核默认创建的（include/linux/workqueue.h）： */

/* 此外，由于工作队列 system_wq 被使用的频率很高， */
/* 于是内核就封装了一个简单的函数(schedule_work)给我们使用： */

/* 当然了，任何事情有利就有弊！ */
/* 由于内核默认创建的工作队列，是被所有的驱动程序共享的。 */
/* 如果所有的驱动程序都把等待处理的工作项委托给它们来处理， */
/* 那么就会导致某个工作队列中过于拥挤。 */
/* 根据先来后到的原则，工作队列中后加入的工作项， */
/* 就可能因为前面工作项的处理函数执行的时间太长，从而导致时效性无法保证。 */
/* 因此，这里存在一个系统平衡的问题。 */
/* 关于工作队列的基本知识点就介绍到这里，下面来实际操作验证一下。 */

/* 测试场景是：加载驱动模块之后，如果监测到键盘上的ESC键被按下，那么就往内核默认的工作队列system_wq中增加一个工作项，然后观察该工作项对应的处理函数是否被调用。 */

/* 编译、测试 */

/* $ make */
/* $ sudo insmod workqueue_test.ko irq=1 devname=mydev */
/* 检查驱动模块是否加载成功： */
/* $ lsmod | grep my_driver_interrupt_wq */
/* my_driver_interrupt_wq    16384  0 */
/* 再看一下 dmesg 的输出信息： */
/* $ dmesg */
/* ... */
/* [  188.247636] myirq_init is called. */
/* [  188.247642] register irq[1] handler success. */

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_ALERT */
#include <linux/init.h>		/*Needed for __init */
#include <linux/interrupt.h>

#include <linux/kdb.h>                                                                                                                                                  
#include <linux/keyboard.h>                                                                                                                                             
#include <linux/ctype.h>                                                                                                                                                
#include <linux/io.h>

static int irq;
static char *devname;

static struct work_struct mywork;

 // 接收驱动模块加载时传入的参数
module_param(irq, int, 0644);
module_param(devname, charp, 0644);

// 定义驱动程序的 ID，在中断处理函数中用来判断是否需要处理                    
#define MY_DEV_ID	   		1226

// 驱动程序数据结构
struct myirq {
	int devid;
};

struct myirq mydev = { MY_DEV_ID };

#define KBD_DATA_REG        0x60
#define KBD_STATUS_REG      0x64
#define KBD_SCANCODE_MASK   0x7f
#define KBD_STATUS_MASK     0x80

// 工作项绑定的处理函数
static void mywork_handler(struct work_struct *work)
{
	printk("mywork_handler is called. \n");
	// do some other things
}

//中断处理函数
static irqreturn_t myirq_handler(int irq, void *dev)
{
	struct myirq mydev;
	unsigned char key_code;
	mydev = *(struct myirq *)dev;

	// 检查设备 id，只有当相等的时候才需要处理
	if (MY_DEV_ID == mydev.devid) {
		// 读取键盘扫描码
		key_code = inb(KBD_DATA_REG);

		if (key_code == 0x01) {
			printk("ESC key is pressed! \n");

			// 初始化工作项
			INIT_WORK(&mywork, mywork_handler);

			// 加入到工作队列 system_wq
			schedule_work(&mywork);
		}
	}

	return IRQ_HANDLED;
}

// 驱动模块初始化函数
static int __init myirq_init(void)
{
	printk("myirq_init is called. \n");

	// 注册中断处理函数
	if (request_irq(irq, myirq_handler, IRQF_SHARED, devname, &mydev) != 0) {
		printk("register irq[%d] handler failed. \n", irq);
		return -1;
	}

	printk("register irq[%d] handler success. \n", irq);
	return 0;
}

// ���动模块退出函数
static void __exit myirq_exit(void)
{
	printk("myirq_exit is called. \n");

	// 释放中断处理函数
	free_irq(irq, &mydev);
}

module_init(myirq_init);
module_exit(myirq_exit);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("ko templete");
MODULE_LICENSE("GPL v2");
