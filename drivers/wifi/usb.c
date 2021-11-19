#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_ALERT */
#include <linux/init.h>		/*Needed for __init */
#include <linux/pci.h>
#include <linux/usb.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/firmware.h>
#include <net/mac80211.h>
#include <net/cfg80211.h>

#include "hal_com_reg.h"
#include "hal8710breg.h"
#include "rtl8710b_spec.h"
#include "rtl8188.h"
#include "osdep_service.h"

#define BIT(nr)                 (UL(1) << (nr))

#if 1

#if defined(CONFIG_VENDOR_REQ_RETRY) && defined(CONFIG_USB_VENDOR_REQ_MUTEX)
	  /* vendor req retry should be in the situation when each vendor req is atomically submitted from others */
#define MAX_USBCTRL_VENDORREQ_TIMES     10
#else
#define MAX_USBCTRL_VENDORREQ_TIMES     1
#endif

#define REALTEK_USB_VENQT_READ          0xC0
#define REALTEK_USB_VENQT_WRITE 0x40
#define REALTEK_USB_VENQT_CMD_REQ       0x05
#define REALTEK_USB_VENQT_CMD_IDX       0x00
#define REALTEK_USB_IN_INT_EP_IDX       1

#define RTW_USB_CONTROL_MSG_TIMEOUT     500	/* ms */

#define VENDOR_CMD_MAX_DATA_LEN 254
#define FW_START_ADDRESS        0x1000

#define ALIGNMENT_UNIT                          16

static int usbctrl_vendorreq(struct usb_device *udev, u8 request, u16 value,
			     u16 index, void *pdata, u16 len, u8 requesttype)
{
	int vendorreq_times = 0;
	unsigned int pipe;
	u8 reqtype;
	u8 *tmp_buf;
	u32 tmp_buflen = 0;
	u8 *pIo_buf;
	int ret = 0;

	tmp_buf = kzalloc((u32) len + ALIGNMENT_UNIT, GFP_KERNEL);
	tmp_buflen = (u32) len + ALIGNMENT_UNIT;

	/* Added by Albert 2010/02/09 */
	/* For mstar platform, mstar suggests the address for USB IO should be 16 bytes alignment. */
	/* Trying to fix it here. */
	pIo_buf =
	    (tmp_buf ==
	     NULL) ? NULL : tmp_buf + ALIGNMENT_UNIT -
	    ((size_t)(tmp_buf) & 0x0f);
	if (pIo_buf == NULL) {
		dev_err(&udev->dev, "[%s] pIo_buf == NULL\n", __FUNCTION__);
		ret = -ENOMEM;
		goto err_out;
	}

	//      while (++vendorreq_times <= MAX_USBCTRL_VENDORREQ_TIMES) {
	memset(pIo_buf, 0, len);

	if (requesttype == 0x01) {
		pipe = usb_rcvctrlpipe(udev, 0);	/* read_in */
		reqtype = REALTEK_USB_VENQT_READ;
	} else {
		pipe = usb_sndctrlpipe(udev, 0);	/* write_out */
		reqtype = REALTEK_USB_VENQT_WRITE;
		memcpy(pIo_buf, pdata, len);
	}

	/* @dev：指向要发送消息的 USB 设备的指针
	 * @pipe：发送消息到端点“管道”通过usb_sndctrlpipe(udev, 0)获取
	 *
	 * USB非标准命令，即VENDOR自定义的命令如：
	 * @request：USB 消息请求值
	 * @requesttype：USB 消息请求类型值
	 *
	 * @value：USB 消息值.2个字节，高字节是msg的类型
	 *      （1为输入，2为输出，3为特性）；低字节为msg的ID（预设为0）
	 * @index：USB 消息索引值.索引字段同样是2个字节，描述的是接口号
	 * @data：指向要发送的数据的指针
	 * @size：要发送的数据的长度（以字节为单位）
	 * @timeout：在计时之前等待消息完成的时间（以毫秒为单位）
	 *
	 * */
	ret =
	    usb_control_msg(udev, pipe, request, reqtype, value, index, pIo_buf,
			    len, RTW_USB_CONTROL_MSG_TIMEOUT);
	if (ret < 0) {
		kfree(pIo_buf);
		goto err_out;
	}

	if (ret == len) {	/* Success this control transfer. */
		if (requesttype == 0x01) {
			/* For Control read transfer, we have to copy the read data from buf to pdata. */
			memcpy(pdata, pIo_buf, len);
		}
	} else {		/* error cases */
		dev_info(&udev->dev,
			 "reg 0x%x, usb %s %u fail, ret:%d value=0x%x, vendorreq_times:%d\n",
			 value,
			 (requesttype == 0x01) ? "read" : "write", len * 8,
			 ret, *(u32 *) pdata, vendorreq_times);

		if (ret < 0) {
			dev_info(&udev->dev, "%s:ret < 0\n", __func__);
		} else {	/* ret != len && ret >= 0 */
			if (ret > 0) {
				dev_info(&udev->dev, "%s:ret > 0\n", __func__);
				if (requesttype == 0x01) {
/* For Control read transfer, we have to copy the read data from buf to
 * pdata. */
					memcpy(pdata, pIo_buf, len);
				}
			}
		}
	}

err_out:
	dev_err(&udev->dev, "firmware upload failed (%d).\n", ret);
	return ret;
}

