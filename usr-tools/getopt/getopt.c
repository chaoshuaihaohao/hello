#include <stdio.h>
#include <getopt.h>

char *l_opt_arg;

char *const short_options = "nbl:";	//有参数的需要跟':'。

/*
struct option {
	const char *name;	//name表示的是长参数名
	int has_arg;
		//has_arg有3个值，no_argument(或者是0)，表示该参数后面不跟参数值
		// required_argument(或者是1),表示该参数后面一定要跟个参数值
		// optional_argument(或者是2),表示该参数后面可以跟，也可以不跟参数值
	int *flag;
		//用来决定，getopt_long()的返回值到底是什么。如果这个指针为NULL，
		//那么getopt_long()返回该结构val字段中的数值。如果该指针不为NULL，
		//getopt_long()会使得它所指向的变量中填入val字段中的数值，并且getopt_long()返回0。
		//如果flag不是NULL，但未发现长选项，那么它所指向的变量的数值不变。
	int val;
		//和flag联合决定返回值 这个值是发现了长选项时的返回值，
		//或者flag不是 NULL时载入*flag中的值。典型情况下，若flag不是NULL，
		//那么val是个真／假值，譬如1 或0；另一方面，如 果flag是NULL，那么val通常是字符常量，
		//若长选项与短选项一致，那么该字符常量应该与optstring中出现的这个选项的参数相同。
}
*/

struct option long_options[] = {
	{ "name", 0, NULL, 'n' },
	{ "bf_name", 0, NULL, 'b' },
	{ "love", 1, NULL, 'l' },
	{ 0, 0, 0, 0 },
};

int main(int argc, char *argv[])
{
	int c;
	while ((c =
		getopt_long(argc, argv, short_options, long_options,
			    NULL)) != -1) {
		switch (c) {
		case 'n':
			printf("My name is XL.\n");
			break;
		case 'b':
			printf("His name is ST.\n");
			break;
		case 'l':
			l_opt_arg = optarg;
			printf("Our love is %s!\n", l_opt_arg);
			break;
		}
	}
	return 0;
}

/* getopt_long介绍 */
/* getopt_long 函数解析命令行参数，argc、argv是调用main函数时传入的参数。
 * 传入的’-'开始的字符被解析为选项，getopt_long 一次执行解析出一个option，
 * 如果循环执行，可以将argv中的全部option解析出来； */
/* 在getopt_long 的执行中，每次进入都会更新getopt_long变量，该变量指向下一个argv参数； */
/* 如getopt_long 返回 - 1，表示argv[]中的所有选项被解析出，optind指向第一个非选项的argument元素；
 * 这里要注意，在getopt_long 执行过程中会将单独的argument交换到argv数组的后面，option选项提前，
 * 如：cmd - a file1 - b file2, 
 * 如果a / b均为不带参数的选项，这最终argv数组变为：cmd - a - b file1 file2; */

/* optstring指定选项合法的选项，一个字符代表一个选项，
 * 在字符后面加一个’:‘表示该选项带一个参数，
 * 字符后带两个’:'表示该选项带可选参数(参数可有可无)，
 * 若有参数，optarg指向该该参数，否则optarg为0； */

/* 前面说了getopt_long 会进行argv顺序的调整，但也可以通过设置optstring改变它的方式，这里有两种： */
/* 如果optstring的第一个参数是’+'或者POSIXLY_CORRECT被设置，
 * 则getopt_long 在原argv的顺序上遇到第一个非选项就返回 - 1； */
/* 如果optstring的第一个参数是’-’，
 * 则会将所有的非选项当选项处理，并且返回1，用字符代码1表示该选项； */
/* 如果getopt_long 不能识别一个选项字符，它会打印一个错误消息到stderr上，
 * 并将该字符存放到optopt中，返回’?’；调用程序可以设置opterr = 0设置不打印错误信息；
 * 注意：要使能打印错误信息，optstring的第一个字符（或者在第一个字符是 + / -之后）不能是’:’，否则也不会打印错误; */
/* 如果optstring中指定了option需要参数，但在命令行没有参数，那么getopt_long 将返回’?’，
 * 如果在optstring的第一个字符（或者在第一个字符是 + / -之后）是’:’，那么将返回’:’; */
/* ###注意： */
/* required_argument(或者是1)时，参数输入格式为：–参数 值 或者 --参数=值。 */
/* optional_argument(或者是2)时，参数输入格式只能为：–参数=值。 */
