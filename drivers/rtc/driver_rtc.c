#include <linux/bcd.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <asm/io.h>

struct foo_regs {
	int seconds;
	int minutes;
	int hours;
	int mday;
	int wday;
	int month;
	int years;
};

/**********************************************************************
 * register summary
 **********************************************************************/
#define RTC_SECONDS             0
#define RTC_SECONDS_ALARM       1
#define RTC_MINUTES             2
#define RTC_MINUTES_ALARM       3
#define RTC_HOURS               4
#define RTC_HOURS_ALARM         5
#define RTC_DAY_OF_WEEK         6
#define RTC_DAY_OF_MONTH        7
#define RTC_MONTH               8
#define RTC_YEAR                9
/* control registers - Moto names
 */
#define RTC_REG_A		10
#define RTC_REG_B		11
#define RTC_REG_C		12
#define RTC_REG_D		13
/* RTC_*_alarm is always true if 2 MSBs are set */
# define RTC_ALARM_DONT_CARE    0xC0


/**********************************************************************
 * register details
 **********************************************************************/
#define RTC_FREQ_SELECT	RTC_REG_A

/* update-in-progress  - set to "1" 244 microsecs before RTC goes off the bus,
 * reset after update (may take 1.984ms @ 32768Hz RefClock) is complete,
 * totalling to a max high interval of 2.228 ms.
 */
# define RTC_UIP		0x80
# define RTC_DIV_CTL		0x70
   /* divider control: refclock values 4.194 / 1.049 MHz / 32.768 kHz */
#  define RTC_REF_CLCK_4MHZ	0x00
#  define RTC_REF_CLCK_1MHZ	0x10
#  define RTC_REF_CLCK_32KHZ	0x20
   /* 2 values for divider stage reset, others for "testing purposes only" */
#  define RTC_DIV_RESET1	0x60
#  define RTC_DIV_RESET2	0x70
  /* Periodic intr. / Square wave rate select. 0=none, 1=32.8kHz,... 15=2Hz */
# define RTC_RATE_SELECT 	0x0F

/**********************************************************************/
#define RTC_CONTROL	RTC_REG_B
# define RTC_SET 0x80		/* disable updates for clock setting */
# define RTC_PIE 0x40		/* periodic interrupt enable */
# define RTC_AIE 0x20		/* alarm interrupt enable */
# define RTC_UIE 0x10		/* update-finished interrupt enable */
# define RTC_SQWE 0x08		/* enable square-wave output */
# define RTC_DM_BINARY 0x04	/* all time/date values are BCD if clear */
# define RTC_24H 0x02		/* 24 hour mode - else hours bit 7 means pm */
# define RTC_DST_EN 0x01	/* auto switch DST - works f. USA only */

/**********************************************************************/
#define RTC_INTR_FLAGS	RTC_REG_C
/* caution - cleared by read */
# define RTC_IRQF 0x80		/* any of the following 3 is active */
# define RTC_PF 0x40
# define RTC_AF 0x20
# define RTC_UF 0x10

/**********************************************************************/
#define RTC_VALID	RTC_REG_D
# define RTC_VRT 0x80		/* valid RAM and time */
/**********************************************************************/

#ifndef ARCH_RTC_LOCATION	/* Override by <asm/mc146818rtc.h>? */

#define RTC_IO_EXTENT	0x8
#define RTC_IO_EXTENT_USED	0x2
#define RTC_IOMAPPED	1	/* Default to I/O mapping. */

#else
#define RTC_IO_EXTENT_USED      RTC_IO_EXTENT
#endif /* ARCH_RTC_LOCATION */

#define RTC_PORT(x)     (0x70 + (x))

#define CMOS_READ(addr) ({ \
outb_p((addr),RTC_PORT(0)); \
inb_p(RTC_PORT(1)); \
})

#define CMOS_WRITE(val, addr) ({ \
outb_p((addr),RTC_PORT(0)); \
outb_p((val),RTC_PORT(1)); \
})