static int usb_write(struct usb_device *udev, u32 addr, u8 val, u8 len)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u8 data;
	int ret;

	request = 0x05;
	requesttype = 0x00;	/* write_out */
	index = 0;		/* n/a */

	wvalue = (u16) (addr & 0x0000ffff);
	len = len / 8;

	/* WLANON PAGE0_REG needs to add an offset 0x8000 */
	if (wvalue >= 0x0000 && wvalue < 0x0100)
		wvalue |= 0x8000;

	data = val;
	ret = usbctrl_vendorreq(udev, request, wvalue, index,
				&data, len, requesttype);
	return ret;
}

#define usb_write8(udev, addr, val) usb_write(udev, addr, val, 8)
#define usb_write16(udev, addr, val) usb_write(udev, addr, val, 16)
#define usb_write32(udev, addr, val) usb_write(udev, addr, val, 32)

u8 usb_read(struct usb_device *udev, u32 addr, u8 len)
{
	u8 request;
	u8 requesttype;
	u16 wvalue;
	u16 index;
	u8 data = 0;

	request = 0x05;
	requesttype = 0x01;	/* read_in */
	index = 0;		/* n/a */

	wvalue = (u16) (addr & 0x0000ffff);
	len = len / 8;

	/* WLANON PAGE0_REG needs to add an offset 0x8000 */
	if (wvalue >= 0x0000 && wvalue < 0x0100)
		wvalue |= 0x8000;

	usbctrl_vendorreq(udev, request, wvalue, index,
			  &data, len, requesttype);

	return data;
}

#define usb_read8(udev, addr) usb_read(udev, addr, 8)
#define usb_read16(udev, addr) usb_read(udev, addr, 16)
#define usb_read32(udev, addr) usb_read(udev, addr, 32)

#endif

#if 0
//static struct dvobj_priv *usb_dvobj_init(struct usb_interface *intf)
static int *usb_dvobj_init(struct usb_interface *intf)
{
	int i;
	unsigned int val8;
	int status = 0;
	struct dvobj_priv *pdvobjpriv;

	//设备
	struct usb_device *pusbd;
	struct usb_device_descriptor *pdev_desc;

	//配置
	struct usb_host_config *phost_conf;
	struct usb_config_descriptor *pconf_desc;

	//接口
	struct usb_host_interface *phost_iface;
	struct usb_interface_descriptor *piface_desc;

	//端点
	struct usb_host_endpoint *phost_endp;
	struct usb_endpoint_descriptor *pendp_desc;

	//设备的初始化
	pdvobjpriv->pusbintf = intf;
	pusbd = pdvobjpriv->pusbdev = interface_to_usbdev(intf);
	usb_set_intfdata(intf, pdvobjpriv);
	pdev_desc = &pusbd->descriptor;

	//配置的初始化
	phost_conf = pusbd->actconfig;
	pconf_desc = &phost_conf->desc;

	//接口的初始化
	phost_iface = &intf->altsetting[0];
	piface_desc = &phost_iface->desc;

	//端点的初始化，由于wifi模块属于网络设备，传输批量数据，因此需要初始化为批量端点，端点方向（输入、输出）等。同时，由于wifi驱动功能比较多，需要初始化几个输入输出端点。
	for (i = 0; i < pdvobjpriv->nr_endpoint; i++) {
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp) {
			pendp_desc = &phost_endp->desc;

			//检查是否为输入端点
			usb_endpoint_is_bulk_in(pendp_desc);

			//检查是否为输出端点
			usb_endpoint_is_bulk_out(pendp_desc);
		}

	}

	usb_get_dev(pusbd);
	return 0;
}

