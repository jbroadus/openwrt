/*
 *  NETGEAR WNR2000v5 board support
 *
 *  Copyright (C) 2017 Jim Broadus <jbroadus@gmail.com>
 *  Copyright (C) 2015 Michael Bazzinotti <mbazzinotti@gmail.com>
 *  Copyright (C) 2014 Michaël Burtin <mburtin@gmail.com>
 *  Copyright (C) 2013 Mathieu Olivari <mathieu.olivari@gmail.com>
 *  Copyright (C) 2008-2009 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2008-2009 Andy Boyett <agb@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/74x164.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-spi.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

/* HC164 */
#define  WNR2000V5_GPIO_HC164_RST         14
#define  WNR2000V5_GPIO_HC164_DAT         15
#define  WNR2000V5_GPIO_HC164_CLK         16

/* LEDs on HC164. Note that the 74x164 use reverse order from Netgear's
   original code, which are reflected in the macros below. */
#define HC164_BASE 100
#define HC164_GPIO(n) (HC164_BASE + 7 - n)
#define WNR2000V5_GPIO_LED_PWR            HC164_GPIO(4)
#define WNR2000V5_GPIO_LED_WPS            HC164_GPIO(7)
#define WNR2000V5_GPIO_LED_WLAN           HC164_GPIO(7)
#define WNR2000V5_GPIO_LED_WAN_GREEN      HC164_GPIO(0)
#define WNR2000V5_GPIO_LED_WAN_AMBER      HC164_GPIO(6)
#define WNR2000V5_GPIO_LED_STATUS         HC164_GPIO(5)
/* Buttons */
#define WNR2000V5_GPIO_BTN_WPS            2
#define WNR2000V5_GPIO_BTN_RESET          1
#define WNR2000V5_GPIO_BTN_WLAN           0
#define WNR2000V5_KEYS_POLL_INTERVAL      20      /* msecs */
#define WNR2000V5_KEYS_DEBOUNCE_INTERVAL  (3 * WNR2000V5_KEYS_POLL_INTERVAL)


/* ART offsets */
#define WNR2000V5_MAC0_OFFSET             0       /* WAN/WLAN0 MAC   */
#define WNR2000V5_MAC1_OFFSET             6       /* Eth-switch0 MAC */

static struct spi_gpio_platform_data wnr2000v5_spi_gpio_data = {
        .miso   = SPI_GPIO_NO_MISO,
        .mosi   = WNR2000V5_GPIO_HC164_DAT,
        .sck    = WNR2000V5_GPIO_HC164_CLK,
        .num_chipselect = 1,
};

static struct platform_device wnr2000v5_spi_gpio_device = {
        .name           = "spi_gpio",
        .id             = 1, /* Bus number */
        .dev            = {
                .platform_data = &wnr2000v5_spi_gpio_data,
        }
};

static struct ath79_spi_platform_data wnr2000v5_ath79_spi_data = {
        .bus_num        = 0,
        .num_chipselect = 1,
};

static struct flash_platform_data wnr2000v5_spi_flash_data = {
        .name = "ath-nor0",
        .type = "mx25l3205d",
};

static struct gen_74x164_chip_platform_data wnr2000v5_ssr_data = {
	.base = HC164_BASE,
	.num_registers = 1
};

static struct ath79_spi_controller_data wnr2000v5_spi0_cdata = {
	.cs_type = ATH79_SPI_CS_TYPE_INTERNAL,
	.cs_line = 0,
	.is_flash = true,
};

static struct spi_board_info wnr2000v5_spi_info[] __initdata = {
	{
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 25000000,
		//.modalias	= "spi-nor",
		.modalias	= "m25p80",
		.platform_data  = &wnr2000v5_spi_flash_data,
		.controller_data = &wnr2000v5_spi0_cdata
	},
	{
		.bus_num	= 1,
		.chip_select	= 0,
		.max_speed_hz	= 10000000,
		.modalias	= "74x164",
		.platform_data	= &wnr2000v5_ssr_data,
		.controller_data = (void *)SPI_GPIO_NO_CHIPSELECT
	}
};

