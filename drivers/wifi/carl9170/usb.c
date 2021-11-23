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

#include "head.h"

static void carl9170_fw_set_if_combinations(struct ar9170 *ar,
					    u16 if_comb_types)
{
	if (ar->fw.vif_num < 2)
		return;

	ar->if_comb_limits[0].max = ar->fw.vif_num;
	ar->if_comb_limits[0].types = if_comb_types;

	ar->if_combs[0].num_different_channels = 1;
	ar->if_combs[0].max_interfaces = ar->fw.vif_num;
	ar->if_combs[0].limits = ar->if_comb_limits;
	ar->if_combs[0].n_limits = ARRAY_SIZE(ar->if_comb_limits);

	ar->hw->wiphy->iface_combinations = ar->if_combs;
	ar->hw->wiphy->n_iface_combinations = ARRAY_SIZE(ar->if_combs);
}

static inline bool carl9170fw_desc_cmp(const struct carl9170fw_desc_head *head,
				       const u8 descid[CARL9170FW_MAGIC_SIZE],
				       u16 min_len, u8 compatible_revision)
{
	if (descid[0] == head->magic[0] && descid[1] == head->magic[1] &&
	    descid[2] == head->magic[2] && descid[3] == head->magic[3] &&
	    !CHECK_HDR_VERSION(head, compatible_revision) &&
	    (le16_to_cpu(head->length) >= min_len))
		return true;

	return false;
}

static const u8 otus_magic[4] = { OTUS_MAGIC };

static struct carl9170fw_desc_head *carl9170_find_fw_desc(struct ar9170 *ar,
							  const __u8 * fw_data,
							  const size_t len)
{
	int scan = 0, found = 0;

	if (!carl9170fw_size_check(len)) {
		dev_err(&ar->udev->dev, "firmware size is out of bound.\n");
		return NULL;
	}

	while (scan < len - sizeof(struct carl9170fw_desc_head)) {
		if (fw_data[scan++] == otus_magic[found])
			found++;
		else
			found = 0;

		if (scan >= len)
			break;

		if (found == sizeof(otus_magic))
			break;
	}

	if (found != sizeof(otus_magic))
		return NULL;

	printk("fw len = %d, fw desc addr = %d\n", len, scan - found);
	return (void *)&fw_data[scan - found];
}

static const void *carl9170_fw_find_desc(struct ar9170 *ar, const u8 descid[4],
					 const unsigned int len,
					 const u8 compatible_revision)
{
	const struct carl9170fw_desc_head *iter;

	carl9170fw_for_each_hdr(iter, ar->fw.desc) {
		if (carl9170fw_desc_cmp(iter, descid, len, compatible_revision))
			return (void *)iter;
	}

	/* needed to find the LAST desc */
	if (carl9170fw_desc_cmp(iter, descid, len, compatible_revision))
		return (void *)iter;

	return NULL;
}

static bool valid_dma_addr(const u32 address)
{
	if (address >= AR9170_SRAM_OFFSET &&
	    address < (AR9170_SRAM_OFFSET + AR9170_SRAM_SIZE))
		return true;

	return false;
}

static bool valid_cpu_addr(const u32 address)
{
	if (valid_dma_addr(address) || (address >= AR9170_PRAM_OFFSET &&
					address <
					(AR9170_PRAM_OFFSET +
					 AR9170_PRAM_SIZE)))
		return true;

	return false;
}

static int carl9170_fw_tx_sequence(struct ar9170 *ar)
{
	const struct carl9170fw_txsq_desc *txsq_desc;

	txsq_desc = carl9170_fw_find_desc(ar, TXSQ_MAGIC, sizeof(*txsq_desc),
					  CARL9170FW_TXSQ_DESC_CUR_VER);
	if (txsq_desc) {
		ar->fw.tx_seq_table = le32_to_cpu(txsq_desc->seq_table_addr);
		if (!valid_cpu_addr(ar->fw.tx_seq_table))
			return -EINVAL;
	} else {
		ar->fw.tx_seq_table = 0;
	}

	return 0;
}

