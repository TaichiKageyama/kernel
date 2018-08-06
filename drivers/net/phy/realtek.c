/*
 * drivers/net/phy/realtek.c
 *
 * Driver for Realtek PHYs
 *
 * Author: Johnson Leung <r58129@freescale.com>
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/bitops.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/of.h>

#define RTL821x_PHYSR				0x11
#define RTL821x_PHYSR_DUPLEX			BIT(13)
#define RTL821x_PHYSR_SPEED			GENMASK(15, 14)

#define RTL821x_INER				0x12
#define RTL8211B_INER_INIT			0x6400
#define RTL8211E_INER_LINK_STATUS		BIT(10)
#define RTL8211F_INER_LINK_STATUS		BIT(4)

#define RTL821x_INSR				0x13

#define RTL821x_PAGE_SELECT			0x1f

#define RTL8211E_EXT_PAGE_SELECT		0x1e
#define RTL8211E_EXT_PAGE			0x0007
#define RTL8211E_LED_EXT_PAGE			0x002c

#define RTL8211E_LACR_ADDR			0x001a
#define RTL8211E_LACR_LED0_ACT_CTRL		BIT(4)
#define RTL8211E_LACR_LED1_ACT_CTRL		BIT(5)
#define RTL8211E_LACR_LED2_ACT_CTRL		BIT(6)

#define RTL8211E_LCR_ADDR			0x001c
#define RTL8211E_LCR_LED0_10			BIT(0)
#define RTL8211E_LCR_LED0_100			BIT(1)
#define RTL8211E_LCR_LED0_1000			BIT(2)
#define RTL8211E_LCR_LED1_10			BIT(4)
#define RTL8211E_LCR_LED1_100			BIT(5)
#define RTL8211E_LCR_LED1_1000			BIT(6)
#define RTL8211E_LCR_LED2_10			BIT(8)
#define RTL8211E_LCR_LED2_100			BIT(9)
#define RTL8211E_LCR_LED2_1000			BIT(10)

#define RTL8211F_INSR				0x1d

#define RTL8211F_TX_DELAY			BIT(8)

#define RTL8211F_LED_EXT_PAGE			0x0d04

#define RTL8211F_LCR_ADDR			0x0010
#define RTL8211F_LCR_LED0_LINK_10		BIT(0)
#define RTL8211F_LCR_LED0_LINK_100		BIT(1)
#define RTL8211F_LCR_LED0_LINK_1000		BIT(3)
#define RTL8211F_LCR_LED0_ACT			BIT(4)
#define RTL8211F_LCR_LED1_LINK_10		BIT(5)
#define RTL8211F_LCR_LED1_LINK_100		BIT(6)
#define RTL8211F_LCR_LED1_LINK_1000		BIT(8)
#define RTL8211F_LCR_LED1_ACT			BIT(9)
#define RTL8211F_LCR_LED2_LINK_10		BIT(10)
#define RTL8211F_LCR_LED2_LINK_100		BIT(11)
#define RTL8211F_LCR_LED2_LINK_1000		BIT(13)
#define RTL8211F_LCR_LED2_ACT			BIT(14)

#define RTL8211F_EEELCR_ADDR			0x0011
#define RTL8211F_LCR_LED0_EEE			BIT(1)
#define RTL8211F_LCR_LED1_EEE			BIT(2)
#define RTL8211F_LCR_LED2_EEE			BIT(3)

#define RTL8201F_ISR				0x1e
#define RTL8201F_IER				0x13

MODULE_DESCRIPTION("Realtek PHY driver");
MODULE_AUTHOR("Johnson Leung");
MODULE_LICENSE("GPL");

static int rtl821x_read_page(struct phy_device *phydev)
{
	return __phy_read(phydev, RTL821x_PAGE_SELECT);
}

static int rtl821x_write_page(struct phy_device *phydev, int page)
{
	return __phy_write(phydev, RTL821x_PAGE_SELECT, page);
}

static void rtl8211e_setup_led(struct phy_device *phydev)
{
	const char *model;
	u32 reg_lacr = 0;
	u32 reg_lcr = 0;

	model = of_get_property(of_find_node_by_path("/"), "model", NULL);
	if (strncmp(model, "Rockchip RK3288 Tinker Board", 28) == 0){
		pr_debug("%s: Set up LED for %s\n", __FUNCTION__, model);

		/* LED0(Gree ACT),LED1(Green Link1000),LED2(Orange:Link100) */
		reg_lacr= RTL8211E_LACR_LED0_ACT_CTRL;
		reg_lcr= RTL8211E_LCR_LED0_10 |
			RTL8211E_LCR_LED0_100 |
			RTL8211E_LCR_LED0_1000 |
			RTL8211E_LCR_LED1_1000 |
			RTL8211E_LCR_LED2_100;
	}

	if(reg_lacr == 0 && reg_lcr == 0)
		return;

	/* Switch to ext page */
	phy_write(phydev, RTL821x_PAGE_SELECT, RTL8211E_EXT_PAGE);
	/* Switch to ext page 44 */
	phy_write(phydev, RTL8211E_EXT_PAGE_SELECT, RTL8211E_LED_EXT_PAGE);
	/* Set up LED */
	phy_write(phydev, RTL8211E_LACR_ADDR, reg_lacr);
	pr_debug("%s: Wrote 0x%x to LACR\n", __FUNCTION__, reg_lacr);
	phy_write(phydev, RTL8211E_LCR_ADDR, reg_lcr);
	pr_debug("%s: Wrote 0x%x to LCR\n", __FUNCTION__, reg_lcr);
	/* Switch to Page 0 */

	return;
}

