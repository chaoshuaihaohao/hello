#ifndef __HEAD_H
#define __HEAD_H
#define CARL9170FW_NAME        "carl9170-1.fw"

enum ar9170_txq {
	AR9170_TXQ_BK = 0,	/* TXQ0 */
	AR9170_TXQ_BE,		/* TXQ1 */
	AR9170_TXQ_VI,		/* TXQ2 */
	AR9170_TXQ_VO,		/* TXQ3 */

	__AR9170_NUM_TXQ,
};
#define BIT(nr)                 (1 << (nr))
enum carl9170_device_features {
	CARL9170_WPS_BUTTON = BIT(0),
	CARL9170_ONE_LED = BIT(1),
};

#define CARL9170FW_MAGIC_SIZE                   4
#define CARL9170FW_API_MIN_VER          1
#define CARL9170FW_API_MAX_VER          1

#define AR9170_MAX_ACKTABLE_ENTRIES                     8
#define AR9170_MAX_VIRTUAL_MAC                          7

#define CARL9170FW_DESC_MAX_LENGTH                      8192

#define         AR9170_MAC_BCN_LENGTH_MAX               (512 - 32)

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
	bool usb_ep_cmd_is_bulk;

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
		bool rx_filter;
		bool hw_counters;
		unsigned int mem_blocks;
		unsigned int mem_block_size;
		unsigned int rx_size;
		unsigned int tx_seq_table;
		bool ba_filter;
		bool disable_offload_fw;
	} fw;

	/* interface configuration combinations */
	struct ieee80211_iface_limit if_comb_limits[1];
	struct ieee80211_iface_combination if_combs[1];
};
#define carl9170fw_for_each_hdr(desc, fw_desc)                          \
          for (desc = fw_desc;                                            \
               memcmp(desc->magic, LAST_MAGIC, CARL9170FW_MAGIC_SIZE) &&  \
               le16_to_cpu(desc->length) >= CARL9170FW_DESC_HEAD_SIZE &&  \
               le16_to_cpu(desc->length) < CARL9170FW_DESC_MAX_LENGTH;    \
               desc = (void *)((unsigned long)desc + le16_to_cpu(desc->length)))

#define CHECK_HDR_VERSION(head, _min_ver)                               \
          (((head)->cur_ver < _min_ver) || ((head)->min_ver > _min_ver))  \

#define AR9170_NUM_LEDS                         2

  /* CAM */
#define AR9170_CAM_MAX_USER                     64
#define AR9170_CAM_MAX_KEY_LENGTH               16

#define AR9170_SRAM_OFFSET              0x100000
#define AR9170_SRAM_SIZE                0x18000

#define AR9170_PRAM_OFFSET              0x200000
#define AR9170_PRAM_SIZE                0x8000
/* NOTE: Don't mess with the order of the flags! */
enum carl9170fw_feature_list {
	/* Always set */
	CARL9170FW_DUMMY_FEATURE,

	/*
	 * Indicates that this image has special boot block which prevents
	 * legacy drivers to drive the firmware.
	 */
	CARL9170FW_MINIBOOT,

	/* usb registers are initialized by the firmware */
	CARL9170FW_USB_INIT_FIRMWARE,

	/* command traps & notifications are send through EP2 */
	CARL9170FW_USB_RESP_EP2,

	/* usb download (app -> fw) stream */
	CARL9170FW_USB_DOWN_STREAM,

	/* usb upload (fw -> app) stream */
	CARL9170FW_USB_UP_STREAM,

	/* unusable - reserved to flag non-functional debug firmwares */
	CARL9170FW_UNUSABLE,

	/* AR9170_CMD_RF_INIT, AR9170_CMD_FREQ_START, AR9170_CMD_FREQUENCY */
	CARL9170FW_COMMAND_PHY,

	/* AR9170_CMD_EKEY, AR9170_CMD_DKEY */
	CARL9170FW_COMMAND_CAM,

	/* Firmware has a software Content After Beacon Queueing mechanism */
	CARL9170FW_WLANTX_CAB,

	/* The firmware is capable of responding to incoming BAR frames */
	CARL9170FW_HANDLE_BACK_REQ,

	/* GPIO Interrupt | CARL9170_RSP_GPIO */
	CARL9170FW_GPIO_INTERRUPT,

	/* Firmware PSM support | CARL9170_CMD_PSM */
	CARL9170FW_PSM,

