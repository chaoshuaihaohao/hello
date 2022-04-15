#include <stdio.h>
#include <hhh.h>

int main()
{
	hhh();
	return 0;
}

void hhh(void)
{
	if (CRON > 0)
		printf("first's header\n");
	else
		printf("last's header\n");
}
