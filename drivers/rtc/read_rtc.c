#include <asm/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/rtc.h>
#include <time.h>
#include <unistd.h>

/* RTC_RD_TIME etc have this definition since 1.99.9 (pre2.0-9) */
#ifndef RTC_RD_TIME
//# define RTC_RD_TIME    _IOR('p', 0x09, struct rtc_time)
//# define RTC_SET_TIME   _IOW('p', 0x0a, struct linux_rtc_time)
//# define RTC_UIE_ON     _IO('p', 0x03)  /* Update int. enable on */
//# define RTC_UIE_OFF    _IO('p', 0x04)  /* Update int. enable off */
#endif

int main()
{
	char *dev = "/dev/rtc1";
	struct tm nowtime;
	int ret;

	int fd = open(dev, O_RDWR, 0664);
	if (fd < 0)
		printf("open dev failed\n");
#if 1
	ret = ioctl(fd, RTC_RD_TIME, &nowtime);
	if (ret == 0)
		printf("%d/%d/%d %d:%d %02d\n", nowtime.tm_year + 1900,
		       nowtime.tm_mon, nowtime.tm_mday, nowtime.tm_hour,
		       nowtime.tm_min, nowtime.tm_sec);
#endif

#if 0
	/* Set the rtc time */
	nowtime.tm_year = 21;
	ret = ioctl(fd, RTC_SET_TIME, &nowtime);
	if (ret)
		printf("Error: set time failed, %d\n", ret);
#endif

	float time_use = 0;
	struct timeval start;
	struct timeval end;

	gettimeofday(&start, NULL);	//gettimeofday(&start,&tz);结果一样
	printf("start.tv_sec:%d\n", start.tv_sec);
	printf("start.tv_usec:%d\n", start.tv_usec);

	int i = 100000000;
	while (i--) ;

	gettimeofday(&end, NULL);
	printf("end.tv_sec:%d\n", end.tv_sec);
	printf("end.tv_usec:%d\n", end.tv_usec);

	time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);	//微秒
	printf("time_use is %.10f usec\n", time_use);

	return 0;
}
