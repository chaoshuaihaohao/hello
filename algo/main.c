#include <stdio.h>  
#include <stdlib.h>  
#include <time.h>  
  
// 声明随机生成10到20之间数的函数  
int generateRandomNumber() {  
    // 使用当前时间作为随机数生成的种子  
    srand((unsigned)time(NULL));  
    // 生成10到20之间的随机数（包括10和20）  
    // 注意：rand() % 11 会生成0到10之间的数，所以要调整为10到20  
    return 6 + rand() % 12;
}  
  
struct income {
	int salary;
	int fund;	//company
	float n;
	int work_month; // month in company
};

/* Layoff time: Time until the current month */
static int l_time = 0;

/* Current monthly salary */
static int c_salary = 3;

static int gross_salary = 0;

static int year_end_bonus = 0;

int main() {  
	
	l_time = generateRandomNumber();

	struct income old;

	old.salary = 30000;
	old.fund = old.salary * 12 / 100;
	old.work_month = 27;
	old.n = (old.work_month / 6)*0.5 + 0.5;

	if (l_time > 6)
		year_end_bonus = old.salary * 1.5;

	gross_salary = (old.salary + old.fund) * l_time + (old.n + 1) * old.salary + year_end_bonus;
        printf("Random Number: %d, year_end_bonus=%d, gross_salary=%d\n", l_time, year_end_bonus, gross_salary);
      
    return 0;  
}
