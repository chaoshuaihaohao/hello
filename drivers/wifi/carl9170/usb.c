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
#include <linux/input.h>

#include "carl9170.h"
static int carl9170_usb_submit_cmd_urb(struct ar9170 *ar)
{
	struct urb *urb;
	int err;

	if (atomic_inc_return(&ar->tx_cmd_urbs) != 1) {
		atomic_dec(&ar->tx_cmd_urbs);
		return 0;
	}

	urb = usb_get_from_anchor(&ar->tx_cmd);
	if (!urb) {
		atomic_dec(&ar->tx_cmd_urbs);
		return 0;
	}

	usb_anchor_urb(urb, &ar->tx_anch);
	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (unlikely(err)) {
		usb_unanchor_urb(urb);
		atomic_dec(&ar->tx_cmd_urbs);
	}
	usb_free_urb(urb);

	return err;
}


static void carl9170_usb_cmd_complete(struct urb *urb)
{
	struct ar9170 *ar = urb->context;
	int err = 0;

	if (WARN_ON_ONCE(!ar))
		return;

	atomic_dec(&ar->tx_cmd_urbs);

	switch (urb->status) {
	/* everything is fine */
	case 0:
		break;

	/* disconnect */
	case -ENOENT:
	case -ECONNRESET:
	case -ENODEV:
	case -ESHUTDOWN:
		return;

	default:
		err = urb->status;
		break;
	}

	if (!IS_INITIALIZED(ar))
		return;

	if (err)
		dev_err(&ar->udev->dev, "submit cmd cb failed (%d).\n", err);

	err = carl9170_usb_submit_cmd_urb(ar);
	if (err)
		dev_err(&ar->udev->dev, "submit cmd failed (%d).\n", err);
}

int __carl9170_exec_cmd(struct ar9170 *ar, struct carl9170_cmd *cmd,
			const bool free_buf)
{
	struct urb *urb;
	int err = 0;

	if (!IS_INITIALIZED(ar)) {
		err = -EPERM;
		goto err_free;
	}

	if (WARN_ON(cmd->hdr.len > CARL9170_MAX_CMD_LEN - 4)) {
		err = -EINVAL;
		goto err_free;
	}

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		err = -ENOMEM;
		goto err_free;
	}

	if (ar->usb_ep_cmd_is_bulk)
		usb_fill_bulk_urb(urb, ar->udev,
				  usb_sndbulkpipe(ar->udev, AR9170_USB_EP_CMD),
				  cmd, cmd->hdr.len + 4,
				  carl9170_usb_cmd_complete, ar);
	else
		usb_fill_int_urb(urb, ar->udev,
				 usb_sndintpipe(ar->udev, AR9170_USB_EP_CMD),
				 cmd, cmd->hdr.len + 4,
				 carl9170_usb_cmd_complete, ar, 1);

	if (free_buf)
		urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &ar->tx_cmd);
	usb_free_urb(urb);

	return carl9170_usb_submit_cmd_urb(ar);

err_free:
	if (free_buf)
		kfree(cmd);

	return err;
}
int carl9170_exec_cmd(struct ar9170 *ar, const enum carl9170_cmd_oids cmd,
	unsigned int plen, void *payload, unsigned int outlen, void *out)
{
	int err = -ENOMEM;
	unsigned long time_left;

	if (!IS_ACCEPTING_CMD(ar))
		return -EIO;

	if (!(cmd & CARL9170_CMD_ASYNC_FLAG))
		might_sleep();

	ar->cmd.hdr.len = plen;
	ar->cmd.hdr.cmd = cmd;
	/* writing multiple regs fills this buffer already */
	if (plen && payload != (u8 *)(ar->cmd.data))
		memcpy(ar->cmd.data, payload, plen);

	spin_lock_bh(&ar->cmd_lock);
	ar->readbuf = (u8 *)out;
	ar->readlen = outlen;
	spin_unlock_bh(&ar->cmd_lock);

	reinit_completion(&ar->cmd_wait);
	err = __carl9170_exec_cmd(ar, &ar->cmd, false);

	if (!(cmd & CARL9170_CMD_ASYNC_FLAG)) {
		time_left = wait_for_completion_timeout(&ar->cmd_wait, HZ);
		if (time_left == 0) {
			err = -ETIMEDOUT;
			goto err_unbuf;
		}

		if (ar->readlen != outlen) {
			err = -EMSGSIZE;
			goto err_unbuf;
		}
	}

	return 0;

err_unbuf:
	/* Maybe the device was removed in the moment we were waiting? */
	if (IS_STARTED(ar)) {
		dev_err(&ar->udev->dev, "no command feedback "
			"received (%d).\n", err);

		/* provide some maybe useful debug information */
		print_hex_dump_bytes("carl9170 cmd: ", DUMP_PREFIX_NONE,
				     &ar->cmd, plen + 4);

//		carl9170_restart(ar, CARL9170_RR_COMMAND_TIMEOUT);
	}

	/* invalidate to avoid completing the next command prematurely */
	spin_lock_bh(&ar->cmd_lock);
	ar->readbuf = NULL;
	ar->readlen = 0;
	spin_unlock_bh(&ar->cmd_lock);

	return err;
}

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