static void rtl8211f_setup_led(struct phy_device *phydev)
{
	const char *model;
	u32 reg_lcr = 0;
	u32 reg_eeelcr = 0;

	model = of_get_property(of_find_node_by_path("/"), "model", NULL);
	if (strncmp(model, "Pine64 Rock64", 13) == 0){
		pr_debug("%s: Set up LED for %s\n", __FUNCTION__, model);

		/* Configure EEE LED Function */
		reg_eeelcr = RTL8211F_LCR_LED0_EEE |
			RTL8211F_LCR_LED1_EEE |
			RTL8211F_LCR_LED2_EEE;
		/*
		 * Configure LED Function
		 *	LED0(N/A), LED1(Green:GreeACT), LED2(Orange:OrangeACT)
		 */
		reg_lcr = RTL8211F_LCR_LED1_LINK_10 |
			RTL8211F_LCR_LED1_LINK_100 |
			RTL8211F_LCR_LED1_LINK_1000 |
			RTL8211F_LCR_LED1_ACT |
			RTL8211F_LCR_LED2_LINK_1000;
	}

	if(reg_lcr == 0 && reg_eeelcr == 0)
		return;

	/* Set up EEELCR */
	phy_write_paged(phydev, RTL8211F_LED_EXT_PAGE,
		RTL8211F_EEELCR_ADDR, reg_eeelcr);
	/* Setup LCR */
	phy_write_paged(phydev, RTL8211F_LED_EXT_PAGE,
		RTL8211F_LCR_ADDR, reg_lcr);
}

static int rtl8201_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, RTL8201F_ISR);

	return (err < 0) ? err : 0;
}

static int rtl821x_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, RTL821x_INSR);

	return (err < 0) ? err : 0;
}

static int rtl8211f_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read_paged(phydev, 0xa43, RTL8211F_INSR);

	return (err < 0) ? err : 0;
}

static int rtl8201_config_intr(struct phy_device *phydev)
{
	u16 val;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		val = BIT(13) | BIT(12) | BIT(11);
	else
		val = 0;

	return phy_write_paged(phydev, 0x7, RTL8201F_IER, val);
}

static int rtl8211b_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL8211B_INER_INIT);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

static int rtl8211e_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL8211E_INER_LINK_STATUS);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

static int rtl8211e_config_init(struct phy_device *phydev)
{
	/* Setup phy LED if needed */
	rtl8211e_setup_led(phydev);

	return 0;
}

static int rtl8211f_config_intr(struct phy_device *phydev)
{
	u16 val;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		val = RTL8211F_INER_LINK_STATUS;
	else
		val = 0;

	return phy_write_paged(phydev, 0xa42, RTL821x_INER, val);
}

static int rtl8211f_config_init(struct phy_device *phydev)
{
	int ret;
	u16 val = 0;

	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

	/* Setup phy LED if needed */
	rtl8211f_setup_led(phydev);

	/* enable TX-delay for rgmii-id and rgmii-txid, otherwise disable it */
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
		val = RTL8211F_TX_DELAY;

	return phy_modify_paged(phydev, 0xd08, 0x11, RTL8211F_TX_DELAY, val);
}

static int rtl8211b_suspend(struct phy_device *phydev)
{
	phy_write(phydev, MII_MMD_DATA, BIT(9));

	return genphy_suspend(phydev);
}

static int rtl8211b_resume(struct phy_device *phydev)
{
	phy_write(phydev, MII_MMD_DATA, 0);

	return genphy_resume(phydev);
}

static struct phy_driver realtek_drvs[] = {
	{
		.phy_id         = 0x00008201,
		.name           = "RTL8201CP Ethernet",
		.phy_id_mask    = 0x0000ffff,
		.features       = PHY_BASIC_FEATURES,
		.flags          = PHY_HAS_INTERRUPT,
	}, {
		.phy_id		= 0x001cc816,
		.name		= "RTL8201F 10/100Mbps Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_BASIC_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.ack_interrupt	= &rtl8201_ack_interrupt,
		.config_intr	= &rtl8201_config_intr,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		.phy_id		= 0x001cc912,
		.name		= "RTL8211B Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.ack_interrupt	= &rtl821x_ack_interrupt,
		.config_intr	= &rtl8211b_config_intr,
		.read_mmd	= &genphy_read_mmd_unsupported,
		.write_mmd	= &genphy_write_mmd_unsupported,
		.suspend	= rtl8211b_suspend,
		.resume		= rtl8211b_resume,
	}, {
		.phy_id		= 0x001cc914,
		.name		= "RTL8211DN Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.ack_interrupt	= rtl821x_ack_interrupt,
		.config_intr	= rtl8211e_config_intr,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
	}, {
		.phy_id		= 0x001cc915,
		.name		= "RTL8211E Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.config_init    = &rtl8211e_config_init,
		.ack_interrupt	= &rtl821x_ack_interrupt,
		.config_intr	= &rtl8211e_config_intr,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
	}, {
		.phy_id		= 0x001cc916,
		.name		= "RTL8211F Gigabit Ethernet",
		.phy_id_mask	= 0x001fffff,
		.features	= PHY_GBIT_FEATURES,
		.flags		= PHY_HAS_INTERRUPT,
		.config_init	= &rtl8211f_config_init,
		.ack_interrupt	= &rtl8211f_ack_interrupt,
		.config_intr	= &rtl8211f_config_intr,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	},
};

module_phy_driver(realtek_drvs);

static struct mdio_device_id __maybe_unused realtek_tbl[] = {
	{ 0x001cc816, 0x001fffff },
	{ 0x001cc912, 0x001fffff },
	{ 0x001cc914, 0x001fffff },
	{ 0x001cc915, 0x001fffff },
	{ 0x001cc916, 0x001fffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, realtek_tbl);
