#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */

#include <linux/unistd.h>
#include <linux/mm.h>


#define addr_value	0x4009965

int A;			//全局未初始化变量
int B=0;		//全局初始化为0的变量
int C=2;		//全局初始化变量
static int D;
static int E=0;
static int F=4;	//全局静态初始化变量
const int G=5;	//全局常量
const char H=6;	

static int elf(void)
{
	struct page *page;
	void *va;

	int a;	//局部未初始化变量
	int b=0;//局部初始化为0的变量
	int c=2;
	
	static int d;
	static int e=0;
	static int f=4;	//全局静态初始化变量
	const int g=5;	//全局常量
	const char h=6;	

	char char1[]="abcde";//局部字符数组
	
	char *cptr="13456";	//p在栈上，13456在常量区
	
	int *heap=kvmalloc(sizeof(int)*4, GFP_KERNEL);	//堆
	
	printk("PID is:%d\n\n", current->pid);
	
	printk("int A\t\t\t\t A_addr=%p\n", &A);
	printk("int B=0\t\t\t\t B_addr=%p\n", &B);
	printk("int C=2\t\t\t\t C_addr=%p\n", &C);
	printk("static int D\t\t\t\t D_addr=%p\n", &D);
	printk("static int E=0\t\t\t\t E_addr=%p\n", &E);
	printk("static int F=4\t\t\t\t F_addr=%p\n", &F);
	printk("const int G=5\t\t\t\t G_addr=%p\n", &G);
	printk("const char H=6\t\t\t\t H_addr=%p\n", &H);
	
	printk("\n");

	printk("int a\t\t\t a_addr=%p\n", &a);
	printk("int b=0\t\t\t b_addr=%p\n", &b);
	printk("int c=2\t\t\t c_addr=%p\n", &c);
	printk("static int d\t\t\t d_addr=%p\n", &d);
	printk("static int e=0\t\t\t e_addr=%p\n", &e);
	printk("static int f=4\t\t\t f_addr=%p\n", &f);
	printk("const int g=5\t\t\t g_addr=%p\n", &g);
	printk("const char h=6\t\t\t h_addr=%p\n", &h);

	printk("\n");
	printk("char char1[]='abcde'\t\t char1_addr=%p\n", char1);
	printk("char *cptr='13456'\t\t cptr_addr=%p\n", &cptr);
	
	printk("value of the cptr\t\t cptr_value=%p\n", cptr);
//	printk("value of the 0x4009965\t\t 0x4009965_value=%d\n", *(char *)addr_value);
	
	printk("int *heap=kvmalloc(sizeof(int)*4)\t heap_addr=%p\n", heap);
	
//	pause();
	
	/*程序运行结束之后再回收堆内存，方便观察堆的地址 */
	kvfree(heap);

	page = alloc_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	va = page_address(page);
	printk("device_buffer physical address: %lx, virtual address: %px\n",
		page_to_pfn(page) << PAGE_SHIFT, va);

	free_page((unsigned long)va);
	printk("PID is:%d\n\n", current->pid);
	
	return 0;
}

static int __init test_init(void)
{
	printk(KERN_ALERT"Hello world!\n");
	elf();

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