static void carl9170_cmd_callback(struct ar9170 *ar, u32 len, void *buffer)
{
#if 0
	/*
	 * Some commands may have a variable response length
	 * and we cannot predict the correct length in advance.
	 * So we only check if we provided enough space for the data.
	 */
	if (unlikely(ar->readlen != (len - 4))) {
		dev_warn(&ar->udev->dev, "received invalid command response:"
			 "got %d, instead of %d\n", len - 4, ar->readlen);
		print_hex_dump_bytes("carl9170 cmd:", DUMP_PREFIX_OFFSET,
				     ar->cmd_buf, (ar->cmd.hdr.len + 4) & 0x3f);
		print_hex_dump_bytes("carl9170 rsp:", DUMP_PREFIX_OFFSET,
				     buffer, len);
		/*
		 * Do not complete. The command times out,
		 * and we get a stack trace from there.
		 */
		//                carl9170_restart(ar, CARL9170_RR_INVALID_RSP);
	}
	spin_lock(&ar->cmd_lock);
	if (ar->readbuf) {
		if (len >= 4)
			memcpy(ar->readbuf, buffer + 4, len - 4);

		ar->readbuf = NULL;
	}
	complete(&ar->cmd_wait);
	spin_unlock(&ar->cmd_lock);
#endif
}

