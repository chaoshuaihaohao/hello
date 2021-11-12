#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_ALERT */
#include <linux/init.h>		/*Needed for __init */
#include <linux/pci.h>
#include <linux/usb.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/firmware.h>

#include "rtl8188.h"
#define DRV_VERSION "v1.0"
#define R8188_FW_NAME       "9170-1.fw"
#define CONFIG_EMBEDDED_FWIMG
  #define FW_8710B_SIZE           0x8000
  #define FW_8710B_START_ADDRESS  0x1000
  #define FW_8710B_END_ADDRESS    0x1FFF /* 0x5FFF */

#define MAX_DLFW_PAGE_SIZE                      4096	/* @ page : 4k bytes */
typedef enum _firmware_source {
	fw_source_img_file = 0,
	fw_source_header_file = 1,	/* from header file */
} firmware_source, *pfirmware_source;

#define FW_8710B_SIZE         0x8000
typedef struct _rt_firmware {
	firmware_source efwsource;
#ifdef CONFIG_EMBEDDED_FWIMG
	unsigned int *szfwbuffer;
#else
	unsigned int szfwbuffer[FW_8710B_SIZE];
#endif
	unsigned long ulfwlength;
} rt_firmware_8710b, *prt_firmware_8710b;

struct rtl8188_priv {
	struct usb_device *udev;
	struct usb_interface *intf;
	struct {
		const struct firmware *fw;
		unsigned long firmware_size;
          unsigned short firmware_version;
          unsigned short     FirmwareVersionRev;
          unsigned short firmware_sub_version;
          unsigned short     FirmwareSignature;
          unsigned int      RegFWOffload;
          unsigned int      bFWReady;
          unsigned int      bBTFWReady;
          unsigned int      fw_ractrl;
          unsigned int      LastHMEBoxNum;  /* H2C - for host message to fw */

	} fw;

};

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

#if 0
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
#endif
	return 0;
}

//a -- 定义一个usb_driver结构体变量
//b -- 填充该设备的usb_driver结构体成员变量
//c -- 将该驱动注册到USB子系统
//若要让USB设备真正工作起来，需要对USB设备的4个层次（设备、配置、接口、端点）进行初始化。当然这四个层次并不是一定都要进行初始化，而是根据你的USB设备的功能进行选择的
//
static int usb_init(struct usb_interface *intf, const struct usb_device_id *id)
{
	int ret;

	usb_dvobj_init(intf);

	return 0;
}

static int net_init(void)
{
	struct net_device *ndev;
	/* net work */
//      ndev = alloc_etherdev(sizeof(struct net_device));
//      if (usb_priv == NULL)
//              return -ENOMEM;

//      ndev->wireless_handlers = (struct iw_handler_def *)&xxx_handlers_def;
//      register_netdev(ndev);

#if 0
	xxx_read_chip_info();

	//a.自身的配置及初始化
	xxx_chip_configure();

	xxx_hal_init();

//b.在proc和sys文件系统上建立与用户空间的交互接口
	xxx_drv_proc_init();

	xxx_ndev_notifier_register();
//c -- 自身功能的实现
//WIFI的网络及接入原理，如扫描等。同时由于WIFI在移动设备中，相对功耗比较大，因此，对于功耗、电源管理也会在驱动中体现。
//
#endif

//out_free_netdev:
//      free_netdev(ndev);
	return 0;
}

//static int __maybe_unused rtl8188_wifi_usb_suspend(struct device *dev)
static int rtl8188_suspend(struct usb_interface *intf, pm_message_t message)
{

	return 0;
}

//static int __maybe_unused rtl8188_wifi_usb_resume(struct device *dev)
static int rtl8188_resume(struct usb_interface *intf)
{

	return 0;
}

#if 0
static const struct dev_pm_ops rtl8188_wifi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rtl8188_wifi_usb_suspend,
				rtl8188_wifi_usb_resume)
};
#endif

static const struct usb_device_id rtl8188_table[] = {
	/*  */
	{ USB_DEVICE(0x0BDA, 0xB711) },
	/* terminate */
	{ }
};

#if 0
  int usb_write32(struct intf_hdl *pintfhdl, unsigned long addr, unsigned long val)
  {
          unsigned int request;
          unsigned int requesttype;
          unsigned short wvalue;
          unsigned short index;
          unsigned short len;
          unsigned long data;
          int ret;


          request = 0x05;
          requesttype = 0x00;/* write_out */
          index = 0;/* n/a */

          wvalue = (unsigned short)(addr & 0x0000ffff);
          len = 4;

  /* WLANON PAGE0_REG needs to add an offset 0x8000 */
  #if defined(CONFIG_RTL8710B)
          if(wvalue >= 0x0000 && wvalue < 0x0100)
                  wvalue |= 0x8000;
  #endif

          data = val;
          ret = usbctrl_vendorreq(pintfhdl, request, wvalue, index,
                                  &data, len, requesttype);


          return ret;

  }