//a -- 定义一个usb_driver结构体变量
//b -- 填充该设备的usb_driver结构体成员变量
//c -- 将该驱动注册到USB子系统
//若要让USB设备真正工作起来，需要对USB设备的4个层次（设备、配置、接口、端点）进行初始化。当然这四个层次并不是一定都要进行初始化，而是根据你的USB设备的功能进行选择的
//
#endif

static int rtl8188_suspend(struct usb_interface *intf, pm_message_t message)
{
	return 0;
}

static int rtl8188_resume(struct usb_interface *intf)
{
	return 0;
}

static const struct usb_device_id rtl8188_table[] = {
	/*  */
	{ USB_DEVICE(0x0BDA, 0xB711) },
	/* terminate */
	{ }
};

static void fw_download_enable(struct usb_device *udev, bool enable)
{
	u8 tmp;

	if (enable) {
		// MCU firmware download enable.
		usb_write8(udev, REG_8051FW_CTRL_V1_8710B | PAGE0_OFFSET, 0x05);
		tmp =
		    usb_read8(udev,
			      (REG_8051FW_CTRL_V1_8710B + 3) | PAGE0_OFFSET);
		tmp |= BIT0;
		usb_write8(udev, (REG_8051FW_CTRL_V1_8710B + 3) | PAGE0_OFFSET,
			   tmp);

		// Clear Rom DL enable
		tmp =
		    usb_read8(udev,
			      (REG_8051FW_CTRL_V1_8710B + 2) | PAGE0_OFFSET);
		usb_write8(udev, (REG_8051FW_CTRL_V1_8710B + 2) | PAGE0_OFFSET,
			   tmp & 0xf7);
	} else {
		// MCU firmware download enable.
		tmp = usb_read8(udev, REG_8051FW_CTRL_V1_8710B | PAGE0_OFFSET);
		usb_write8(udev, REG_8051FW_CTRL_V1_8710B | PAGE0_OFFSET,
			   tmp & 0xfe);
	}
}

static int _BlockWrite(struct usb_device *udev, void *buffer, u32 buffSize)
{
	int ret = 0;
	/* (Default) Phase #1 : PCI muse use 4-byte write to download FW */
	u32 blockSize_p1 = 4;
	/* Phase #2 : Use 8-byte, if Phase#1 use big size to write FW. */
	u32 blockSize_p2 = 8;
	/* Phase #3 : Use 1-byte, the remnant of FW image. */
	u32 blockSize_p3 = 1;
	u32 blockCount_p1 = 0, blockCount_p2 = 0, blockCount_p3 = 0;
	u32 remainSize_p1 = 0, remainSize_p2 = 0;
	u8 *bufferPtr = (u8 *) buffer;
	u32 i = 0, offset = 0;
	/* 3 Phase #1 */
	blockCount_p1 = buffSize / blockSize_p1;
	remainSize_p1 = buffSize % blockSize_p1;

	for (i = 0; i < blockCount_p1; i++) {
		ret =
		    usb_write32(udev,
				(FW_8710B_START_ADDRESS + i * blockSize_p1),
				le32_to_cpu(*
					    ((u32 *) (bufferPtr +
						      i * blockSize_p1))));
		if (ret <= 0) {
			dev_err(&udev->dev, "====>%s %d i:%d\n", __func__,
				__LINE__, i);
			goto exit;
		}
	}

	/* 3 Phase #2 */
	if (remainSize_p1) {
		offset = blockCount_p1 * blockSize_p1;

		blockCount_p2 = remainSize_p1 / blockSize_p2;
		remainSize_p2 = remainSize_p1 % blockSize_p2;
	}

	/* 3 Phase #3 */
	if (remainSize_p2) {
		offset =
		    (blockCount_p1 * blockSize_p1) +
		    (blockCount_p2 * blockSize_p2);
		blockCount_p3 = remainSize_p2 / blockSize_p3;

		for (i = 0; i < blockCount_p3; i++) {
			ret =
			    usb_write8(udev,
				       (FW_8710B_START_ADDRESS + offset + i),
				       *(bufferPtr + offset + i));

			if (ret <= 0) {
				dev_err(&udev->dev, "====>%s %d i:%d\n",
					__func__, __LINE__, i);
				goto exit;
			}
		}
	}
exit:
	return ret;
}

