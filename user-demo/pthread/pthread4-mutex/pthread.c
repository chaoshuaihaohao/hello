//预期是线程打印的count值是不一致的。
//结果是count值不稳定，有时两个线程打印一样的值。

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//共享资源
//可以是队列、链表；比如网络收发包请求队列,IO请求队列
//load count--->ops count--->str count
//           load count---          >ops count--->str count
static int count = 0;

//线程函数1
void *my_thread1()
{
	int i;
	for (i = 0; i < 10000; i++) {
		pthread_mutex_lock(&lock);
		count++;
		pthread_mutex_unlock(&lock);
		//printf("I am the 1st pthread, count=%d\n", count);
		usleep(10);
	}
	//printf("I am the 1st pthread, count=%d\n", count);
}

//线程函数2
void *my_thread2()
{
	int i;
	for (i = 0; i < 10000; i++) {
		pthread_mutex_lock(&lock);
		//count--;
		count++;
		pthread_mutex_unlock(&lock);
		//printf("I am the 2st pthread, count=%d\n", count);
		usleep(10);
	}
	//printf("I am the 2st pthread, count=%d\n", count);
}

int main()
{
	pthread_t id1, id2;	//线程id
	int ret;

	ret = pthread_create(&id1, NULL, &my_thread1, NULL);
	if (ret) {
		printf("Create pthread1 error!\n");
		return ret;
	}

	ret = pthread_create(&id2, NULL, &my_thread2, NULL);
	if (ret) {
		printf("Create pthread2 error!\n");
		return ret;
	}
	//等待两个线程均退出后，main函数再退出。
	//以阻塞的方式等待thread指定的线程结束。
	//没有pthread_join，主进程直接运行结束，不会等线程执行完毕。
	pthread_join(id1, NULL);
	pthread_join(id2, NULL);

	printf("hello world: count=%d\n", count);

	return 0;
}
