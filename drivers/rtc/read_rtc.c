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

static void rtc_time_to_tm(const struct rtc_time rt, struct tm *tm)
{
          tm->tm_sec = rt.tm_sec;                                                                                                                                   
          tm->tm_min = rt.tm_min;                                                                                                                                   
          tm->tm_hour = rt.tm_hour;                                                                                                                                 
          tm->tm_mday = rt.tm_mday;                                                                                                                                 
          tm->tm_mon = rt.tm_mon;                                                                                                                                   
          tm->tm_year = rt.tm_year;                                                                                                                                 
          tm->tm_isdst = -1;  /* assume the system knows better than the RTC */
}

static time_t rtc_time_to_ctime(const struct rtc_time rt)
{
	struct tm tm;

	rtc_time_to_tm(rt, &tm);

	return mktime(&tm);
}

static char *tm_to_string(const struct tm tm)
{
	time_t tmp;
	tmp = mktime(&tm);
	return ctime(&tmp);
}

/* RTC_RD_TIME etc have this definition since 1.99.9 (pre2.0-9) */
#ifndef RTC_RD_TIME
//# define RTC_RD_TIME    _IOR('p', 0x09, struct rtc_time)
//# define RTC_SET_TIME   _IOW('p', 0x0a, struct linux_rtc_time)
//# define RTC_UIE_ON     _IO('p', 0x03)  /* Update int. enable on */
//# define RTC_UIE_OFF    _IO('p', 0x04)  /* Update int. enable off */
#endif

static int setup_alarm(int fd, time_t * wakeup)
{
	struct tm *tm;
	struct rtc_wkalrm wake = { 0 };

	/* The wakeup time is in POSIX time (more or less UTC).  Ideally                                                                                                
	 * RTCs use that same time; but PCs can't do that if they need to                                                                                               
	 * boot MS-Windows.  Messy...                                                                                                                                   
	 *                                                                                                                                                              
	 * When clock_mode == CM_UTC this process's timezone is UTC, so                                                                                                 
	 * we'll pass a UTC date to the RTC.                                                                                                                            
	 *                                                                                                                                                              
	 * Else clock_mode == CM_LOCAL so the time given to the RTC will                                                                                                
	 * instead use the local time zone. */
	tm = localtime(wakeup);
	wake.time.tm_sec = tm->tm_sec;
	wake.time.tm_min = tm->tm_min;
	wake.time.tm_hour = tm->tm_hour;
	wake.time.tm_mday = tm->tm_mday;
	wake.time.tm_mon = tm->tm_mon;
	wake.time.tm_year = tm->tm_year;
	/* wday, yday, and isdst fields are unused */
	wake.time.tm_wday = -1;
	wake.time.tm_yday = -1;
	wake.time.tm_isdst = -1;
	wake.enabled = 1;

	if (ioctl(fd, RTC_WKALM_SET, &wake) < 0) {
		printf("set rtc wake alarm failed\n");
		return -1;
	}
	return 0;
}

#define FAIL(x) printf("\033[47;41mFailed\033[0m %s: %d\n", __func__, x)
#define SUCCESS(x) printf("\033[47;42mSuccess\033[0m %s\n", __func__)