static int _PageWrite(struct usb_device *udev, u32 page, void *buffer, u32 size)
{
	u8 value8;
	u8 u8Page = (u8) (page & 0x07);

	value8 =
	    (usb_read8(udev, REG_8051FW_CTRL_V1_8710B + 2) & 0xF8) | u8Page;
	usb_write8(udev, REG_8051FW_CTRL_V1_8710B + 2, value8);

	return _BlockWrite(udev, buffer, size);
}

static int _WriteFW(struct usb_device *udev, void *buffer, u32 size)
{
	/* Since we need dynamic decide method of dwonload fw, so we call this function to get chip version. */
	int ret = 0;
	u32 pageNums, remainSize;
	u32 page, offset;
	u8 *bufferPtr = (u8 *) buffer;

	pageNums = size / MAX_DLFW_PAGE_SIZE;
	/* RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4\n")); */
	remainSize = size % MAX_DLFW_PAGE_SIZE;

	for (page = 0; page < pageNums; page++) {
		offset = page * MAX_DLFW_PAGE_SIZE;
		ret =
		    _PageWrite(udev, page, bufferPtr + offset,
			       MAX_DLFW_PAGE_SIZE);
		if (ret <= 0) {
			dev_err(&udev->dev, "====>%s %d\n", __func__, __LINE__);
			goto exit;
		}
	}
	if (remainSize) {
		offset = pageNums * MAX_DLFW_PAGE_SIZE;
		page = pageNums;
		ret = _PageWrite(udev, page, bufferPtr + offset, remainSize);
		if (ret <= 0) {
			dev_err(&udev->dev, "====>%s %d\n", __func__, __LINE__);
			goto exit;
		}
	}

exit:
	return ret;
}

static int rtl8188_fw_info(struct rtl8188_priv *priv)
{
	PRT_8710B_FIRMWARE_HDR pFwHdr = priv->fw.pFwHdr;
	PRT_FIRMWARE_8710B pFirmware = NULL;
	u32 FirmwareLen;
	u8 *pFirmwareBuf;

	pFirmware =
	    (PRT_FIRMWARE_8710B) kzalloc(sizeof(RT_FIRMWARE_8710B), GFP_KERNEL);
	if (!pFirmware)
		return -ENOMEM;

#ifdef CONFIG_WOWLAN
	pFirmware->szFwBuffer = array_mp_8710b_fw_wowlan;
	pFirmware->ulFwLength = array_length_mp_8710b_fw_wowlan;
#else
	pFirmware->szFwBuffer = array_mp_8710b_fw_nic;
	pFirmware->ulFwLength = array_length_mp_8710b_fw_nic;
#endif

	if ((pFirmware->ulFwLength - 32) > FW_8710B_SIZE) {
		dev_err(&priv->udev->dev, "Firmware size:%u exceed %u\n",
			pFirmware->ulFwLength, FW_8710B_SIZE);
		return -EINVAL;
	}

	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;
	/* To Check Fw header. Added by tynli. 2009.12.04. */
	pFwHdr = (PRT_8710B_FIRMWARE_HDR) pFirmwareBuf;

	priv->fw.pFwHdr = pFwHdr;
	priv->fw.firmware_version = le16_to_cpu(pFwHdr->Version);
	priv->fw.firmware_sub_version = le16_to_cpu(pFwHdr->Subversion);
	priv->fw.FirmwareSignature = le16_to_cpu(pFwHdr->Signature);
	dev_info(&priv->udev->dev,
		 "firmware API: fw_ver=%x fw_subver=%04x sig=0x%x, Month=%02x, Date=%02x, Hour=%02x, Minute=%02x, FirmwareLen=%u\n",
		 priv->fw.firmware_version,
		 priv->fw.firmware_sub_version,
		 priv->fw.FirmwareSignature,
		 pFwHdr->Month, pFwHdr->Date, pFwHdr->Hour, pFwHdr->Minute,
		 pFirmware->ulFwLength);

	if ((priv->fw.version_id.VendorType == CHIP_VENDOR_UMC
	     && priv->fw.FirmwareSignature == 0x10B1) ||
	    (priv->fw.version_id.VendorType == CHIP_VENDOR_SMIC
	     && priv->fw.FirmwareSignature == 0x10B1)) {
		dev_info(&priv->udev->dev, "%s: Shift for fw header!\n",
			 __FUNCTION__);
		/* Shift 32 bytes for FW header */
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen - 32;
	} else {
		dev_warn(&priv->udev->dev,
			 "%s: Error! No shift for fw header! %04x\n",
			 __FUNCTION__, priv->fw.FirmwareSignature);
	}
	priv->fw.pFirmwareBuf = pFirmwareBuf;
	priv->fw.FirmwareLen = FirmwareLen;
	return 0;
}

