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
#define RTC_REG_A		0x0A
#define RTC_REG_B		0x0B
#define RTC_REG_C		0x0C
#define RTC_REG_D		0x0D
/* RTC_*_alarm is always true if 2 MSBs are set */
#define RTC_ALARM_DONT_CARE    0xC0

/**********************************************************************
 * register details
 **********************************************************************/
#define RTC_FREQ_SELECT	RTC_REG_A

/* update-in-progress  - set to "1" 244 microsecs before RTC goes off the bus,
 * reset after update (may take 1.984ms @ 32768Hz RefClock) is complete,
 * totalling to a max high interval of 2.228 ms.
 */
#define RTC_UIP		0x80
#define RTC_DIV_CTL		0x70
   /* divider control: refclock values 4.194 / 1.049 MHz / 32.768 kHz */
#define RTC_REF_CLCK_4MHZ	0x00
#define RTC_REF_CLCK_1MHZ	0x10
#define RTC_REF_CLCK_32KHZ	0x20
   /* 2 values for divider stage reset, others for "testing purposes only" */
#define RTC_DIV_RESET1	0x60
#define RTC_DIV_RESET2	0x70
  /* Periodic intr. / Square wave rate select. 0=none, 1=32.8kHz,... 15=2Hz */
#define RTC_RATE_SELECT 	0x0F

/**********************************************************************/
#define RTC_CONTROL	RTC_REG_B
#define RTC_SET 0x80		/* disable updates for clock setting */
#define RTC_PIE 0x40		/* periodic interrupt enable */
#define RTC_AIE 0x20		/* alarm interrupt enable */
#define RTC_UIE 0x10		/* update-finished interrupt enable */
#define RTC_SQWE 0x08		/* enable square-wave output */
#define RTC_DM_BINARY 0x04	/* all time/date values are BCD if clear */
#define RTC_24H 0x02		/* 24 hour mode - else hours bit 7 means pm */
#define RTC_DST_EN 0x01		/* auto switch DST - works f. USA only */

/**********************************************************************/
#define RTC_INTR_FLAGS	RTC_REG_C
/* caution - cleared by read */
#define RTC_IRQF 0x80		/* any of the following 3 is active */
#define RTC_PF 0x40
#define RTC_AF 0x20
#define RTC_UF 0x10

/**********************************************************************/
#define RTC_VALID	RTC_REG_D
#define RTC_VRT 0x80		/* valid RAM and time */
/**********************************************************************/

#ifndef ARCH_RTC_LOCATION	/* Override by <asm/mc146818rtc.h>? */

#define RTC_IO_EXTENT	0x8
#define RTC_IO_EXTENT_USED	0x2
#define RTC_IOMAPPED	1	/* Default to I/O mapping. */

#else
#define RTC_IO_EXTENT_USED      RTC_IO_EXTENT
#endif /* ARCH_RTC_LOCATION */

#ifndef RTC_PORT
#define RTC_PORT(x)     (0x70 + (x))
#define RTC_ALWAYS_BCD  1	/* RTC operates in binary mode */
#endif

#define CMOS_READ(addr) ({ \
outb_p((addr),RTC_PORT(0)); \
inb_p(RTC_PORT(1)); \
})

#define CMOS_WRITE(val, addr) ({ \
outb_p((addr),RTC_PORT(0)); \
outb_p((val),RTC_PORT(1)); \
})

