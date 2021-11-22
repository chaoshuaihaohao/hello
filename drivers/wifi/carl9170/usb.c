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

#define BIT(nr)                 (1 << (nr))
enum carl9170_device_features {                                                    
        CARL9170_WPS_BUTTON             = BIT(0),                                  
        CARL9170_ONE_LED                = BIT(1),                                  
};

#define CARL9170FW_MAGIC_SIZE                   4
struct carl9170fw_desc_head {
        u8      magic[CARL9170FW_MAGIC_SIZE];
        __le16 length;
        u8 min_ver;
        u8 cur_ver;
} __packed;

struct ar9170 {                                                                                                                                                                               
//       struct ath_common common;                                                                                                                                                              
       struct ieee80211_hw *hw;                                                                                                                                                               
       struct mutex mutex;                                                                                                                                                                    
//       enum carl9170_device_state state;                                                                                                                                                      
       spinlock_t state_lock;                                                                                                                                                                 
//       enum carl9170_restart_reasons last_reason;                                                                                                                                             
       bool registered;                                                                                                                                                                       
                                                                                                                                                                                              
       /* USB */                                                                                                                                                                              
       struct usb_device *udev;                                                                                                                                                               
       struct usb_interface *intf;                                                                                                                                                            
       struct usb_anchor rx_anch;                                                                                                                                                             
       struct usb_anchor rx_work;                                                                                                                                                             
       struct usb_anchor rx_pool;                                                                                                                                                             
       struct usb_anchor tx_wait;                                                                                                                                                             
       struct usb_anchor tx_anch;                                                                                                                                                             
       struct usb_anchor tx_cmd;                                                                                                                                                              
       struct usb_anchor tx_err;                                                                                                                                                              
       struct tasklet_struct usb_tasklet;                                                                                                                                                     
       atomic_t tx_cmd_urbs;                                                                                                                                                                  
       atomic_t tx_anch_urbs;                                                                                                                                                                 
       atomic_t rx_anch_urbs;                                                                                                                                                                 
       atomic_t rx_work_urbs;                                                                                                                                                                 
       atomic_t rx_pool_urbs;                                                                                                                                                                 
       kernel_ulong_t features;                                                                                                                                                               
                                                                                                                                                                                              
       /* firmware settings */                                                                                                                                                                
       struct completion fw_load_wait;                                                                                                                                                        
       struct completion fw_boot_wait;                                                                                                                                                        
       struct {                                                                                                                                                                               
               const struct carl9170fw_desc_head *desc;                                                                                                                                       
               const struct firmware *fw;                                                                                                                                                     
               unsigned int offset;                                                                                                                                                           
               unsigned int address;                                                                                                                                                          
               unsigned int cmd_bufs;                                                                                                                                                         
               unsigned int api_version;                                                                                                                                                      
               unsigned int vif_num;                                                                                                                                                          
               unsigned int err_counter;                                                                                                                                                      
               unsigned int bug_counter;                                                                                                                                                      
               u32 beacon_addr;                                                                                                                                                               
               unsigned int beacon_max_len;                                                                                                                                                   
               bool rx_stream;                                                                                                                                                                
               bool tx_stream;                                                                                                                                                                
               unsigned int mem_blocks;                                                                                                                                                       
               unsigned int mem_block_size;                                                                                                                                                   
               unsigned int rx_size;                                                                                                                                                          
       } fw;
};

static struct usb_device_id carl9170_usb_ids[] = {
	/* Netgear WN111 v2 */
	{ USB_DEVICE(0x0846, 0x9001), .driver_info = CARL9170_ONE_LED },
};

static int carl9170_usb_probe(struct usb_interface *intf,
			      const struct usb_device_id *id)
{
	return 0;
}

static void carl9170_usb_disconnect(struct usb_interface *intf)
{

}

#ifdef CONFIG_PM
static int carl9170_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	return 0;
}

static int carl9170_usb_resume(struct usb_interface *intf)
{
	return 0;
}
#endif

static struct usb_driver carl9170_driver = {
	.name = KBUILD_MODNAME,
	.probe = carl9170_usb_probe,
	.disconnect = carl9170_usb_disconnect,
	.id_table = carl9170_usb_ids,
	.soft_unbind = 1,
#ifdef CONFIG_PM
	.suspend = carl9170_usb_suspend,
	.resume = carl9170_usb_resume,
#endif /* CONFIG_PM */
//	.disable_hub_initiated_lpm = 1,
};

module_usb_driver(carl9170_driver);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("wifi driver");
MODULE_LICENSE("GPL v2");