#endif

static int rtl8188_parse_firmware(struct rtl8188_priv *priv)
{
	const struct firmware *fw = priv->fw.fw;
	prt_firmware_8710b      pfirmware = NULL;

	if (WARN_ON(!fw))
		return -EINVAL;

	pfirmware = (prt_firmware_8710b)kzalloc(sizeof(rt_firmware_8710b), GFP_KERNEL);
	if (!pfirmware)
		return -ENOMEM;

#ifdef CONFIG_WOWLAN
	pfirmware->szfwbuffer = array_mp_8710b_fw_wowlan;
	pfirmware->ulfwlength = array_length_mp_8710b_fw_wowlan;
#else
	pfirmware->szfwbuffer = array_mp_8710b_fw_nic;
	pfirmware->ulfwlength =	array_length_mp_8710b_fw_nic;
#endif

	return 0;
}

 static void rtl8188_release_firmware(struct rtl8188_priv *priv)
  {
          if (priv->fw.fw) {
                  release_firmware(priv->fw.fw);
                  memset(priv->fw.fw, 0, sizeof(struct firmware));
          }
  }

static int rtl8188_usb_firmware_finish(struct rtl8188_priv *priv)
{
	struct usb_interface *intf = priv->intf;
	int ret;

	ret = rtl8188_parse_firmware(priv);
	if (ret)
		goto err_freefw;

err_freefw:
	rtl8188_release_firmware(priv);

	return 0;

}

static void r8188_fw_callback(const struct firmware *fw, void *context)
{
	struct rtl8188_priv *priv = context;

	if (fw) {
		priv->fw.fw = fw;
		rtl8188_usb_firmware_finish(priv);
	}

	printk(KERN_ALERT "%s\n", __func__);
}

static void rtl8188_disconnect(struct usb_interface *intf)
{
	struct rtl8188_priv *priv = usb_get_intfdata(intf);

//      wait_for_completion(&priv->fw_load_wait);

	usb_set_intfdata(intf, NULL);
	kfree(priv);

}

//static int rtl8188_wifi_usb_probe(struct pci_dev *pdev, const struct pci_device_id *id)
static int rtl8188_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct usb_device *udev;
	struct rtl8188_priv *priv;
	int ret;

//      usb_init(intf, id);

	udev = interface_to_usbdev(intf);
	dev_info(&udev->dev, "probe rtl8188_wifi usb driver " DRV_VERSION "\n");
	ret = usb_reset_device(udev);
	if (ret) {
		dev_err(&udev->dev, "reset usb device failed: ret\n", ret);
		return ret;
	}

	/* private data */
	priv = kmalloc(sizeof(struct rtl8188_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->udev = udev;
	priv->intf = intf;

	usb_set_intfdata(intf, priv);

#if 0
	tasklet_setup(&usb_priv->usb_tasklet, rtl8188_wifi_usb_tasklet);
#endif

//      ret = net_init();

	request_firmware_nowait(THIS_MODULE, 1, R8188_FW_NAME, &priv->udev->dev,
				GFP_KERNEL, priv, r8188_fw_callback);
__end:

	return 0;
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

static const struct net_device_ops rtl8188_netdev_ops = {
#if 0
	.ndo_init = rtl8188_ndev_init,
	.ndo_uninit = rtl8188_ndev_uninit,
	.ndo_open = netdev_open,
	.ndo_stop = netdev_close,
	.ndo_start_xmit = rtl8188_xmit_entry,
	.ndo_set_mac_address = rtl8188_net_set_mac_address,
	.ndo_get_stats = rtl8188_net_get_stats,
	.ndo_do_ioctl = rtl8188_ioctl,
#endif
};

#if 0
#ifdef CONFIG_WIRELESS_EXT
struct iw_handler_def rtw_handlers_def = {
	.standard = rtw_handlers,
	.num_standard = sizeof(rtw_handlers) / sizeof(iw_handler),
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)) || defined(CONFIG_WEXT_PRIV)
	.private = rtw_private_handler,
	.private_args = (struct iw_priv_args *)rtw_private_args,
	.num_private = sizeof(rtw_private_handler) / sizeof(iw_handler),
	.num_private_args =
	    sizeof(rtw_private_args) / sizeof(struct iw_priv_args),
#endif
#if WIRELESS_EXT >= 17
	.get_wireless_stats = rtw_get_wireless_stats,
#endif
};
#endif
#endif

module_usb_driver(rtl8188_usb_wifi_driver);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("wifi driver");
MODULE_LICENSE("GPL v2");