static DEFINE_SPINLOCK(rtc_lock);
static int foo_device_read(struct device *dev, struct foo_regs *regs,
			   int offset, int size)
{
	unsigned long flags;

	printk(KERN_ALERT "read date\n");
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
	printk(KERN_ALERT "aie enabled %s\n", (CMOS_READ(RTC_CONTROL) & RTC_AIE) ? "yes" : "no");
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

static int write_into_device(struct device *dev, struct foo_regs *regs,
			     int size)
{
	printk(KERN_ALERT "write info device\n");
	CMOS_WRITE(regs->seconds, RTC_SECONDS);
	CMOS_WRITE(regs->minutes, RTC_MINUTES);
	CMOS_WRITE(regs->hours, RTC_HOURS);
	CMOS_WRITE(regs->mday, RTC_DAY_OF_MONTH);
	CMOS_WRITE(regs->month, RTC_MONTH);
	CMOS_WRITE(regs->years, RTC_YEAR);
	return 0;
}

// test by `hwclock -w -f /dev/rtc1`
static int fake_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct foo_regs regs;

	printk(KERN_ALERT "write date\n");
	regs.seconds = bin2bcd(tm->tm_sec);
	regs.minutes = bin2bcd(tm->tm_min);
	regs.hours = bin2bcd(tm->tm_hour);

/*
 * * This device expects week days from 1 to 7
 * * But rtc_time.wday contains week days from 0 to 6.
 * * So we need to add 1 to the value given by rtc_time.wday
 * */
	regs.wday = bin2bcd(tm->tm_wday + 1);
	regs.mday = bin2bcd(tm->tm_mday);
/*
 * * This device expects months from 1 to 12
 * * But rtc_time.tm_mon contains months from 0 to 11.
 * * So we need to add 1 to the value given by rtc_time.tm_mon
 * */
	regs.month = bin2bcd(tm->tm_mon + 1);
/*
 * * This device expects year since Epoch 2000
 * * But rtc_time.tm_year contains year since Epoch 1900.
 * * We can just extract the year of the century with the
 * * rest of the division by 100.
 * */
	//regs.hours |= BQ32K_CENT;
	regs.years = bin2bcd(tm->tm_year % 100);

	printk("tm_year = %d\n", tm->tm_year);
	printk("tm_mon = %d\n", tm->tm_mon);
	printk("tm_mday = %d\n", tm->tm_mday);
	printk("tm_hour = %d\n", tm->tm_hour);
	printk("tm_min = %d\n", tm->tm_min);
	printk("tm_sec = %d\n", tm->tm_sec);
	printk("regs->years = %x\n", regs.years);
	printk("regs->month = %x\n", regs.month);
	printk("regs->mday = %x\n", regs.mday);
	printk("regs->hours = %x\n", regs.hours);
	printk("regs->minutes = %x\n", regs.minutes);
	printk("regs->seconds = %x\n", regs.seconds);
	return write_into_device(dev, &regs, sizeof(regs));
}

#if 0
  static int fake_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
  {
          unsigned long   flags;

          spin_lock_irqsave(&rtc_lock, flags);

	  if (enabled) {
		  rtc_control = CMOS_READ(RTC_CONTROL);
		  rtc_control |= RTC_AIE;
		  CMOS_WRITE(rtc_control, RTC_CONTROL);
		  if (device_can_wakeup(dev)) {
			  acpi_clear_event(ACPI_EVENT_RTC);
			  acpi_enable_event(ACPI_EVENT_RTC, 0);
		  }

	  }
                  cmos_irq_enable(cmos, RTC_AIE);
          else
                  cmos_irq_disable(cmos, RTC_AIE);

          spin_unlock_irqrestore(&rtc_lock, flags);
          return 0;
  }
#endif

static int fake_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *t)
{
	unsigned char rtc_control;

	printk(KERN_ALERT "%s\n", __func__);
	spin_lock_irq(&rtc_lock);
	t->time.tm_sec = CMOS_READ(RTC_SECONDS_ALARM);
	t->time.tm_min = CMOS_READ(RTC_MINUTES_ALARM);
	t->time.tm_hour = CMOS_READ(RTC_HOURS_ALARM);

	rtc_control = CMOS_READ(RTC_CONTROL);
	if (!(rtc_control & RTC_DM_BINARY) || RTC_ALWAYS_BCD) {
		if (((unsigned)t->time.tm_sec) < 0x60)
			t->time.tm_sec = bcd2bin(t->time.tm_sec);
		else
			t->time.tm_sec = -1;
		if (((unsigned)t->time.tm_min) < 0x60)
			t->time.tm_min = bcd2bin(t->time.tm_min);
		else
			t->time.tm_min = -1;
		if (((unsigned)t->time.tm_hour) < 0x24)
			t->time.tm_hour = bcd2bin(t->time.tm_hour);
		else
			t->time.tm_hour = -1;

		if (((unsigned)t->time.tm_mday) <= 0x31)
			t->time.tm_mday = bcd2bin(t->time.tm_mday);
		else
			t->time.tm_mday = -1;

		if (((unsigned)t->time.tm_mon) <= 0x12)
			t->time.tm_mon = bcd2bin(t->time.tm_mon) - 1;
		else
			t->time.tm_mon = -1;
	}

	t->enabled = !!(rtc_control & RTC_AIE);
	t->pending = 0;
	spin_unlock_irq(&rtc_lock);

	return 0;
}

