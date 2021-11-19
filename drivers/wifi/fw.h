#ifndef _RTL8188_FW_H_
#define _RTL8188_FW_H_

#define RTL8188_FW_NAME       "rtl8188.fw"
#define CONFIG_EMBEDDED_FWIMG
#define FW_8710B_SIZE           0x8000
#define FW_8710B_START_ADDRESS  0x1000
#define FW_8710B_END_ADDRESS    0x1FFF	/* 0x5FFF */

#define MAX_DLFW_PAGE_SIZE                      4096	/* @ page : 4k bytes */
typedef enum _FIRMWARE_SOURCE {
	FW_SOURCE_IMG_FILE = 0,
	FW_SOURCE_HEADER_FILE = 1,	/* from header file */
} FIRMWARE_SOURCE, *PFIRMWARE_SOURCE;

#define FW_8710B_SIZE         0x8000

typedef struct _RT_FIRMWARE {
	FIRMWARE_SOURCE eFWSource;
#ifdef CONFIG_EMBEDDED_FWIMG
	u8 *szFwBuffer;
#else
	u8 szFwBuffer[FW_8710B_SIZE];
#endif
	u32 ulFwLength;
} RT_FIRMWARE_8710B, *PRT_FIRMWARE_8710B;

  /* Added by tynli. 2009.12.04. */
typedef struct _RT_8710B_FIRMWARE_HDR {
	/* 8-byte alinment required */

	/* --- LONG WORD 0 ---- */
	u16 Signature;		/* 92C0: test chip; 92C, 88C0: test chip; 88C1: MP A-cut; 92C1: MP A-cut */
	u8 Category;		/* AP/NIC and USB/PCI */
	u8 Function;		/* Reserved for different FW function indcation, for further use when driver needs to download different FW in different conditi */
	u16 Version;		/* FW Version */
	u16 Subversion;		/* FW Subversion, default 0x00 */

	/* --- LONG WORD 1 ---- */
	u8 Month;		/* Release time Month field */
	u8 Date;		/* Release time Date field */
	u8 Hour;		/* Release time Hour field */
	u8 Minute;		/* Release time Minute field */
	u16 RamCodeSize;	/* The size of RAM code */
	u16 Rsvd2;

	/* --- LONG WORD 2 ---- */
	u32 SvnIdx;		/* The SVN entry index */
	u32 Rsvd3;

	/* --- LONG WORD 3 ---- */
	u32 Rsvd4;
	u32 Rsvd5;
} RT_8710B_FIRMWARE_HDR, *PRT_8710B_FIRMWARE_HDR;

/* HAL_IC_TYPE_E */
typedef enum tag_HAL_IC_Type_Definition {
	CHIP_8192S = 0,
	CHIP_8188C = 1,
	CHIP_8192C = 2,
	CHIP_8192D = 3,
	CHIP_8723A = 4,
	CHIP_8188E = 5,
	CHIP_8812 = 6,
	CHIP_8821 = 7,
	CHIP_8723B = 8,
	CHIP_8192E = 9,
	CHIP_8814A = 10,
	CHIP_8703B = 11,
	CHIP_8188F = 12,
	CHIP_8822B = 13,
	CHIP_8723D = 14,
	CHIP_8821C = 15,
	CHIP_8710B = 16
} HAL_IC_TYPE_E;

/* HAL_CHIP_TYPE_E */
typedef enum tag_HAL_CHIP_Type_Definition {
	TEST_CHIP = 0,
	NORMAL_CHIP = 1,
	FPGA = 2,
} HAL_CHIP_TYPE_E;

/* HAL_CUT_VERSION_E */
typedef enum tag_HAL_Cut_Version_Definition {
	A_CUT_VERSION = 0,
	B_CUT_VERSION = 1,
	C_CUT_VERSION = 2,
	D_CUT_VERSION = 3,
	E_CUT_VERSION = 4,
	F_CUT_VERSION = 5,
	G_CUT_VERSION = 6,
	H_CUT_VERSION = 7,
	I_CUT_VERSION = 8,
	J_CUT_VERSION = 9,
	K_CUT_VERSION = 10,
} HAL_CUT_VERSION_E;

/* HAL_Manufacturer */
typedef enum tag_HAL_Manufacturer_Version_Definition {
	CHIP_VENDOR_TSMC = 0,
	CHIP_VENDOR_UMC = 1,
	CHIP_VENDOR_SMIC = 2,
} HAL_VENDOR_E;

typedef enum tag_HAL_RF_Type_Definition {
	RF_TYPE_1T1R = 0,
	RF_TYPE_1T2R = 1,
	RF_TYPE_2T2R = 2,
	RF_TYPE_2T3R = 3,
	RF_TYPE_2T4R = 4,
	RF_TYPE_3T3R = 5,
	RF_TYPE_3T4R = 6,
	RF_TYPE_4T4R = 7,
} HAL_RF_TYPE_E;

typedef struct tag_HAL_VERSION {
	HAL_IC_TYPE_E ICType;
	HAL_CHIP_TYPE_E ChipType;
	HAL_CUT_VERSION_E CUTVersion;
	HAL_VENDOR_E VendorType;
	HAL_RF_TYPE_E RFType;
	u8 ROMVer;
} HAL_VERSION, *PHAL_VERSION;

extern u8 array_mp_8710b_fw_ap[21240];
extern u32 array_length_mp_8710b_fw_ap;

extern u8 array_mp_8710b_fw_nic[23750];
extern u32 array_length_mp_8710b_fw_nic;

extern u8 array_mp_8710b_fw_wowlan[25666];
extern u32 array_length_mp_8710b_fw_wowlan;

#endif