void carl9170_handle_command_response(struct ar9170 *ar, void *buf, u32 len)
{
	struct carl9170_rsp *cmd = buf;
	struct ieee80211_vif *vif;

	printk("%s: cmd = %d\n", __func__, cmd->hdr.cmd);
#if 0
	if ((cmd->hdr.cmd & CARL9170_RSP_FLAG) != CARL9170_RSP_FLAG) {
		if (!(cmd->hdr.cmd & CARL9170_CMD_ASYNC_FLAG))
			carl9170_cmd_callback(ar, len, buf);

		return;
	}
	if (unlikely(cmd->hdr.len != (len - 4))) {
		if (net_ratelimit()) {
			wiphy_err(ar->hw->wiphy, "FW: received over-/under"
				  "sized event %x (%d, but should be %d).\n",
				  cmd->hdr.cmd, cmd->hdr.len, len - 4);

			print_hex_dump_bytes("dump:", DUMP_PREFIX_NONE,
					     buf, len);
		}

		return;
	}
#endif

	/* hardware event handlers */
	switch (cmd->hdr.cmd) {
#if 0
	case CARL9170_RSP_PRETBTT:
		/* pre-TBTT event */
		rcu_read_lock();
		vif = carl9170_get_main_vif(ar);

		if (!vif) {
			rcu_read_unlock();
			break;
		}

		switch (vif->type) {
		case NL80211_IFTYPE_STATION:
			carl9170_handle_ps(ar, cmd);
			break;

		case NL80211_IFTYPE_AP:
		case NL80211_IFTYPE_ADHOC:
		case NL80211_IFTYPE_MESH_POINT:
			carl9170_update_beacon(ar, true);
			break;

		default:
			break;
		}
		rcu_read_unlock();

		break;

	case CARL9170_RSP_TXCOMP:
		/* TX status notification */
		carl9170_tx_process_status(ar, cmd);
		break;

	case CARL9170_RSP_BEACON_CONFIG:
		/*
		 * (IBSS) beacon send notification
		 * bytes: 04 c2 XX YY B4 B3 B2 B1
		 *
		 * XX always 80
		 * YY always 00
		 * B1-B4 "should" be the number of send out beacons.
		 */
		break;

	case CARL9170_RSP_ATIM:
		/* End of Atim Window */
		break;

	case CARL9170_RSP_WATCHDOG:
		/* Watchdog Interrupt */
		carl9170_restart(ar, CARL9170_RR_WATCHDOG);
		break;

	case CARL9170_RSP_TEXT:
		/* firmware debug */
		carl9170_dbg_message(ar, (char *)buf + 4, len - 4);
		break;

	case CARL9170_RSP_HEXDUMP:
		wiphy_dbg(ar->hw->wiphy, "FW: HD %d\n", len - 4);
		print_hex_dump_bytes("FW:", DUMP_PREFIX_NONE,
				     (char *)buf + 4, len - 4);
		break;

	case CARL9170_RSP_RADAR:
		if (!net_ratelimit())
			break;

		wiphy_info(ar->hw->wiphy, "FW: RADAR! Please report this "
			   "incident to linux-wireless@vger.kernel.org !\n");
		break;

	case CARL9170_RSP_GPIO:
#ifdef CONFIG_CARL9170_WPC
		if (ar->wps.pbc) {
			bool state =
			    !!(cmd->gpio.gpio &
			       cpu_to_le32
			       (AR9170_GPIO_PORT_WPS_BUTTON_PRESSED));

			if (state != ar->wps.pbc_state) {
				ar->wps.pbc_state = state;
				input_report_key(ar->wps.pbc, KEY_WPS_BUTTON,
						 state);
				input_sync(ar->wps.pbc);
			}
		}
#endif /* CONFIG_CARL9170_WPC */
		break;
#endif
	case CARL9170_RSP_BOOT:
		complete(&ar->fw_boot_wait);
		break;
	default:
		wiphy_err(ar->hw->wiphy, "FW: received unhandled event %x\n",
			  cmd->hdr.cmd);
		print_hex_dump_bytes("dump:", DUMP_PREFIX_NONE, buf, len);
		break;
	}
}

static void carl9170_usb_rx_irq_complete(struct urb *urb)
{
	struct ar9170 *ar = urb->context;
	printk("%s\n", __func__);

	if (WARN_ON_ONCE(!ar))
		return;

	switch (urb->status) {
		/* everything is fine */
	case 0:
		break;

		/* disconnect */
	case -ENOENT:
	case -ECONNRESET:
	case -ENODEV:
	case -ESHUTDOWN:
		return;

	default:
		goto resubmit;
	}

	/*
	 * While the carl9170 firmware does not use this EP, the
	 * firmware loader in the EEPROM unfortunately does.
	 * Therefore we need to be ready to handle out-of-band
	 * responses and traps in case the firmware crashed and
	 * the loader took over again.
	 */
	carl9170_handle_command_response(ar, urb->transfer_buffer,
					 urb->actual_length);

resubmit:
	usb_anchor_urb(urb, &ar->rx_anch);
	if (unlikely(usb_submit_urb(urb, GFP_ATOMIC)))
		usb_unanchor_urb(urb);
}

static int carl9170_usb_send_rx_irq_urb(struct ar9170 *ar)
{
	struct urb *urb = NULL;
	void *ibuf;
	int err = -ENOMEM;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		goto out;

	ibuf = kmalloc(AR9170_USB_EP_CTRL_MAX, GFP_KERNEL);
	if (!ibuf)
		goto out;

	usb_fill_int_urb(urb, ar->udev, usb_rcvintpipe(ar->udev,
						       AR9170_USB_EP_IRQ), ibuf,
			 AR9170_USB_EP_CTRL_MAX, carl9170_usb_rx_irq_complete,
			 ar, 1);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &ar->rx_anch);
	err = usb_submit_urb(urb, GFP_KERNEL);
	if (err)
		usb_unanchor_urb(urb);

out:
	usb_free_urb(urb);
	return err;
}