static void fw_download(struct rtl8188_priv *priv)
{
	struct usb_device *udev = priv->udev;
	u8 write_fw = 0;
	int rtStatus;
	u8 value8;
	/* We must set 0x90[8]=1 before download FW. Recommented by Ben, 20171221 */
	value8 = usb_read8(udev, REG_8051FW_CTRL_V1_8710B + 1);
	value8 |= BIT(0);
	usb_write8(udev, REG_8051FW_CTRL_V1_8710B + 1, value8);

#define DL_FW_MAX 5
	while (write_fw++ < DL_FW_MAX) {
		/* reset FWDL chksum */
		usb_write8(udev, REG_8051FW_CTRL_V1_8710B,
			   usb_read8(udev, REG_8051FW_CTRL_V1_8710B) |
			   FWDL_ChkSum_rpt);

		/* start dl firmware at here */
		rtStatus =
		    _WriteFW(udev, priv->fw.pFirmwareBuf, priv->fw.FirmwareLen);
		if (rtStatus <= 0)
			continue;
	}
}

static void rtl8188_parse_firmware(struct rtl8188_priv *priv)
{
	struct usb_device *udev = priv->udev;
	int ret;

	ret = rtl8188_fw_info(priv);
	if (ret) {
		dev_err(&udev->dev, "FW info parse failed\n");
		return;
	}

	/* To check if FW already exists before download FW */
	if (usb_read8(udev, REG_8051FW_CTRL_V1_8710B | PAGE0_OFFSET) &
	    RAM_DL_SEL) {
		dev_info(&udev->dev,
			 "%s: FW exists before download FW\n", __func__);
		usb_write8(udev, REG_8051FW_CTRL_V1_8710B | PAGE0_OFFSET, 0x00);
	} else
		dev_info(&udev->dev,
			 "%s: FW not exists before download FW\n", __func__);

	fw_download_enable(udev, true);
	fw_download(priv);
	fw_download_enable(udev, false);
}

static struct usb_driver rtl8188_usb_wifi_driver;

static int test_rtl8188_fw_info(struct rtl8188_priv *priv)
{
	return 0;
}

static void rtl8188_fw_callback(const struct firmware *fw, void *context)
{
	struct rtl8188_priv *priv = context;

	if (!fw) {
		dev_info(&priv->udev->dev, "Load firmware by callback\n");
		rtl8188_parse_firmware(priv);
	} else {
		dev_info(&priv->udev->dev, "Load firmware by /lib/firmware\n");
		dev_info(&priv->udev->dev, "fw size = %lu\n", fw->size);
		priv->fw.fw = fw;

		PRT_8710B_FIRMWARE_HDR pFwHdr = priv->fw.pFwHdr;
		if (fw->size > FW_8710B_SIZE) {
			dev_err(&priv->udev->dev,
				"Firmware size:%u exceed %lu\n", fw->size,
				FW_8710B_SIZE);
			return -EINVAL;
		}

		/* header size is 32 bytes */
		/* To Check Fw header. Added by tynli. 2009.12.04. */
		pFwHdr = (PRT_8710B_FIRMWARE_HDR) fw->data;

		priv->fw.pFwHdr = pFwHdr;
		priv->fw.firmware_version = le16_to_cpu(pFwHdr->Version);
		priv->fw.firmware_sub_version = le16_to_cpu(pFwHdr->Subversion);
		priv->fw.FirmwareSignature = le16_to_cpu(pFwHdr->Signature);
		dev_info(&priv->udev->dev,
			 "firmware API: fw_ver=%x fw_subver=%04x sig=0x%x, Month=%02x, Date=%02x, Hour=%02x, Minute=%02x, FirmwareLen=%lu\n",
			 priv->fw.firmware_version,
			 priv->fw.firmware_sub_version,
			 priv->fw.FirmwareSignature,
			 pFwHdr->Month, pFwHdr->Date, pFwHdr->Hour,
			 pFwHdr->Minute, fw->size);
		priv->fw.fw->data += 32;
		priv->fw.fw->size -= 32;

		rtl8188_fw_set_if(priv);
	}
}

