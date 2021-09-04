#include <linux/module.h>/* Needed by all modules */
#include <linux/kernel.h>/* Needed for KERN_ALERT */
#include <linux/init.h>/*Needed for __init */
#include <linux/slab.h>

#if 1
#define ALLOC_OBJECT
#endif

struct my_struct {
	struct list_head list;
	void *data;
	int a[118];
};

static struct kmem_cache *test_kmem_cache;
static struct my_struct *test_struct;

static int __init test_init(void)
{

	//创建一个名为test_kmem_cache的高速缓存，
	//每个对象的大小是500字节.500分配504;512分配失败;602分配608;612分配616;没懂:-<,有概率在/proc/slabinfo中看不到分配的高速缓存。
	//对齐方式.500大小使用500对齐，分配的是520.没懂:-<
	//slab标志，是可选的，０表示没有特殊行为。
	//构造函数不使用，已经被抛弃，NULL。
	test_kmem_cache = kmem_cache_create("test_kmem_cache", sizeof(struct my_struct), 0, 0, NULL);
	if (!test_kmem_cache)
		printk( KERN_ALERT "test_kmem_cache create failed\n");
	printk("object size = %lu\n", sizeof(struct my_struct));

#ifdef ALLOC_OBJECT
	test_struct = kmem_cache_alloc(test_kmem_cache, GFP_KERNEL);
	if (!test_struct)
		printk( KERN_ALERT "alloc object from test_kmem_cache failed\n");
#endif

	printk(KERN_ALERT"Hello world!\n");

	return 0;
}

static void __exit test_exit(void)
{
#ifdef ALLOC_OBJECT
	kmem_cache_free(test_kmem_cache, test_struct);
#endif
	kmem_cache_destroy(test_kmem_cache);
	printk(KERN_ALERT"Goodbye world!\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("ko templete");
MODULE_LICENSE("GPL v2");