static int test_rtc_aie_on(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_AIE_ON);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_aie_off(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_AIE_OFF);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_uie_on(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_UIE_ON);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_uie_off(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_UIE_OFF);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_pie_on(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_PIE_ON);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_pie_off(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_PIE_OFF);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_wie_on(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_WIE_ON);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_wie_off(int fd)
{
	int ret;

	ret = ioctl(fd, RTC_WIE_OFF);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_alm_read(int fd)
{
	struct rtc_time time;
	struct tm t;
	time_t alarm;
	int ret;

	ret = ioctl(fd, RTC_ALM_READ, &time);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	alarm = rtc_time_to_ctime(time);
	printf("alarm: on  %s", ctime(&alarm));

	return 0;
}

static int test_rtc_alm_set(int fd)
{
	struct rtc_time time;
	time_t alarm;
	int ret;

	ret = ioctl(fd, RTC_ALM_READ, &time);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();

	time.tm_year += 52;
	alarm = rtc_time_to_ctime(time);
	printf("alarm will set: on  %s", ctime(&alarm));

	ret = ioctl(fd, RTC_ALM_SET, &time);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();

	ret = ioctl(fd, RTC_ALM_READ, &time);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	alarm = rtc_time_to_ctime(time);
	printf("alarm set: on  %s", ctime(&alarm));
	return 0;
}

static int test_rtc_read_time(int fd)
{
	struct tm nowtime;
	int ret;

	ret = ioctl(fd, RTC_RD_TIME, &nowtime);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	printf("%d/%d/%d %d:%d %02d\n", nowtime.tm_year + 1900,
	       nowtime.tm_mon, nowtime.tm_mday, nowtime.tm_hour,
	       nowtime.tm_min, nowtime.tm_sec);
	return 0;
}

static int test_rtc_set_time(int fd)
{
	struct tm nowtime;
	int ret;

	ret = ioctl(fd, RTC_RD_TIME, &nowtime);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();

	printf("rd time: %s", tm_to_string(nowtime));

	/* Set the rtc time */
	//nowtime.tm_year = 21;
	ret = ioctl(fd, RTC_SET_TIME, &nowtime);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	
	return 0;
}

static int test_rtc_irqp_read(int fd)
{
	unsigned long rate;
	int ret;

	ret = ioctl(fd, RTC_IRQP_READ, &rate);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_irqp_set(int fd)
{
	unsigned long rate;
	int ret;

	ret = ioctl(fd, RTC_IRQP_READ, &rate);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	printf("irqp rate = %d\n", rate);

	ret = ioctl(fd, RTC_IRQP_SET, &rate);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_epoch_read(int fd)
{
	unsigned long epoch;
	int ret;

	ret = ioctl(fd, RTC_EPOCH_READ, &epoch);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_epoch_set(int fd)
{
	unsigned long epoch;
	int ret;

	ret = ioctl(fd, RTC_EPOCH_READ, &epoch);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	printf("epoch = %d\n", epoch);

	ret = ioctl(fd, RTC_EPOCH_SET, &epoch);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_wkalm_read(int fd)
{
	struct rtc_wkalrm wake;
	int ret;

	ret = ioctl(fd, RTC_WKALM_RD, &wake);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_wkalm_set(int fd)
{
	struct rtc_wkalrm wake;
	int ret;

	ret = ioctl(fd, RTC_WKALM_RD, &wake);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
//	printf("epoch = %d\n", epoch);

	ret = ioctl(fd, RTC_WKALM_SET, &wake);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_pll_info_get(int fd)
{
	struct rtc_pll_info pinfo;
	int ret;

	ret = ioctl(fd, RTC_PLL_GET, &pinfo);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}

static int test_rtc_pll_info_set(int fd)
{
	struct rtc_pll_info pinfo;
	int ret;

	ret = ioctl(fd, RTC_PLL_GET, &pinfo);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
//	printf("epoch = %d\n", epoch);

	ret = ioctl(fd, RTC_PLL_SET, &pinfo);
	if (ret) {
		FAIL(ret);
		return ret;
	} else
		SUCCESS();
	return 0;
}
int main()
{
	char *dev = "/dev/rtc1";
	struct tm nowtime;
	int ret;

	int fd = open(dev, O_RDWR, 0664);
	if (fd < 0)
		printf("Error: open dev failed\n");

	test_rtc_aie_on(fd);
	test_rtc_aie_off(fd);
	test_rtc_uie_on(fd);
	test_rtc_uie_off(fd);
	test_rtc_pie_on(fd);
	test_rtc_pie_off(fd);
#if 0
	//只有一个驱动用到了wie
	test_rtc_wie_on(fd);
	test_rtc_wie_off(fd);
#endif
	test_rtc_alm_read(fd);
	test_rtc_alm_set(fd);

	test_rtc_read_time(fd);
	test_rtc_set_time(fd);

	test_rtc_irqp_read(fd);
//	test_rtc_irqp_set(fd); //error

//	test_rtc_epoch_read(fd);
//	test_rtc_epoch_set(fd);
	
	test_rtc_wkalm_read(fd);
	test_rtc_wkalm_set(fd);

//	test_rtc_pll_info_get(fd);
//	test_rtc_pll_info_set(fd);
#if 0
	struct rtc_wkalrm wake;
	time_t alarm;

	ret = ioctl(fd, RTC_ALM_READ, &wake);
	if (ret)
		printf("Error: rtc alarm time read failed, %d\n", ret);
	
	//alarm = mktime(&wake.time);
	//printf("alarm: on  %s", ctime(&alarm));

#endif

#if 1
	time_t now;
	now = alarm + 100000000;
	time(&now);
	setup_alarm(fd, &now);
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

	close(fd);
	return 0;
}