static int rtl8188_op_start(struct ieee80211_hw *hw)
{
	return 0;
}

static void rtl8188_op_stop(struct ieee80211_hw *hw)
{
}

static void rtl8188_op_flush(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif, u32 queues, bool drop)
{
}

static int rtl8188_op_add_interface(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif)
{
	return 0;
}

static void rtl8188_op_remove_interface(struct ieee80211_hw *hw,
					struct ieee80211_vif *vif)
{
}

static int rtl8188_op_config(struct ieee80211_hw *hw, u32 changed)
{
	return 0;
}

static void rtl8188_op_tx(struct ieee80211_hw *hw,
			  struct ieee80211_tx_control *control,
			  struct sk_buff *skb)
{
}

static void rtl8188_op_configure_filter(struct ieee80211_hw *hw,
					unsigned int changed_flags,
					unsigned int *new_flags, u64 multicast)
{
}

static const struct ieee80211_ops rtl8188_ops = {
	.tx = rtl8188_op_tx,
	.start = rtl8188_op_start,
	.stop = rtl8188_op_stop,
	.config = rtl8188_op_config,
	.add_interface = rtl8188_op_add_interface,
	.remove_interface = rtl8188_op_remove_interface,
	.configure_filter = rtl8188_op_configure_filter,
	.flush = rtl8188_op_flush,
};

static void rtl8188_preinit_wiphy(struct wiphy *wiphy)
{
#define RTW_SSID_SCAN_AMOUNT 9	/* for WEXT_CSCAN_AMOUNT 9 */
#define RTW_MAX_NUM_PMKIDS 4
#define NUM_ACL 16
#define RTW_MAX_REMAIN_ON_CHANNEL_DURATION 5000	/* ms */

	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

	wiphy->max_scan_ssids = RTW_SSID_SCAN_AMOUNT;
	wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;
	wiphy->max_num_pmkids = RTW_MAX_NUM_PMKIDS;

//      wiphy->max_acl_mac_addrs = NUM_ACL; //this can make wiphy_register call trace

//      wiphy->max_remain_on_channel_duration = RTW_MAX_REMAIN_ON_CHANNEL_DURATION;
	wiphy->interface_modes |= BIT(NL80211_IFTYPE_STATION)
	    | BIT(NL80211_IFTYPE_ADHOC)
#ifdef CONFIG_AP_MODE
	    | BIT(NL80211_IFTYPE_AP)
#ifdef CONFIG_WIFI_MONITOR
	    | BIT(NL80211_IFTYPE_MONITOR)
#endif
#endif
#if defined(CONFIG_P2P) || defined(COMPAT_KERNEL_RELEASE)
	    | BIT(NL80211_IFTYPE_P2P_CLIENT)
	    | BIT(NL80211_IFTYPE_P2P_GO)
#if defined(RTW_DEDICATED_P2P_DEVICE)
	    | BIT(NL80211_IFTYPE_P2P_DEVICE)
#endif
#endif
	    ;

#ifdef COMPAT_KERNEL_RELEASE
#ifdef CONFIG_AP_MODE
	wiphy->mgmt_stypes = rtw_cfg80211_default_mgmt_stypes;
#endif /* CONFIG_AP_MODE */
#endif

#ifdef CONFIG_WIFI_MONITOR
	wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);
#endif

//#if defined(RTW_SINGLE_WIPHY)
//      wiphy->iface_combinations = rtw_combinations;
//      wiphy->n_iface_combinations = ARRAY_SIZE(rtw_combinations);
//#endif

//      wiphy->cipher_suites = rtw_cipher_suites;
//      wiphy->n_cipher_suites = ARRAY_SIZE(rtw_cipher_suites);

//      if (IsSupported24G(adapter->registrypriv.wireless_mode))
//              wiphy->bands[NL80211_BAND_2GHZ] =
//                  rtw_spt_band_alloc(NL80211_BAND_2GHZ);

