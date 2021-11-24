#include <linux/slab.h>
#include <linux/module.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <net/mac80211.h>
#include <net/cfg80211.h>
#include "carl9170.h"

#define RATE(_bitrate, _hw_rate, _txpidx, _flags) {	\
	.bitrate	= (_bitrate),			\
	.flags		= (_flags),			\
	.hw_value	= (_hw_rate) | (_txpidx) << 4,	\
}

struct ieee80211_rate __carl9170_ratetable[] = {
	RATE(10, 0, 0, 0),
	RATE(20, 1, 1, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(55, 2, 2, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(110, 3, 3, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(60, 0xb, 0, 0),
	RATE(90, 0xf, 0, 0),
	RATE(120, 0xa, 0, 0),
	RATE(180, 0xe, 0, 0),
	RATE(240, 0x9, 0, 0),
	RATE(360, 0xd, 1, 0),
	RATE(480, 0x8, 2, 0),
	RATE(540, 0xc, 3, 0),
};
#undef RATE

#define carl9170_g_ratetable	(__carl9170_ratetable + 0)
#define carl9170_g_ratetable_size	12
#define carl9170_a_ratetable	(__carl9170_ratetable + 4)
#define carl9170_a_ratetable_size	8

/*
 * NB: The hw_value is used as an index into the carl9170_phy_freq_params
 *     array in phy.c so that we don't have to do frequency lookups!
 */
#define CHAN(_freq, _idx) {		\
	.center_freq	= (_freq),	\
	.hw_value	= (_idx),	\
	.max_power	= 18, /* XXX */	\
}

#define CARL9170_HT_CAP							\
{									\
	.ht_supported	= true,						\
	.cap		= IEEE80211_HT_CAP_MAX_AMSDU |			\
			  IEEE80211_HT_CAP_SUP_WIDTH_20_40 |		\
			  IEEE80211_HT_CAP_SGI_40 |			\
			  IEEE80211_HT_CAP_DSSSCCK40 |			\
			  IEEE80211_HT_CAP_SM_PS,			\
	.ampdu_factor	= IEEE80211_HT_MAX_AMPDU_64K,			\
	.ampdu_density	= IEEE80211_HT_MPDU_DENSITY_8,			\
	.mcs		= {						\
		.rx_mask = { 0xff, 0xff, 0, 0, 0x1, 0, 0, 0, 0, 0, },	\
		.rx_highest = cpu_to_le16(300),				\
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,		\
	},								\
}

static struct ieee80211_channel carl9170_2ghz_chantable[] = {
	CHAN(2412,  0),
	CHAN(2417,  1),
	CHAN(2422,  2),
	CHAN(2427,  3),
	CHAN(2432,  4),
	CHAN(2437,  5),
	CHAN(2442,  6),
	CHAN(2447,  7),
	CHAN(2452,  8),
	CHAN(2457,  9),
	CHAN(2462, 10),
	CHAN(2467, 11),
	CHAN(2472, 12),
	CHAN(2484, 13),
};

static struct ieee80211_channel carl9170_5ghz_chantable[] = {
	CHAN(4920, 14),
	CHAN(4940, 15),
	CHAN(4960, 16),
	CHAN(4980, 17),
	CHAN(5040, 18),
	CHAN(5060, 19),
	CHAN(5080, 20),
	CHAN(5180, 21),
	CHAN(5200, 22),
	CHAN(5220, 23),
	CHAN(5240, 24),
	CHAN(5260, 25),
	CHAN(5280, 26),
	CHAN(5300, 27),
	CHAN(5320, 28),
	CHAN(5500, 29),
	CHAN(5520, 30),
	CHAN(5540, 31),
	CHAN(5560, 32),
	CHAN(5580, 33),
	CHAN(5600, 34),
	CHAN(5620, 35),
	CHAN(5640, 36),
	CHAN(5660, 37),
	CHAN(5680, 38),
	CHAN(5700, 39),
	CHAN(5745, 40),
	CHAN(5765, 41),
	CHAN(5785, 42),
	CHAN(5805, 43),
	CHAN(5825, 44),
	CHAN(5170, 45),
	CHAN(5190, 46),
	CHAN(5210, 47),
	CHAN(5230, 48),
};
#undef CHAN

static struct ieee80211_supported_band carl9170_band_2GHz = {
	.channels	= carl9170_2ghz_chantable,
	.n_channels	= ARRAY_SIZE(carl9170_2ghz_chantable),
	.bitrates	= carl9170_g_ratetable,
	.n_bitrates	= carl9170_g_ratetable_size,
	.ht_cap		= CARL9170_HT_CAP,
};

static struct ieee80211_supported_band carl9170_band_5GHz = {
	.channels	= carl9170_5ghz_chantable,
	.n_channels	= ARRAY_SIZE(carl9170_5ghz_chantable),
	.bitrates	= carl9170_a_ratetable,
	.n_bitrates	= carl9170_a_ratetable_size,
	.ht_cap		= CARL9170_HT_CAP,
};

#if 1
void carl9170_unregister(struct ar9170 *ar)
{
#if 0
	if (!ar->registered)
		return;

	ar->registered = false;

#ifdef CONFIG_CARL9170_LEDS
	carl9170_led_unregister(ar);
#endif /* CONFIG_CARL9170_LEDS */

#ifdef CONFIG_CARL9170_DEBUGFS
	carl9170_debugfs_unregister(ar);
#endif /* CONFIG_CARL9170_DEBUGFS */

#ifdef CONFIG_CARL9170_WPC
	if (ar->wps.pbc) {
		input_unregister_device(ar->wps.pbc);
		ar->wps.pbc = NULL;
	}
#endif /* CONFIG_CARL9170_WPC */

#ifdef CONFIG_CARL9170_HWRNG
	carl9170_unregister_hwrng(ar);
#endif /* CONFIG_CARL9170_HWRNG */

	carl9170_cancel_worker(ar);
	cancel_work_sync(&ar->restart_work);

	ieee80211_unregister_hw(ar->hw);
#endif
}


static int carl9170_read_eeprom(struct ar9170 *ar)
{
#define RW	8	/* number of words to read at once */
#define RB	(sizeof(u32) * RW)
	u8 *eeprom = (void *)&ar->eeprom;
	__le32 offsets[RW];
	int i, j, err;

	BUILD_BUG_ON(sizeof(ar->eeprom) & 3);

	BUILD_BUG_ON(RB > CARL9170_MAX_CMD_LEN - 4);
#ifndef __CHECKER__
	/* don't want to handle trailing remains */
	BUILD_BUG_ON(sizeof(ar->eeprom) % RB);
#endif

	for (i = 0; i < sizeof(ar->eeprom) / RB; i++) {
		for (j = 0; j < RW; j++)
			offsets[j] = cpu_to_le32(AR9170_EEPROM_START +
						 RB * i + 4 * j);

		err = carl9170_exec_cmd(ar, CARL9170_CMD_RREG,
					RB, (u8 *) &offsets,
					RB, eeprom + RB * i);
		if (err)
			return err;
	}

#undef RW
#undef RB
	return 0;
}

static int carl9170_parse_eeprom(struct ar9170 *ar)
{
	struct ath_regulatory *regulatory = &ar->common.regulatory;
	unsigned int rx_streams, tx_streams, tx_params = 0;
	int bands = 0;
	int chans = 0;

	if (ar->eeprom.length == cpu_to_le16(0xffff))
		return -ENODATA;

	rx_streams = hweight8(ar->eeprom.rx_mask);
	tx_streams = hweight8(ar->eeprom.tx_mask);

	if (rx_streams != tx_streams) {
		tx_params = IEEE80211_HT_MCS_TX_RX_DIFF;

		WARN_ON(!(tx_streams >= 1 && tx_streams <=
			IEEE80211_HT_MCS_TX_MAX_STREAMS));

		tx_params = (tx_streams - 1) <<
			    IEEE80211_HT_MCS_TX_MAX_STREAMS_SHIFT;

		carl9170_band_2GHz.ht_cap.mcs.tx_params |= tx_params;
		carl9170_band_5GHz.ht_cap.mcs.tx_params |= tx_params;
	}

	if (ar->eeprom.operating_flags & AR9170_OPFLAG_2GHZ) {
		ar->hw->wiphy->bands[NL80211_BAND_2GHZ] =
			&carl9170_band_2GHz;
		chans += carl9170_band_2GHz.n_channels;
		bands++;
	}
	if (ar->eeprom.operating_flags & AR9170_OPFLAG_5GHZ) {
		ar->hw->wiphy->bands[NL80211_BAND_5GHZ] =
			&carl9170_band_5GHz;
		chans += carl9170_band_5GHz.n_channels;
		bands++;
	}

	if (!bands)
		return -EINVAL;

	ar->survey = kcalloc(chans, sizeof(struct survey_info), GFP_KERNEL);
	if (!ar->survey)
		return -ENOMEM;
	ar->num_channels = chans;

	regulatory->current_rd = le16_to_cpu(ar->eeprom.reg_domain[0]);

	/* second part of wiphy init */
	SET_IEEE80211_PERM_ADDR(ar->hw, ar->eeprom.mac_address);

	return 0;
}
#endif

int carl9170_register(struct ar9170 *ar)
{
//	struct ath_regulatory *regulatory = &ar->common.regulatory;
	int err = 0, i;

#if 0
	if (WARN_ON(ar->mem_bitmap))
		return -EINVAL;

	ar->mem_bitmap = kcalloc(roundup(ar->fw.mem_blocks, BITS_PER_LONG),
				 sizeof(unsigned long),
				 GFP_KERNEL);

	if (!ar->mem_bitmap)
		return -ENOMEM;
#endif

	/* try to read EEPROM, init MAC addr */
	err = carl9170_read_eeprom(ar);
	if (err)
		return err;

	err = carl9170_parse_eeprom(ar);
	if (err)
		return err;

#if 0
	err = ath_regd_init(regulatory, ar->hw->wiphy,
			    carl9170_reg_notifier);
	if (err)
		return err;

	if (modparam_noht) {
		carl9170_band_2GHz.ht_cap.ht_supported = false;
		carl9170_band_5GHz.ht_cap.ht_supported = false;
	}

	for (i = 0; i < ar->fw.vif_num; i++) {
		ar->vif_priv[i].id = i;
		ar->vif_priv[i].vif = NULL;
	}

	err = ieee80211_register_hw(ar->hw);
	if (err)
		return err;

	/* mac80211 interface is now registered */
	ar->registered = true;

	if (!ath_is_world_regd(regulatory))
		regulatory_hint(ar->hw->wiphy, regulatory->alpha2);

#ifdef CONFIG_CARL9170_DEBUGFS
	carl9170_debugfs_register(ar);
#endif /* CONFIG_CARL9170_DEBUGFS */

	err = carl9170_led_init(ar);
	if (err)
		goto err_unreg;

#ifdef CONFIG_CARL9170_LEDS
	err = carl9170_led_register(ar);
	if (err)
		goto err_unreg;
#endif /* CONFIG_CARL9170_LEDS */

#ifdef CONFIG_CARL9170_WPC
	err = carl9170_register_wps_button(ar);
	if (err)
		goto err_unreg;
#endif /* CONFIG_CARL9170_WPC */

#ifdef CONFIG_CARL9170_HWRNG
	err = carl9170_register_hwrng(ar);
	if (err)
		goto err_unreg;
#endif /* CONFIG_CARL9170_HWRNG */
#endif
	dev_info(&ar->udev->dev, "Atheros AR9170 is registered as '%s'\n",
		 wiphy_name(ar->hw->wiphy));

	return 0;

err_unreg:
	carl9170_unregister(ar);
	return err;
}