	/* Firmware RX filter | CARL9170_CMD_RX_FILTER */
	CARL9170FW_RX_FILTER,

	/* Wake up on WLAN */
	CARL9170FW_WOL,

	/* Firmware supports PSM in the 5GHZ Band */
	CARL9170FW_FIXED_5GHZ_PSM,

	/* HW (ANI, CCA, MIB) tally counters */
	CARL9170FW_HW_COUNTERS,

	/* Firmware will pass BA when BARs are queued */
	CARL9170FW_RX_BA_FILTER,

	/* Firmware has support to write a byte at a time */
	CARL9170FW_HAS_WREGB_CMD,

	/* Pattern generator */
	CARL9170FW_PATTERN_GENERATOR,

	/* KEEP LAST */
	__CARL9170FW_FEATURE_NUM
};

#define OTUS_MAGIC	"OTAR"
#define MOTD_MAGIC	"MOTD"
#define FIX_MAGIC	"FIX\0"
#define DBG_MAGIC	"DBG\0"
#define CHK_MAGIC	"CHK\0"
#define TXSQ_MAGIC	"TXSQ"
#define WOL_MAGIC	"WOL\0"
#define LAST_MAGIC	"LAST"

#define CARL9170FW_SET_DAY(d) (((d) - 1) % 31)
#define CARL9170FW_SET_MONTH(m) ((((m) - 1) % 12) * 31)
#define CARL9170FW_SET_YEAR(y) (((y) - 10) * 372)

#define CARL9170FW_GET_DAY(d) (((d) % 31) + 1)
#define CARL9170FW_GET_MONTH(m) ((((m) / 31) % 12) + 1)
#define CARL9170FW_GET_YEAR(y) ((y) / 372 + 10)

#define CARL9170FW_MAGIC_SIZE			4

struct carl9170fw_desc_head {
	u8 magic[CARL9170FW_MAGIC_SIZE];
	__le16 length;
	u8 min_ver;
	u8 cur_ver;
} __packed;
#define CARL9170FW_DESC_HEAD_SIZE			\
	(sizeof(struct carl9170fw_desc_head))

#define CARL9170FW_OTUS_DESC_MIN_VER		6
#define CARL9170FW_OTUS_DESC_CUR_VER		7
struct carl9170fw_otus_desc {
	struct carl9170fw_desc_head head;
	__le32 feature_set;
	__le32 fw_address;
	__le32 bcn_addr;
	__le16 bcn_len;
	__le16 miniboot_size;
	__le16 tx_frag_len;
	__le16 rx_max_frame_len;
	u8 tx_descs;
	u8 cmd_bufs;
	u8 api_ver;
	u8 vif_num;
} __packed;
#define CARL9170FW_OTUS_DESC_SIZE			\
	(sizeof(struct carl9170fw_otus_desc))

#define CARL9170FW_MOTD_STRING_LEN			24
#define CARL9170FW_MOTD_RELEASE_LEN			20
#define CARL9170FW_MOTD_DESC_MIN_VER			1
#define CARL9170FW_MOTD_DESC_CUR_VER			2
struct carl9170fw_motd_desc {
	struct carl9170fw_desc_head head;
	__le32 fw_year_month_day;
	char desc[CARL9170FW_MOTD_STRING_LEN];
	char release[CARL9170FW_MOTD_RELEASE_LEN];
} __packed;
#define CARL9170FW_MOTD_DESC_SIZE			\
	(sizeof(struct carl9170fw_motd_desc))

#define CARL9170FW_TXSQ_DESC_MIN_VER                    1
#define CARL9170FW_TXSQ_DESC_CUR_VER                    1
struct carl9170fw_txsq_desc {
	struct carl9170fw_desc_head head;

	__le32 seq_table_addr;
} __packed;
#define CARL9170FW_TXSQ_DESC_SIZE                       \
(sizeof(struct carl9170fw_txsq_desc))
static inline bool carl9170fw_supports(__le32 list, u8 feature)
{
	return le32_to_cpu(list) & BIT(feature);
}

#define CARL9170FW_MIN_SIZE     32
#define CARL9170FW_MAX_SIZE     16384

static inline bool carl9170fw_size_check(unsigned int len)
{
	return (len <= CARL9170FW_MAX_SIZE && len >= CARL9170FW_MIN_SIZE);
}

#endif //__HEAD_H