static int carl9170_fw(struct ar9170 *ar, const __u8 * data, size_t len)
{
	const struct carl9170fw_otus_desc *otus_desc;
	int err;
	u16 if_comb_types;

	otus_desc = carl9170_fw_find_desc(ar, OTUS_MAGIC,
					  sizeof(*otus_desc),
					  CARL9170FW_OTUS_DESC_CUR_VER);
	if (!otus_desc) {
		return -ENODATA;
	}
#define SUPP(feat)                                              \
	(carl9170fw_supports(otus_desc->feature_set, feat))
	ar->fw.api_version = otus_desc->api_ver;

	if (ar->fw.api_version < CARL9170FW_API_MIN_VER ||
	    ar->fw.api_version > CARL9170FW_API_MAX_VER) {
		dev_err(&ar->udev->dev, "unsupported firmware api version.\n");
		return -EINVAL;
	}

	if (!SUPP(CARL9170FW_COMMAND_PHY) || SUPP(CARL9170FW_UNUSABLE) ||
	    !SUPP(CARL9170FW_HANDLE_BACK_REQ)) {
		dev_err(&ar->udev->dev, "firmware does support "
			"mandatory features.\n");
		return -ECANCELED;
	}

	if (ilog2(le32_to_cpu(otus_desc->feature_set)) >=
	    __CARL9170FW_FEATURE_NUM) {
		dev_warn(&ar->udev->dev, "driver does not support all "
			 "firmware features.\n");
	}

	if (!SUPP(CARL9170FW_COMMAND_CAM)) {
		dev_info(&ar->udev->dev, "crypto offloading is disabled "
			 "by firmware.\n");
		ar->fw.disable_offload_fw = true;
	}

	if (SUPP(CARL9170FW_PSM) && SUPP(CARL9170FW_FIXED_5GHZ_PSM))
		ieee80211_hw_set(ar->hw, SUPPORTS_PS);

	if (!SUPP(CARL9170FW_USB_INIT_FIRMWARE)) {
		dev_err(&ar->udev->dev, "firmware does not provide "
			"mandatory interfaces.\n");
		return -EINVAL;
	}

	if (SUPP(CARL9170FW_MINIBOOT))
		ar->fw.offset = le16_to_cpu(otus_desc->miniboot_size);
	else
		ar->fw.offset = 0;

#if 0
	if (SUPP(CARL9170FW_USB_DOWN_STREAM)) {
		ar->hw->extra_tx_headroom += sizeof(struct ar9170_stream);
		ar->fw.tx_stream = true;
	}

	if (SUPP(CARL9170FW_USB_UP_STREAM))
		ar->fw.rx_stream = true;

	if (SUPP(CARL9170FW_RX_FILTER)) {
		ar->fw.rx_filter = true;
		ar->rx_filter_caps = FIF_FCSFAIL | FIF_PLCPFAIL |
		    FIF_CONTROL | FIF_PSPOLL | FIF_OTHER_BSS;
	}
#endif
	if (SUPP(CARL9170FW_HW_COUNTERS))
		ar->fw.hw_counters = true;

	if (SUPP(CARL9170FW_WOL))
		device_set_wakeup_enable(&ar->udev->dev, true);

	if (SUPP(CARL9170FW_RX_BA_FILTER))
		ar->fw.ba_filter = true;

	if_comb_types = BIT(NL80211_IFTYPE_STATION) |
	    BIT(NL80211_IFTYPE_P2P_CLIENT);

	ar->fw.vif_num = otus_desc->vif_num;
	ar->fw.cmd_bufs = otus_desc->cmd_bufs;
	ar->fw.address = le32_to_cpu(otus_desc->fw_address);
	ar->fw.rx_size = le16_to_cpu(otus_desc->rx_max_frame_len);
	ar->fw.mem_blocks = min_t(unsigned int, otus_desc->tx_descs, 0xfe);
//          atomic_set(&ar->mem_free_blocks, ar->fw.mem_blocks);
	ar->fw.mem_block_size = le16_to_cpu(otus_desc->tx_frag_len);

	if (ar->fw.vif_num >= AR9170_MAX_VIRTUAL_MAC || !ar->fw.vif_num ||
	    ar->fw.mem_blocks < 16 || !ar->fw.cmd_bufs ||
	    ar->fw.mem_block_size < 64 || ar->fw.mem_block_size > 512 ||
	    ar->fw.rx_size > 32768 || ar->fw.rx_size < 4096 ||
	    !valid_cpu_addr(ar->fw.address)) {
		dev_err(&ar->udev->dev, "firmware shows obvious signs of "
			"malicious tampering.\n");
		return -EINVAL;
	}

	ar->fw.beacon_addr = le32_to_cpu(otus_desc->bcn_addr);
	ar->fw.beacon_max_len = le16_to_cpu(otus_desc->bcn_len);

	if (valid_dma_addr(ar->fw.beacon_addr) && ar->fw.beacon_max_len >=
	    AR9170_MAC_BCN_LENGTH_MAX) {
		ar->hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_ADHOC);

		if (SUPP(CARL9170FW_WLANTX_CAB)) {
			if_comb_types |= BIT(NL80211_IFTYPE_AP);

#ifdef CONFIG_MAC80211_MESH
			if_comb_types |= BIT(NL80211_IFTYPE_MESH_POINT);
#endif /* CONFIG_MAC80211_MESH */
		}
	}

	carl9170_fw_set_if_combinations(ar, if_comb_types);

	ar->hw->wiphy->interface_modes |= if_comb_types;

	ar->hw->wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;

	/* As IBSS Encryption is software-based, IBSS RSN is supported. */
	ar->hw->wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
	    WIPHY_FLAG_IBSS_RSN | WIPHY_FLAG_SUPPORTS_TDLS;