static struct gpio_led wnr2000v5_leds_gpio[] __initdata = {
	{
		.name		= "netgear:green:power",
		.gpio		= WNR2000V5_GPIO_LED_PWR,
		.active_low	= 1,
		.default_trigger = "default-on",
	},
	{
		.name		= "netgear:amber:status",
		.gpio		= WNR2000V5_GPIO_LED_STATUS,
		.active_low	= 1,
	},
	{
		.name		= "netgear:green:wan",
		.gpio		= WNR2000V5_GPIO_LED_WAN_GREEN,
		.active_low	= 1,
	},
	{
		.name		= "netgear:amber:wan",
		.gpio		= WNR2000V5_GPIO_LED_WAN_AMBER,
		.active_low	= 1,
	},
	{
		.name		= "netgear:blue:wlan",
		.gpio		= WNR2000V5_GPIO_LED_WLAN,
		.active_low	= 1,
	},
/*
	{
		.name		= "netgear:green:wps",
		.gpio		= WNR2000V5_GPIO_LED_WPS,
		.active_low	= 1,
	},
*/
};

static struct gpio_keys_button wnr2000v5_gpio_keys[] __initdata = {
	{
		.desc		= "WPS button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = WNR2000V5_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= WNR2000V5_GPIO_BTN_WPS,
		.active_low	= 1,
	},
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = WNR2000V5_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= WNR2000V5_GPIO_BTN_RESET,
		.active_low	= 1,
	},
	{
		.desc		= "WLAN button",
		.type		= EV_KEY,
		.code		= KEY_RFKILL,
		.debounce_interval = WNR2000V5_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= WNR2000V5_GPIO_BTN_WLAN,
		.active_low	= 1,
	},
};

static void __init wnr_common_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	u8 *ee  = (u8 *) KSEG1ADDR(0x1fff1000);

	ath79_setup_qca955x_eth_cfg(QCA955X_ETH_CFG_RGMII_EN);

	ath79_register_mdio(0, 0x0);

	ath79_register_usb();

#if 0
        spi_register_board_info(wnr2000v5_spi_info,
                                ARRAY_SIZE(wnr2000v5_spi_info));
#else
	/* This calls spi_register_board_info and platform_device_register */
	ath79_register_spi(&wnr2000v5_ath79_spi_data, wnr2000v5_spi_info,
                           ARRAY_SIZE(wnr2000v5_spi_info));
#endif

	ath79_init_mac(ath79_eth0_data.mac_addr, art+WNR2000V5_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, art+WNR2000V5_MAC1_OFFSET, 0);

	/* WAN port */
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.speed = SPEED_100;
	ath79_eth0_data.duplex = DUPLEX_FULL;
	ath79_eth0_data.phy_mask = BIT(4);
	ath79_register_eth(0);

	/* LAN ports */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_eth1_data.speed = SPEED_100;
	ath79_eth1_data.duplex = DUPLEX_FULL;
	ath79_switch_data.phy_poll_mask |= BIT(4);
	ath79_switch_data.phy4_mii_en = 1;
	ath79_register_eth(1);

	ath79_register_wmac(ee, art);
}

static void __init wnr2000v5_led_setup(void)
{
	/* Set output */
	ath79_gpio_direction_select(WNR2000V5_GPIO_HC164_RST, true);
	ath79_gpio_direction_select(WNR2000V5_GPIO_HC164_DAT, true);
	ath79_gpio_direction_select(WNR2000V5_GPIO_HC164_CLK, true);

	platform_device_register(&wnr2000v5_spi_gpio_device);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(wnr2000v5_leds_gpio),
				 wnr2000v5_leds_gpio);
}

static void __init wnr2000v5_key_setup(void)
{
	int i;
	/* Set input */
	for (i = 0; i < ARRAY_SIZE(wnr2000v5_gpio_keys); i++) {
		ath79_gpio_direction_select(wnr2000v5_gpio_keys[i].gpio, false);
	}

	ath79_register_gpio_keys_polled(-1, WNR2000V5_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(wnr2000v5_gpio_keys),
					wnr2000v5_gpio_keys);
}

static void __init wnr2000v5_setup(void)
{
	wnr_common_setup();

	wnr2000v5_led_setup();

	wnr2000v5_key_setup();
}

MIPS_MACHINE(ATH79_MACH_WNR2000_V5, "WNR2000V5", "NETGEAR WNR2000 V5", wnr2000v5_setup);