static int carl9170_usb_submit_rx_urb(struct ar9170 *ar, gfp_t gfp)
{
	struct urb *urb;
	int err = 0, runs = 0;

	while ((atomic_read(&ar->rx_anch_urbs) < AR9170_NUM_RX_URBS) &&
	       (runs++ < AR9170_NUM_RX_URBS)) {
		err = -ENOSPC;
		urb = usb_get_from_anchor(&ar->rx_pool);
		if (urb) {
			usb_anchor_urb(urb, &ar->rx_anch);
			err = usb_submit_urb(urb, gfp);
			if (unlikely(err)) {
				usb_unanchor_urb(urb);
				usb_anchor_urb(urb, &ar->rx_pool);
			} else {
				atomic_dec(&ar->rx_pool_urbs);
				atomic_inc(&ar->rx_anch_urbs);
			}
			usb_free_urb(urb);
		}
	}

	return err;
}

static void carl9170_usb_rx_complete(struct urb *urb)
{
	printk("%s\n", __func__);
	struct ar9170 *ar = (struct ar9170 *)urb->context;
	int err;

	if (WARN_ON_ONCE(!ar))
		return;

	atomic_dec(&ar->rx_anch_urbs);

	switch (urb->status) {
	case 0:
		/* rx path */
		usb_anchor_urb(urb, &ar->rx_work);
		atomic_inc(&ar->rx_work_urbs);
		break;

	case -ENOENT:
	case -ECONNRESET:
	case -ENODEV:
	case -ESHUTDOWN:
		/* handle disconnect events */
		return;

	default:
		/* handle all other errors */
		usb_anchor_urb(urb, &ar->rx_pool);
		atomic_inc(&ar->rx_pool_urbs);
		break;
	}

	err = carl9170_usb_submit_rx_urb(ar, GFP_ATOMIC);
	if (unlikely(err)) {
#if 0
		/*
		 * usb_submit_rx_urb reported a problem.
		 * In case this is due to a rx buffer shortage,
		 * elevate the tasklet worker priority to
		 * the highest available level.
		 */
		tasklet_hi_schedule(&ar->usb_tasklet);

		if (atomic_read(&ar->rx_anch_urbs) == 0) {
			/*
			 * The system is too slow to cope with
			 * the enormous workload. We have simply
			 * run out of active rx urbs and this
			 * unfortunately leads to an unpredictable
			 * device.
			 */

			ieee80211_queue_work(ar->hw, &ar->ping_work);
		}
#endif
	} else {
		/*
		 * Using anything less than _high_ priority absolutely
		 * kills the rx performance my UP-System...
		 */
		//      tasklet_hi_schedule(&ar->usb_tasklet);
	}
}

static struct urb *carl9170_usb_alloc_rx_urb(struct ar9170 *ar, gfp_t gfp)
{
	struct urb *urb;
	void *buf;

	buf = kmalloc(ar->fw.rx_size, gfp);
	if (!buf)
		return NULL;

	urb = usb_alloc_urb(0, gfp);
	if (!urb) {
		kfree(buf);
		return NULL;
	}

	usb_fill_bulk_urb(urb, ar->udev, usb_rcvbulkpipe(ar->udev,
							 AR9170_USB_EP_RX), buf,
			  ar->fw.rx_size, carl9170_usb_rx_complete, ar);

	urb->transfer_flags |= URB_FREE_BUFFER;

	return urb;
}

static int carl9170_usb_init_rx_bulk_urbs(struct ar9170 *ar)
{
	struct urb *urb;
	int i, err = -EINVAL;

	/*
	 * The driver actively maintains a second shadow
	 * pool for inactive, but fully-prepared rx urbs.
	 *
	 * The pool should help the driver to master huge
	 * workload spikes without running the risk of
	 * undersupplying the hardware or wasting time by
	 * processing rx data (streams) inside the urb
	 * completion (hardirq context).
	 */
	for (i = 0; i < AR9170_NUM_RX_URBS_POOL; i++) {
		urb = carl9170_usb_alloc_rx_urb(ar, GFP_KERNEL);
		if (!urb) {
			err = -ENOMEM;
			goto err_out;
		}

		usb_anchor_urb(urb, &ar->rx_pool);
		atomic_inc(&ar->rx_pool_urbs);
		usb_free_urb(urb);
	}

	err = carl9170_usb_submit_rx_urb(ar, GFP_KERNEL);
	if (err)
		goto err_out;

	/* the device now waiting for the firmware. */
	//carl9170_set_state_when(ar, CARL9170_STOPPED, CARL9170_IDLE);
	return 0;

err_out:

	usb_scuttle_anchored_urbs(&ar->rx_pool);
	usb_scuttle_anchored_urbs(&ar->rx_work);
	usb_kill_anchored_urbs(&ar->rx_anch);
	return err;
}