#undef SUPPORTED
	return carl9170_fw_tx_sequence(ar);
}

int carl9170_parse_firmware(struct ar9170 *ar)
{
	const struct carl9170fw_desc_head *fw_desc;
	const struct firmware *fw = ar->fw.fw;
	unsigned long header_offset;

	int err;

	fw_desc = carl9170_find_fw_desc(ar, fw->data, fw->size);
	if (!fw_desc) {
		dev_err(&ar->udev->dev, "unsupported firmware.\n");
		return -ENODATA;
	}

	ar->fw.desc = fw_desc;

	err = carl9170_fw(ar, fw->data, fw->size);
	if (err) {
		dev_err(&ar->udev->dev, "failed to parse firmware (%d).\n",
			err);
		return err;
	}

	return 0;
}

static void carl9170_release_firmware(struct ar9170 *ar)
{
	if (ar->fw.fw) {
		release_firmware(ar->fw.fw);
		memset(&ar->fw, 0, sizeof(ar->fw));
	}
}

static void carl9170_usb_firmware_finish(struct ar9170 *ar)
{
	struct usb_interface *intf = ar->intf;
	int err;

	//解析firmware并给hw结构赋值
	err = carl9170_parse_firmware(ar);
	if (err)
		goto err_freefw;

err_freefw:
	carl9170_release_firmware(ar);
}

static void carl9170_usb_firmware_step2(const struct firmware *fw,
					void *context)
{
	struct ar9170 *ar = context;
	if (fw) {
		ar->fw.fw = fw;
		carl9170_usb_firmware_finish(ar);
		return;
	}
	dev_err(&ar->udev->dev, "firmware not found.\n");
}

static int carl9170_op_start(struct ieee80211_hw *hw)
{
	return 0;
}

static void carl9170_op_stop(struct ieee80211_hw *hw)
{
}

void carl9170_op_tx(struct ieee80211_hw *hw,
		    struct ieee80211_tx_control *control, struct sk_buff *skb)
{
	return 0;
}

static void carl9170_op_flush(struct ieee80211_hw *hw,
			      struct ieee80211_vif *vif, u32 queues, bool drop)
{
}

static int carl9170_op_add_interface(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif)
{
	return 0;
}

static void carl9170_op_remove_interface(struct ieee80211_hw *hw,
					 struct ieee80211_vif *vif)
{
}

static int carl9170_op_config(struct ieee80211_hw *hw, u32 changed)
{
	return 0;
}

static void carl9170_op_configure_filter(struct ieee80211_hw *hw,
					 unsigned int changed_flags,
					 unsigned int *new_flags, u64 multicast)
{
}

