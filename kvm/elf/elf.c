#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define addr_value	0x4009965

int A;			//全局未初始化变量
int B=0;		//全局初始化为0的变量
int C=2;		//全局初始化变量
static int D;
static int E=0;
static int F=4;	//全局静态初始化变量
const int G=5;	//全局常量
const char H=6;	

int main(void)
{
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
	
	int *heap=malloc(sizeof(int)*4);	//堆
	
	printf("PID is:%d\n\n", getpid());
	
	printf("int A\t\t\t\t A_addr=%p\n", &A);
	printf("int B=0\t\t\t\t B_addr=%p\n", &B);
	printf("int C=2\t\t\t\t C_addr=%p\n", &C);
	printf("static int D\t\t\t\t D_addr=%p\n", &D);
	printf("static int E=0\t\t\t\t E_addr=%p\n", &E);
	printf("static int F=4\t\t\t\t F_addr=%p\n", &F);
	printf("const int G=5\t\t\t\t G_addr=%p\n", &G);
	printf("const char H=6\t\t\t\t H_addr=%p\n", &H);
	
	printf("\n");

	printf("int a\t\t\t a_addr=%p\n", &a);
	printf("int b=0\t\t\t b_addr=%p\n", &b);
	printf("int c=2\t\t\t c_addr=%p\n", &c);
	printf("static int d\t\t\t d_addr=%p\n", &d);
	printf("static int e=0\t\t\t e_addr=%p\n", &e);
	printf("static int f=4\t\t\t f_addr=%p\n", &f);
	printf("const int g=5\t\t\t g_addr=%p\n", &g);
	printf("const char h=6\t\t\t h_addr=%p\n", &h);

	printf("\n");
	printf("char char1[]='abcde'\t\t char1_addr=%p\n", char1);
	printf("char *cptr='13456'\t\t cptr_addr=%p\n", &cptr);
	
	printf("value of the cptr\t\t cptr_value=%p\n", cptr);
//	printf("value of the 0x4009965\t\t 0x4009965_value=%d\n", *(char *)addr_value);
	
	printf("int *heap=malloc(sizeof(int)*4)\t heap_addr=%p\n", heap);
	
	pause();
	
	/*程序运行结束之后再回收堆内存，方便观察堆的地址 */
	free(heap);
	printf("PID is:%d\n\n", getpid());
	
	return 0;
}