static void carl9170_usb_cancel_urbs(struct ar9170 *ar)
{
	int err;

#if 0
	//         carl9170_set_state(ar, CARL9170_UNKNOWN_STATE);

//          err = carl9170_usb_flush(ar);
	//        if (err)
	//              dev_err(&ar->udev->dev, "stuck tx urbs!\n");

	usb_poison_anchored_urbs(&ar->tx_anch);
//          carl9170_usb_handle_tx_err(ar);
#endif
	usb_poison_anchored_urbs(&ar->rx_anch);

#if 1
//      tasklet_kill(&ar->usb_tasklet);

	usb_scuttle_anchored_urbs(&ar->rx_work);
	usb_scuttle_anchored_urbs(&ar->rx_pool);
	usb_scuttle_anchored_urbs(&ar->tx_cmd);
#endif
}

int carl9170_usb_open(struct ar9170 *ar)
{
//          usb_unpoison_anchored_urbs(&ar->tx_anch);

//          carl9170_set_state_when(ar, CARL9170_STOPPED, CARL9170_IDLE);
	return 0;
}

void carl9170_usb_stop(struct ar9170 *ar)
{
	int ret;

#if 0
	carl9170_set_state_when(ar, CARL9170_IDLE, CARL9170_STOPPED);

	ret = carl9170_usb_flush(ar);
	if (ret)
		dev_err(&ar->udev->dev, "kill pending tx urbs.\n");
	usb_poison_anchored_urbs(&ar->tx_anch);
	carl9170_usb_handle_tx_err(ar);

	/* kill any pending command */
	spin_lock_bh(&ar->cmd_lock);
	ar->readlen = 0;
	spin_unlock_bh(&ar->cmd_lock);
	complete(&ar->cmd_wait);

	/*
	 * Note:
	 * So far we freed all tx urbs, but we won't dare to touch any rx urbs.
	 * Else we would end up with a unresponsive device...
	 */
#endif
}

static int carl9170_usb_load_firmware(struct ar9170 *ar)
{
	const u8 *data;
	u8 *buf;
	unsigned int transfer;
	size_t len;
	u32 addr;
	int err = 0;

	buf = kmalloc(4096, GFP_KERNEL);
	if (!buf) {
		err = -ENOMEM;
		goto err_out;
	}

	data = ar->fw.fw->data;
	len = ar->fw.fw->size;
	addr = ar->fw.address;

	/* this removes the miniboot image */
	data += ar->fw.offset;
	len -= ar->fw.offset;

	while (len) {
		transfer = min_t(unsigned int, len, 4096u);
		memcpy(buf, data, transfer);

		err = usb_control_msg(ar->udev, usb_sndctrlpipe(ar->udev, 0),
				      0x30 /* FW DL */ , 0x40 | USB_DIR_OUT,
				      addr >> 8, 0, buf, transfer, 100);
		if (err < 0) {
			kfree(buf);
			dev_err(&ar->udev->dev, "fw dl failed!\n");
			goto err_out;
		}

		len -= transfer;
		data += transfer;
		addr += transfer;
	}
	kfree(buf);

	err = usb_control_msg(ar->udev, usb_sndctrlpipe(ar->udev, 0),
			      0x31 /* FW DL COMPLETE */ ,
			      0x40 | USB_DIR_OUT, 0, 0, NULL, 0, 200);
	if (err < 0)
		dev_err(&ar->udev->dev, "fw dl complete failed!\n");

	if (wait_for_completion_timeout(&ar->fw_boot_wait, HZ) == 0) {
		err = -ETIMEDOUT;
		goto err_out;
	}

#if 0
	err = carl9170_echo_test(ar, 0x4a110123);
	if (err)
		goto err_out;

	/* now, start the command response counter */
	ar->cmd_seq = -1;
#endif
	return 0;

err_out:
	dev_err(&ar->udev->dev, "firmware upload failed (%d).\n", err);
	return err;
}