static const struct ieee80211_ops carl9170_ops = {
	.start = carl9170_op_start,
	.stop = carl9170_op_stop,
	.tx = carl9170_op_tx,
	//      .flush                  = carl9170_op_flush,
	.add_interface = carl9170_op_add_interface,
	.remove_interface = carl9170_op_remove_interface,
	.config = carl9170_op_config,
//       .prepare_multicast      = carl9170_op_prepare_multicast,
	.configure_filter = carl9170_op_configure_filter,
#if 0
	.conf_tx = carl9170_op_conf_tx,
	.bss_info_changed = carl9170_op_bss_info_changed,
	.get_tsf = carl9170_op_get_tsf,
	.set_key = carl9170_op_set_key,
	.sta_add = carl9170_op_sta_add,
	.sta_remove = carl9170_op_sta_remove,
	.sta_notify = carl9170_op_sta_notify,
	.get_survey = carl9170_op_get_survey,
	.get_stats = carl9170_op_get_stats,
	.ampdu_action = carl9170_op_ampdu_action,
#endif
};

void *carl9170_alloc(size_t priv_size)
{
	struct ieee80211_hw *hw;
	struct ar9170 *ar;
	struct sk_buff *skb;
	int i;

#if 0
	/*
	 * this buffer is used for rx stream reconstruction.
	 * Under heavy load this device (or the transport layer?)
	 * tends to split the streams into separate rx descriptors.
	 */

	skb = __dev_alloc_skb(AR9170_RX_STREAM_MAX_SIZE, GFP_KERNEL);
	if (!skb)
		goto err_nomem;
#endif
	hw = ieee80211_alloc_hw(priv_size, &carl9170_ops);
	if (!hw)
		goto err_nomem;

	ar = hw->priv;
	ar->hw = hw;
#if 0
	ar->rx_failover = skb;

	memset(&ar->rx_plcp, 0, sizeof(struct ar9170_rx_head));
	ar->rx_has_plcp = false;
#endif
	/*
	 * Here's a hidden pitfall!
	 *
	 * All 4 AC queues work perfectly well under _legacy_ operation.
	 * However as soon as aggregation is enabled, the traffic flow
	 * gets very bumpy. Therefore we have to _switch_ to a
	 * software AC with a single HW queue.
	 */
	hw->queues = __AR9170_NUM_TXQ;

	mutex_init(&ar->mutex);
#if 0
	spin_lock_init(&ar->beacon_lock);
	spin_lock_init(&ar->cmd_lock);
	spin_lock_init(&ar->tx_stats_lock);
	spin_lock_init(&ar->tx_ampdu_list_lock);
	spin_lock_init(&ar->mem_lock);
	spin_lock_init(&ar->state_lock);
	atomic_set(&ar->pending_restarts, 0);
	ar->vifs = 0;
	for (i = 0; i < ar->hw->queues; i++) {
		skb_queue_head_init(&ar->tx_status[i]);
		skb_queue_head_init(&ar->tx_pending[i]);

		INIT_LIST_HEAD(&ar->bar_list[i]);
		spin_lock_init(&ar->bar_list_lock[i]);
	}
	INIT_WORK(&ar->ps_work, carl9170_ps_work);
	INIT_WORK(&ar->ping_work, carl9170_ping_work);
	INIT_WORK(&ar->restart_work, carl9170_restart_work);
	INIT_WORK(&ar->ampdu_work, carl9170_ampdu_work);
	INIT_DELAYED_WORK(&ar->stat_work, carl9170_stat_work);
	INIT_DELAYED_WORK(&ar->tx_janitor, carl9170_tx_janitor);
	INIT_LIST_HEAD(&ar->tx_ampdu_list);
	rcu_assign_pointer(ar->tx_ampdu_iter,
			   (struct carl9170_sta_tid *)&ar->tx_ampdu_list);

	bitmap_zero(&ar->vif_bitmap, ar->fw.vif_num);
	INIT_LIST_HEAD(&ar->vif_list);
	init_completion(&ar->tx_flush);
#endif
	/* firmware decides which modes we support */
	hw->wiphy->interface_modes = 0;
	ieee80211_hw_set(hw, RX_INCLUDES_FCS);
	ieee80211_hw_set(hw, MFP_CAPABLE);
	ieee80211_hw_set(hw, REPORTS_TX_ACK_STATUS);
	ieee80211_hw_set(hw, SUPPORTS_PS);
	ieee80211_hw_set(hw, PS_NULLFUNC_STACK);
	ieee80211_hw_set(hw, NEED_DTIM_BEFORE_ASSOC);
	ieee80211_hw_set(hw, SUPPORTS_RC_TABLE);
	ieee80211_hw_set(hw, SIGNAL_DBM);
	ieee80211_hw_set(hw, SUPPORTS_HT_CCK_RATES);
#if 0
	if (!modparam_noht) {
		/*
		 * see the comment above, why we allow the user
		 * to disable HT by a module parameter.
		 */
		ieee80211_hw_set(hw, AMPDU_AGGREGATION);
	}
	hw->extra_tx_headroom = sizeof(struct _carl9170_tx_superframe);
	hw->sta_data_size = sizeof(struct carl9170_sta_info);
	hw->vif_data_size = sizeof(struct carl9170_vif_info);

	hw->max_rates = CARL9170_TX_MAX_RATES;
	hw->max_rate_tries = CARL9170_TX_USER_RATE_TRIES;

	for (i = 0; i < ARRAY_SIZE(ar->noise); i++)
		ar->noise[i] = -95;	/* ATH_DEFAULT_NOISE_FLOOR */
#endif

	wiphy_ext_feature_set(hw->wiphy, NL80211_EXT_FEATURE_CQM_RSSI_LIST);

	return ar;

err_nomem:
//          kfree_skb(skb);
	return ERR_PTR(-ENOMEM);
}

