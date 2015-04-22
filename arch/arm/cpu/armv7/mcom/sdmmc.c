/*
 * This code was extracted from:
 * git://github.com/gonzoua/u-boot-pi.git master
 * and hence presumably (C) 2012 Oleksandr Tymoshenko
 *
 * Tweaks for U-Boot upstreaming
 * (C) 2012 Stephen Warren
 *
 * Portions (e.g. read/write macros, concepts for back-to-back register write
 * timing workarounds) obviously extracted from the Linux kernel at:
 * https://github.com/raspberrypi/linux.git rpi-3.6.y
 *
 * The Linux kernel code has the following (c) and license, which is hence
 * propagated to Oleksandr's tree and here:
 *
 * Support for SDHCI device on 2835
 * Based on sdhci-bcm2708.c (c) 2010 Broadcom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Supports:
 * SDHCI platform device - Arasan SD controller in BCM2708
 *
 * Inspired by sdhci-pci.c, by Pierre Ossman
 */

#include <common.h>
#include <malloc.h>
#include <sdhci.h>

/* 400KHz is max freq for card ID etc. Use that as min */
#define MIN_FREQ 400000

int sdmmc_init(u32 regbase, u32 emmc_freq)
{
	struct sdhci_host *host;

	host = malloc(sizeof(*host));
	if (!host) {
		printf("sdhci_host malloc fail!\n");
		return 1;
	}
	host->name = "sdhci";
	host->ioaddr = (void *)regbase;
	host->quirks = SDHCI_QUIRK_BROKEN_VOLTAGE | SDHCI_QUIRK_BROKEN_R1B |
		SDHCI_QUIRK_WAIT_SEND_CMD | SDHCI_QUIRK_NO_HISPD_BIT;
	host->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

	host->version = sdhci_readw(host, SDHCI_HOST_VERSION);
	add_sdhci(host, emmc_freq, MIN_FREQ);

	return 0;
}