static int fake_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *t)
{
	unsigned char mon, mday, hrs, min, sec, rtc_control;

	printk(KERN_ALERT "%s start\n", __func__);

	mon = t->time.tm_mon + 1;
	mday = t->time.tm_mday;
	hrs = t->time.tm_hour;
	min = t->time.tm_min;
	sec = t->time.tm_sec;

	spin_lock_irq(&rtc_lock);
	rtc_control = CMOS_READ(RTC_CONTROL);
	if (!(rtc_control & RTC_DM_BINARY) || RTC_ALWAYS_BCD) {
		/* Writing 0xff means "don't care" or "match all".  */
		mon = (mon <= 12) ? bin2bcd(mon) : 0xff;
		mday = (mday >= 1 && mday <= 31) ? bin2bcd(mday) : 0xff;
		hrs = (hrs < 24) ? bin2bcd(hrs) : 0xff;
		min = (min < 60) ? bin2bcd(min) : 0xff;
		sec = (sec < 60) ? bin2bcd(sec) : 0xff;
	}

	rtc_control &= ~RTC_AIE;
	CMOS_WRITE(rtc_control, RTC_CONTROL);

	/* update alarm */
	CMOS_WRITE(hrs, RTC_HOURS_ALARM);
	CMOS_WRITE(min, RTC_MINUTES_ALARM);
	CMOS_WRITE(sec, RTC_SECONDS_ALARM);

	if (t->enabled) {
		rtc_control = CMOS_READ(RTC_CONTROL);
		rtc_control |= RTC_AIE;
		CMOS_WRITE(rtc_control, RTC_CONTROL);
	}
	spin_unlock_irq(&rtc_lock);
	printk(KERN_ALERT "%s end\n", __func__);

	return 0;
}

static int fake_rtc_procfs(struct device *dev, struct seq_file *seq)
{
	struct rtc_device *rtc = dev_get_drvdata(dev);
	unsigned char rtc_control, valid;

	spin_lock_irq(&rtc_lock);
	rtc_control = CMOS_READ(RTC_CONTROL);
	valid = CMOS_READ(RTC_VALID);
	spin_unlock_irq(&rtc_lock);

	/* NOTE:  at least ICH6 reports battery status using a different                                                                                                
	 * (non-RTC) bit; and SQWE is ignored on many current systems.                                                                                                  
	 */
	seq_printf(seq,
		   "periodic_IRQ\t: %s\n"
		   "update_IRQ\t: %s\n"
		   "alarm_IRQ enabled\t: %s\n"
		   "HPET_emulated\t: %s\n"
		   // "square_wave\t: %s\n"                                                                                                                             
		   "BCD\t\t: %s\n"
		   "DST_enable\t: %s\n"
		   "periodic_freq\t: %d\n"
		   "batt_status\t: %s\n",
		   (rtc_control & RTC_PIE) ? "yes" : "no",
		   (rtc_control & RTC_UIE) ? "yes" : "no",
		   (rtc_control & RTC_AIE) ? "yes" : "no",
		   //use_hpet_alarm() ? "yes" : "no",                                                                                                                     
		   "no",
		   // (rtc_control & RTC_SQWE) ? "yes" : "no",                                                                                                          
		   (rtc_control & RTC_DM_BINARY) ? "no" : "yes",
		   (rtc_control & RTC_DST_EN) ? "yes" : "no",
		   rtc->irq_freq, (valid & RTC_VRT) ? "okay" : "dead");

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
//      .alarm_irq_enable = fake_rtc_alarm_irq_enable,
	.read_alarm = fake_rtc_read_alarm,
	.set_alarm = fake_rtc_set_alarm,
	.proc = fake_rtc_procfs,
};

static int fake_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	char name[12];

	rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
				       &fake_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);
	/* RTC always wakes from S1/S2/S3, and often S4/STD */
	device_init_wakeup(&pdev->dev, 1);

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
