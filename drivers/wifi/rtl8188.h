#ifndef __wifi_drv_h__
#define __wifi_drv_h__

#include <asm/types.h>
#include "fw.h"

/* for all modules */
//#define DRV_NAME        ""
#define DRV_VERSION "v1.0"
#define DRV_AUTHOR      "Hao Chen <947481900@qq.com>"

struct rtl8188_priv {
	/* USB */
	struct usb_device *udev;
	struct usb_interface *intf;
	struct {
		struct firmware *fw;
		HAL_VERSION version_id;
			  /****** FW related ******/
		u32 firmware_size;
		u16 firmware_version;
		u16 FirmwareVersionRev;
		u16 firmware_sub_version;
		u16 FirmwareSignature;
		u8 RegFWOffload;
		u8 bFWReady;
		u8 bBTFWReady;
		u8 fw_ractrl;
		u8 LastHMEBoxNum;	/* H2C - for host message to fw */
	} fw;
};

#endif