static int foo_device_read(struct device *dev, struct foo_regs *regs,
			   int offset, int size)
{
	DEFINE_SPINLOCK(rtc_lock);
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	regs->seconds = CMOS_READ(RTC_SECONDS);
	regs->minutes = CMOS_READ(RTC_MINUTES);
	regs->hours = CMOS_READ(RTC_HOURS);
	regs->mday = CMOS_READ(RTC_DAY_OF_MONTH);
	regs->wday = CMOS_READ(RTC_DAY_OF_WEEK);
	regs->month = CMOS_READ(RTC_MONTH);
	regs->years = CMOS_READ(RTC_YEAR);
#if 1
	printk("regs->seconds = %x\n", regs->seconds);
	printk("regs->minutes = %x\n", regs->minutes);
	printk("regs->hours = %x\n", regs->hours);
	printk("regs->mday = %x\n", regs->mday);
	printk("regs->wday = %x\n", regs->wday);
	printk("regs->month = %x\n", regs->month);
	printk("regs->years = %x\n", regs->years);
	printk("time: 20%x/%x/%x %x:%x:%x\n", regs->years, regs->month,
	       regs->mday, regs->hours, regs->minutes, regs->seconds);
#endif
	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

static int fake_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct foo_regs regs;
	int error;

	error = foo_device_read(dev, &regs, 0, sizeof(regs));
	if (error)
		return error;

	tm->tm_sec = bcd2bin(regs.seconds);
	tm->tm_min = bcd2bin(regs.minutes);
	tm->tm_hour = bcd2bin(regs.hours);
	tm->tm_mday = bcd2bin(regs.mday);
/*
* This device returns weekdays from 1 to 7
* But rtc_time.wday expect days from 0 to 6.
* So we need to subtract 1 to the value returned by the chip
*/
	tm->tm_wday = bcd2bin(regs.wday) - 1;
/*
* This device returns months from 1 to 12
* But rtc_time.tm_month expect a months 0 to 11.
* So we need to subtract 1 to the value returned by the chip
*/
	tm->tm_mon = bcd2bin(regs.month);
/*
* This device's Epoch is 2000.
* But rtc_time.tm_year expect years from Epoch 1900.
* So we need to add 100 to the value returned by the chip
*/
	tm->tm_year = bcd2bin(regs.years) + 100;
	return rtc_valid_tm(tm);
}

// test by `hwclock -w -f /dev/rtc1`
static int fake_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	printk("write date\n");
	return 0;
}

/* struct rtc_class_ops { */
/*         int (*ioctl)(struct device *, unsigned int, unsigned long); */
/*         int (*read_time)(struct device *, struct rtc_time *); */
/*         int (*set_time)(struct device *, struct rtc_time *); */
/*         int (*read_alarm)(struct device *, struct rtc_wkalrm *); */
/*         int (*set_alarm)(struct device *, struct rtc_wkalrm *); */
/*         int (*proc)(struct device *, struct seq_file *); */
/*         int (*alarm_irq_enable)(struct device *, unsigned int enabled); */
/*         int (*read_offset)(struct device *, long *offset); */
/*         int (*set_offset)(struct device *, long offset); */
/* }; */
static const struct rtc_class_ops fake_rtc_ops = {
	.read_time = fake_rtc_read_time,
	.set_time = fake_rtc_set_time,
//	.alarm_irq_enable = fake_rtc_alarm_irq_enable,
//	.read_alarm     = fake_rtc_read_alarm,
//	.set_alarm      = fake_rtc_set_alarm,
//	.proc           = fake_rtc_procfs,
};

static int fake_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;

	rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
				  &fake_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);
	printk(KERN_INFO "Fake RTC driver loaded\n");

	return 0;
}

static struct platform_driver fake_rtc_drv = {
	.probe = fake_rtc_probe,
	.driver = {
		   .name = "rtc-fake",
		   .owner = THIS_MODULE,
		    },
};

module_platform_driver_probe(fake_rtc_drv, fake_rtc_probe);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Fake RTC driver description");