static int carl9170_usb_probe(struct usb_interface *intf,
			      const struct usb_device_id *id)
{
	struct ar9170 *ar;
	struct usb_device *udev;
	int err;

	ar = carl9170_alloc(sizeof(*ar));
	if (IS_ERR(ar))
		return PTR_ERR(ar);

	udev = interface_to_usbdev(intf);
	usb_get_dev(udev);
	ar->udev = udev;
	ar->intf = intf;
	ar->features = id->driver_info;

	usb_set_intfdata(intf, ar);
	SET_IEEE80211_DEV(ar->hw, &intf->dev);

#if 0
	init_usb_anchor(&ar->rx_anch);
	init_usb_anchor(&ar->rx_pool);
	init_usb_anchor(&ar->rx_work);
	init_usb_anchor(&ar->tx_wait);
	init_usb_anchor(&ar->tx_anch);
	init_usb_anchor(&ar->tx_cmd);
	init_usb_anchor(&ar->tx_err);
	init_completion(&ar->cmd_wait);
	init_completion(&ar->fw_boot_wait);
	init_completion(&ar->fw_load_wait);
	tasklet_init(&ar->usb_tasklet, carl9170_usb_tasklet, (unsigned long)ar);

	atomic_set(&ar->tx_cmd_urbs, 0);
	atomic_set(&ar->tx_anch_urbs, 0);
	atomic_set(&ar->rx_work_urbs, 0);
	atomic_set(&ar->rx_anch_urbs, 0);
	atomic_set(&ar->rx_pool_urbs, 0);
	ar->cmd_seq = -2;
#endif
	usb_get_dev(ar->udev);

//       carl9170_set_state(ar, CARL9170_STOPPED);

	return request_firmware_nowait(THIS_MODULE, 1, CARL9170FW_NAME,
				       &ar->udev->dev, GFP_KERNEL, ar,
				       carl9170_usb_firmware_step2);
}

static void carl9170_usb_disconnect(struct usb_interface *intf)
{
	struct ar9170 *ar = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	carl9170_release_firmware(ar);
	ieee80211_free_hw(ar->hw);

}

#ifdef CONFIG_PM
static int carl9170_usb_suspend(struct usb_interface *intf,
				pm_message_t message)
{
	return 0;
}

static int carl9170_usb_resume(struct usb_interface *intf)
{
	return 0;
}
#endif
static const struct usb_device_id carl9170_usb_ids[] = {
	/* Netgear WN111 v2 */
	{ USB_DEVICE(0x0846, 0x9001),.driver_info = CARL9170_ONE_LED },
	/* terminate */
	{ }
};

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
//      .disable_hub_initiated_lpm = 1,
};

module_usb_driver(carl9170_driver);

MODULE_AUTHOR("Hao Chen <947481900@qq.com>");
MODULE_DESCRIPTION("wifi driver");
MODULE_LICENSE("GPL v2");