#ifdef CONFIG_IEEE80211_BAND_5GHZ
//      if (is_supported_5g(adapter->registrypriv.wireless_mode))
//              wiphy->bands[NL80211_BAND_5GHZ] =
//                  rtw_spt_band_alloc(NL80211_BAND_5GHZ);
#endif

	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	wiphy->flags |= WIPHY_FLAG_HAVE_AP_SME;
	/* remove WIPHY_FLAG_OFFCHAN_TX, because we not support this feature */
	/* wiphy->flags |= WIPHY_FLAG_OFFCHAN_TX | WIPHY_FLAG_HAVE_AP_SME; */

#ifdef CONFIG_PNO_SUPPORT
	wiphy->max_sched_scan_ssids = MAX_PNO_LIST_COUNT;
#endif
	wiphy->rfkill = NULL;

}

static void rtl8188_disconnect(struct usb_interface *intf)
{
	struct rtl8188_priv *priv = usb_get_intfdata(intf);

	if (WARN_ON(!priv))
		return;

	ieee80211_unregister_hw(priv->hw);

	usb_put_intf(priv->intf);
	usb_set_intfdata(intf, NULL);
	kfree(priv);
}

//static int rtl8188_wifi_usb_probe(struct pci_dev *pdev, const struct pci_device_id *id)
static int rtl8188_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct rtl8188_priv *priv;
	struct ieee80211_hw *hw;
	const struct firmware *fw_entry;
	int ret;

	dev_info(&udev->dev, "probe rtl8188_wifi usb driver " DRV_VERSION "\n");

	hw = ieee80211_alloc_hw(sizeof(*priv), &rtl8188_ops);
	if (!hw) {
		printk(KERN_ALERT "alloc hw failed\n");
		return -ENOMEM;
	}
	priv = hw->priv;
	priv->hw = hw;

	hw->queues = 4;
	/* firmware decides which modes we support */
//      rtl8188_preinit_wiphy(priv->hw->wiphy);
	hw->wiphy->rfkill = NULL;

	ieee80211_hw_set(hw, RX_INCLUDES_FCS);
	ieee80211_hw_set(hw, MFP_CAPABLE);
	ieee80211_hw_set(hw, REPORTS_TX_ACK_STATUS);
	ieee80211_hw_set(hw, SUPPORTS_PS);
	ieee80211_hw_set(hw, PS_NULLFUNC_STACK);
	ieee80211_hw_set(hw, NEED_DTIM_BEFORE_ASSOC);
	ieee80211_hw_set(hw, SUPPORTS_RC_TABLE);
	ieee80211_hw_set(hw, SIGNAL_DBM);
	ieee80211_hw_set(hw, SUPPORTS_HT_CCK_RATES);

	wiphy_ext_feature_set(hw->wiphy, NL80211_EXT_FEATURE_CQM_RSSI_LIST);

	priv->udev = udev;
	priv->intf = intf;

	usb_set_intfdata(intf, priv);
	SET_IEEE80211_DEV(priv->hw, &intf->dev);

#if 1
	request_firmware_nowait(THIS_MODULE, 1, RTL8188_FW_NAME,
				&priv->udev->dev, GFP_KERNEL, priv,
				rtl8188_fw_callback);
#else
	if (request_firmware(&fw_entry, RTL8188_FW_NAME, &udev->dev) == 0) {	/*从用户空间请求映像数据 */
		/*将固件映像拷贝到硬件的存储器，拷贝函数由用户编写 */
//              copy_fw_to_device(fw_entry->data, fw_entry->size);
		memcpy(priv->fw, fw_entry->data, fw_entry->size);
		release_firmware(fw_entry);
	}
#endif
	ret = ieee80211_register_hw(priv->hw);
	if (ret)
		goto err_priv;

	return ret;

//err_freefw:
//      ieee80211_unregister_hw(priv->hw);

err_priv:
	usb_put_intf(priv->intf);
	usb_set_intfdata(intf, NULL);
	kfree(priv);

	return ret;
}

static struct usb_driver rtl8188_usb_wifi_driver = {
	.name = KBUILD_MODNAME,
	.probe = rtl8188_probe,
	.disconnect = rtl8188_disconnect,
	.id_table = rtl8188_table,
	.soft_unbind = 1,
#ifdef CONFIG_PM
	.suspend = rtl8188_suspend,
	.resume = rtl8188_resume,
	.reset_resume = rtl8188_resume,
#endif /* CONFIG_PM */
	.disable_hub_initiated_lpm = 1,
};

module_usb_driver(rtl8188_usb_wifi_driver);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("wifi driver");
MODULE_LICENSE("GPL v2");
