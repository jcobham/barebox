/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * Copyright (C) 2011 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <environment.h>
#include <fcntl.h>
#include <fec.h>
#include <fs.h>
#include <init.h>
#include <nand.h>
#include <net.h>
#include <partition.h>
#include <linux/sizes.h>
#include <gpio.h>

#include <i2c/i2c.h>
#include <generated/mach-types.h>

#include <mach/imx53-regs.h>
#include <mach/iomux-mx53.h>
#include <mach/devices-imx53.h>
#include <mach/generic.h>
#include <mach/imx-nand.h>
#include <mach/iim.h>
#include <mach/imx5.h>
#include <mach/esdctl.h>

#include <asm/armlinux.h>
#include <io.h>
#include <asm/mmu.h>
#include "ccwmx53.h"

static struct ccwmx53_ident ccwmx53_ids[] = {
/* 0x00 - 5500xxxx-xx */	{ "Unknown",						         			0,			0,		0,		0,      0},
/* 0x01 - 5500xxxx-xx */	{ "Not supported",					         			0,			0,		0,		0,      0},
/* 0x02 - 55001604-01 */	{ "i.MX535@1000MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M,	0,		1,		1, 		1},
/* 0x03 - 55001605-01 */	{ "i.MX535@1000MHz, PHY, Accel",						SZ_512M,	0,		1,		0,		0},
/* 0x04 - 55001604-02 */	{ "i.MX535@1000MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M,	0,		1,		1,		1},
/* 0x05 - 5500xxxx-xx */	{ "i.MX535@1000MHz, PHY, Ext. Eth, Accel",				SZ_512M,	0,		1,		1,		0},
/* 0x06 - 55001604-03 */	{ "i.MX535@1000MHz, Wireless, PHY, Accel",				SZ_512M,	0,		1,		0,		1},
/* 0x07 - 5500xxxx-xx */	{ "i.MX535@1000MHz, PHY, Accel",						SZ_512M,	0,		1,		0,		0},
/* 0x08 - 55001604-04 */	{ "i.MX537@800MHz, Wireless, PHY, Accel",				SZ_512M,	1,		1,		0,		1},
/* 0x09 - 55001605-02 */	{ "i.MX537@800MHz, PHY, Accel",							SZ_512M,	1,		1,		0,		0},
/* 0x0a - 5500xxxx-xx */	{ "i.MX537@800MHz, Wireless, PHY, Ext. Eth, Accel",		SZ_512M,	1,		1,		1,		1},
/* 0x0b - 55001605-03 */	{ "i.MX537@800MHz, PHY, Ext. Eth, Accel",				SZ_1G,		1,		1,		1,		0},
/* 0x0c - 5500xxxx-xx */	{ "Reserved for future use",				        	0,     		0,      0,		0,	    0},
/* 0x0d - 55001605-05 */	{ "i.MX537@800MHz, PHY, Accel",							SZ_1G,		1,  	1,		0,		0},
/* 0x0e - 5500xxxx-xx */	{ "Reserved for future use",				         	0,     		0,      0,   	0,		0},
/* 0x0f - 5500xxxx-xx */	{ "Reserved for future use",				        	0,     		0,      0,		0,	    0},
};

struct ccwmx53_ident *ccwmx53_id;

static struct fec_platform_data fec_info = {
	.xcv_type = PHY_INTERFACE_MODE_RMII,
};

static iomux_v3_cfg_t ccwmx53_pads[] = {
	/* UART1 */
	MX53_PAD_PATA_DIOW__UART1_TXD_MUX,
	MX53_PAD_PATA_DMACK__UART1_RXD_MUX,

	/* FEC */
	MX53_PAD_FEC_MDC__FEC_MDC,
	MX53_PAD_FEC_MDIO__FEC_MDIO,
	MX53_PAD_FEC_REF_CLK__FEC_TX_CLK,
	MX53_PAD_FEC_RX_ER__FEC_RX_ER,
	MX53_PAD_FEC_CRS_DV__FEC_RX_DV,
	MX53_PAD_FEC_RXD1__FEC_RDATA_1,
	MX53_PAD_FEC_RXD0__FEC_RDATA_0,
	MX53_PAD_FEC_TX_EN__FEC_TX_EN,
	MX53_PAD_FEC_TXD1__FEC_TDATA_1,
	MX53_PAD_FEC_TXD0__FEC_TDATA_0,
	/* FEC_nRST */
	MX53_PAD_PATA_DA_0__GPIO7_6,

	/* SD2 */
	MX53_PAD_SD2_CMD__ESDHC2_CMD,
	MX53_PAD_SD2_CLK__ESDHC2_CLK,
	MX53_PAD_SD2_DATA0__ESDHC2_DAT0,
	MX53_PAD_SD2_DATA1__ESDHC2_DAT1,
	MX53_PAD_SD2_DATA2__ESDHC2_DAT2,
	MX53_PAD_SD2_DATA3__ESDHC2_DAT3,
	/* SD2_CD */
	MX53_PAD_GPIO_4__GPIO1_4,
	/* SD2_WP */
	MX53_PAD_GPIO_2__GPIO1_2,

	/* SD3 */
	MX53_PAD_PATA_DATA8__ESDHC3_DAT0,
	MX53_PAD_PATA_DATA9__ESDHC3_DAT1,
	MX53_PAD_PATA_DATA10__ESDHC3_DAT2,
	MX53_PAD_PATA_DATA11__ESDHC3_DAT3,
	MX53_PAD_PATA_IORDY__ESDHC3_CLK,
	MX53_PAD_PATA_RESET_B__ESDHC3_CMD,
	
	/* I2C1 */
	MX53_PAD_CSI0_DAT8__I2C1_SDA,
	MX53_PAD_CSI0_DAT9__I2C1_SCL,
	
	/* I2C2 */
	MX53_PAD_KEY_ROW3__I2C2_SDA,
	MX53_PAD_KEY_COL3__I2C2_SCL,
	
	/* I2C3 */
	MX53_PAD_GPIO_6__I2C3_SDA,
	MX53_PAD_GPIO_5__I2C3_SCL,
	
	/* NAND */
	MX53_PAD_PATA_DATA0__EMI_NANDF_D_0,
	MX53_PAD_PATA_DATA1__EMI_NANDF_D_1,
	MX53_PAD_PATA_DATA2__EMI_NANDF_D_2,
	MX53_PAD_PATA_DATA3__EMI_NANDF_D_3,
	MX53_PAD_PATA_DATA4__EMI_NANDF_D_4,
	MX53_PAD_PATA_DATA5__EMI_NANDF_D_5,
	MX53_PAD_PATA_DATA6__EMI_NANDF_D_6,
	MX53_PAD_PATA_DATA7__EMI_NANDF_D_7,
	MX53_PAD_NANDF_CS0__EMI_NANDF_CS_0,
	MX53_PAD_NANDF_RE_B__EMI_NANDF_RE_B,
	MX53_PAD_NANDF_WE_B__EMI_NANDF_WE_B,
	MX53_PAD_NANDF_WP_B__EMI_NANDF_WP_B,
	MX53_PAD_NANDF_ALE__EMI_NANDF_ALE,
	MX53_PAD_NANDF_CLE__EMI_NANDF_CLE,
	MX53_PAD_NANDF_RB0__EMI_NANDF_RB_0,
};

#define ccwmx53_FEC_PHY_RST		IMX_GPIO_NR(7, 6)

static void ccwmx53_fec_reset(void)
{
	gpio_direction_output(ccwmx53_FEC_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(ccwmx53_FEC_PHY_RST, 1);
}

static struct esdhc_platform_data sd2_data = {
	.cd_type = IMX_GPIO_NR(1, 4),
	.wp_type = IMX_GPIO_NR(1, 2),
};

static struct esdhc_platform_data sd3_data = {
	.wp_type = ESDHC_WP_NONE,
	.cd_type = ESDHC_CD_PERMANENT,
};

struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("da9053-i2c", 0x68),
	},
};

/*
 * On this board the SDRAM is always configured for 512Mib. The real
 * size is determined by the board id read from the IIM module.
 */
static int ccxmx53_sdram_fixup(void)
{
	imx_esdctl_disable();

	return 0;
}
postcore_initcall(ccxmx53_sdram_fixup);

static int ccxmx53_memory0_init(void)
{
	arm_add_mem_device("ram0", MX53_CSD0_BASE_ADDR, SZ_512M);

	return 0;
}
mem_initcall(ccxmx53_memory0_init);	

static int ccwmx53_devices_init(void)
{
	u8 hwid[6] = {0};
	char manloc = 0;

	if ((imx_iim_read(1, 9, hwid, sizeof(hwid)) != sizeof(hwid)) || (hwid[0] < 0x02) || (hwid[0] >= ARRAY_SIZE(ccwmx53_ids)))
	{
		printf("Module Variant: Unknown (0x%02x) (0x%02x) (0x%02x) (0x%02x) (0x%02x) (0x%02x)\n", hwid[0],hwid[1],hwid[2],hwid[3],hwid[4],hwid[5]);
		memset(hwid, 0x00, sizeof(hwid));
	}
	
	ccwmx53_id = &ccwmx53_ids[hwid[0]];
	printf("Module Variant: %s (0x%02x)\n", ccwmx53_id->id_string, hwid[0]);

	if (hwid[0]) {
		printf("Module HW Rev : %02x\n", hwid[1] + 1);
		switch (hwid[2] & 0xc0) {
		case 0x00:
			manloc = 'B';
			break;
		case 0x40:
			manloc = 'W';
			break;
		case 0x80:
			manloc = 'S';
			break;
		default:
			manloc = 'N';
			break;
		}
		printf("Module Serial : %c%d\n", manloc, ((hwid[2] & 0x3f) << 24) | (hwid[3] << 16) | (hwid[4] << 8) | hwid[5]);
		if ((ccwmx53_id->mem_sz - SZ_128M) > 0)
		{
			printf("Module RAM    : %d\n", ccwmx53_id->mem_sz);
			arm_add_mem_device("ram0", MX53_CSD0_BASE_ADDR, ccwmx53_id->mem_sz);
		}
	} else {
		return -ENOSYS;
	}
	
	imx53_iim_register_fec_ethaddr();
	imx53_add_fec(&fec_info);
	imx53_add_mmc2(&sd2_data);
	imx53_add_mmc1(&sd3_data);
	imx53_add_i2c2(NULL);
	i2c_register_board_info(2, i2c_devices, ARRAY_SIZE(i2c_devices));
	imx53_add_nand(&nand_info);
	imx53_add_sata();
	ccwmx53_fec_reset();

	armlinux_set_architecture(MACH_TYPE_CCWMX53);

	return 0;
}
device_initcall(ccwmx53_devices_init);

static int ccwmx53_late_init(void)
{

	unsigned char value = 0;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int addr = -1, bus = 0;

	/* I2C2 bus */
	bus = 2;

	/* da9053 device address is 0x68 */
	addr = 0x68;

	adapter = i2c_get_adapter(bus);
	if (!adapter){
		printf("****No Adapter\n");
		return -ENODEV;
	}
	
	client.adapter = adapter;
	client.addr = addr;

	/* Enable 3.3V ext regulator */
	value = 0xfa;
	if (i2c_write_reg(&client, 0x19, &value, 1) < 0){
		printf("****write failed\n");
		return -ENOSYS;
	}
		
	return 0;
}
late_initcall(ccwmx53_late_init);

static int ccwmx53_part_init(void)
{
	const char *envdev;

	switch (bootsource_get()) {
		case BOOTSOURCE_MMC:
			devfs_add_partition("disk0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, "self0");
			devfs_add_partition("disk0", SZ_512K, SZ_1M, DEVFS_PARTITION_FIXED, "env0");
			envdev = "MMC";
			break;
		case BOOTSOURCE_NAND:
		default:
			devfs_add_partition("nand0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, "self_raw");
			dev_add_bb_dev("self_raw", "self0");
			devfs_add_partition("nand0", SZ_512K, SZ_1M, DEVFS_PARTITION_FIXED, "env_raw");
			dev_add_bb_dev("env_raw", "env0");
			envdev = "NAND";
			break;
	}

	printf("Using environment in %s\n", envdev);

	return 0;
}
late_initcall(ccwmx53_part_init);
	
static int ccwmx53_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(ccwmx53_pads, ARRAY_SIZE(ccwmx53_pads));

	barebox_set_model("Digi CCWMX53");
	barebox_set_hostname("ccwmx53");

	imx53_init_lowlevel(1000);

	imx53_add_uart0();

	return 0;
}
console_initcall(ccwmx53_console_init);