static int carl9170_usb_init_device(struct ar9170 *ar)
{
	int err;

	/*
	 * The carl9170 firmware let's the driver know when it's
	 * ready for action. But we have to be prepared to gracefully
	 * handle all spurious [flushed] messages after each (re-)boot.
	 * Thus the command response counter remains disabled until it
	 * can be safely synchronized.
	 */
	//        ar->cmd_seq = -2;

	err = carl9170_usb_send_rx_irq_urb(ar);
	if (err)
		goto err_out;

	err = carl9170_usb_init_rx_bulk_urbs(ar);
	if (err)
		goto err_unrx;

	err = carl9170_usb_open(ar);
	if (err)
		goto err_unrx;

	mutex_lock(&ar->mutex);
	err = carl9170_usb_load_firmware(ar);
	mutex_unlock(&ar->mutex);
	if (err)
		goto err_stop;

	return 0;

err_stop:
	carl9170_usb_stop(ar);

err_unrx:
	carl9170_usb_cancel_urbs(ar);

err_out:
	return err;
}

static struct usb_driver carl9170_driver;
static void carl9170_usb_firmware_failed(struct ar9170 *ar)
{
	/* Store a copies of the usb_interface and usb_device pointer locally.
	 * This is because release_driver initiates carl9170_usb_disconnect,
	 * which in turn frees our driver context (ar).
	 */
	struct usb_interface *intf = ar->intf;
	struct usb_device *udev = ar->udev;

	complete(&ar->fw_load_wait);
	/* at this point 'ar' could be already freed. Don't use it anymore */
	ar = NULL;

#if 1
	/* unbind anything failed */
	usb_lock_device(udev);
	usb_driver_release_interface(&carl9170_driver, intf);
	usb_unlock_device(udev);
#endif
	usb_put_intf(intf);
}

static void carl9170_usb_firmware_finish(struct ar9170 *ar)
{
	struct usb_interface *intf = ar->intf;
	int err;

	//解析firmware并给hw结构赋值
	err = carl9170_parse_firmware(ar);
	if (err)
		goto err_freefw;

	err = carl9170_usb_init_device(ar);
	if (err)
		goto err_freefw;

	err = carl9170_register(ar);

	carl9170_usb_stop(ar);
	if (err)
		goto err_unrx;

	complete(&ar->fw_load_wait);
	usb_put_intf(intf);
	return;

#if 1
err_unrx:
	carl9170_usb_cancel_urbs(ar);
#endif

err_freefw:
	carl9170_release_firmware(ar);
	carl9170_usb_firmware_failed(ar);
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
	carl9170_usb_firmware_failed(ar);
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

	init_usb_anchor(&ar->rx_anch);
	init_usb_anchor(&ar->rx_pool);
	init_usb_anchor(&ar->rx_work);
	init_usb_anchor(&ar->tx_wait);
	init_usb_anchor(&ar->tx_anch);
	init_usb_anchor(&ar->tx_cmd);
	init_usb_anchor(&ar->tx_err);
//      init_completion(&ar->cmd_wait);
	init_completion(&ar->fw_boot_wait);
	init_completion(&ar->fw_load_wait);
//      tasklet_init(&ar->usb_tasklet, carl9170_usb_tasklet, (unsigned long)ar);

	atomic_set(&ar->tx_cmd_urbs, 0);
	atomic_set(&ar->tx_anch_urbs, 0);
	atomic_set(&ar->rx_work_urbs, 0);
	atomic_set(&ar->rx_pool_urbs, 0);
	atomic_set(&ar->rx_anch_urbs, 0);
//      ar->cmd_seq = -2;
	usb_get_dev(ar->udev);

//       carl9170_set_state(ar, CARL9170_STOPPED);

	return request_firmware_nowait(THIS_MODULE, 1, CARL9170FW_NAME,
				       &ar->udev->dev, GFP_KERNEL, ar,
				       carl9170_usb_firmware_step2);
}

static void carl9170_usb_disconnect(struct usb_interface *intf)
{
	struct ar9170 *ar = usb_get_intfdata(intf);

	wait_for_completion(&ar->fw_load_wait);

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
