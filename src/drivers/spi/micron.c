/*
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Copyright 2008, Network Appliance Inc.
 * Jason McMullan <mcmullan@netapp.com>
 *
 * Copyright (C) 2004-2007 Freescale Semiconductor, Inc.
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com)
 *
 * Copyright (C) 2014 Sage Electronic Engineering
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdlib.h>
#include <spi_flash.h>
#include "spi_flash_internal.h"

/* N25Q256-specific commands */
#define CMD_MICRON_WREN		0x06	/* Write Enable */
#define CMD_MICRON_WRDI		0x04	/* Write Disable */
#define CMD_MICRON_RDSR		0x05	/* Read Status Register */
#define CMD_MICRON_WRSR		0x01	/* Write Status Register */
#define CMD_MICRON_READ		0x03	/* Read Data Bytes */
#define CMD_MICRON_PP		0x02	/* Page Program */
#define CMD_MICRON_SE		0x20	/* SubSector Erase */
#define CMD_MICRON_BE		0xC7	/* Bulk Erase */
#define CMD_MICRON_DP		0xB9	/* Deep Power-down */
#define CMD_MICRON_RES		0xAB	/* Release from DP, and Read Signature */

#define MICRON_ID_N25Q256	0x19

struct micron_spi_flash_params {
	u8 idcode1;
	u16 page_size;
	u16 pages_per_sector;
	u16 nr_sectors;
	const char *name;
};

/* spi_flash needs to be first so upper layers can free() it */
struct micron_spi_flash {
	struct spi_flash flash;
	const struct micron_spi_flash_params *params;
};

static inline struct micron_spi_flash *to_micron_spi_flash(struct spi_flash
							     *flash)
{
	return container_of(flash, struct micron_spi_flash, flash);
}

static const struct micron_spi_flash_params micron_spi_flash_table[] = {
	{
		.idcode1 = MICRON_ID_N25Q256,
		.page_size = 256,
		.pages_per_sector = 16,
		.nr_sectors = 8192,
		.name = "N25Q256",
	},
};

static int micron_write(struct spi_flash *flash,
			 u32 offset, size_t len, const void *buf)
{
	struct micron_spi_flash *stm = to_micron_spi_flash(flash);
	unsigned long byte_addr;
	unsigned long page_size;
	size_t chunk_len;
	size_t actual;
	int ret;
	u8 cmd[4];

	page_size = min(stm->params->page_size, CONTROLLER_PAGE_LIMIT);
	byte_addr = offset % page_size;

	flash->spi->rw = SPI_WRITE_FLAG;
	ret = spi_claim_bus(flash->spi);
	if (ret) {
		printk(BIOS_WARNING, "SF: Unable to claim SPI bus\n");
		return ret;
	}

	for (actual = 0; actual < len; actual += chunk_len) {
		chunk_len = min(len - actual, page_size - byte_addr);
#ifdef CONFIG_ICH_SPI
		chunk_len = min(chunk_len, CONTROLLER_PAGE_LIMIT);
#endif

		cmd[0] = CMD_MICRON_PP;
		cmd[1] = (offset >> 16) & 0xff;
		cmd[2] = (offset >> 8) & 0xff;
		cmd[3] = offset & 0xff;
#if CONFIG_DEBUG_SPI_FLASH
		printk(BIOS_SPEW, "PP: 0x%p => cmd = { 0x%02x 0x%02x%02x%02x }"
		     " chunk_len = %zu\n",
		     buf + actual, cmd[0], cmd[1], cmd[2], cmd[3], chunk_len);
#endif

		ret = spi_flash_cmd(flash->spi, CMD_MICRON_WREN, NULL, 0);
		if (ret < 0) {
			printk(BIOS_WARNING, "SF: Enabling Write failed\n");
			goto out;
		}

		ret = spi_flash_cmd_write(flash->spi, cmd, 4,
					  buf + actual, chunk_len);
		if (ret < 0) {
			printk(BIOS_WARNING, "SF: Micron Page Program failed\n");
			goto out;
		}

		ret = spi_flash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
		if (ret)
			goto out;

		offset += chunk_len;
		byte_addr = 0;
	}

#if CONFIG_DEBUG_SPI_FLASH
	printk(BIOS_SPEW, "SF: Micron: Successfully programmed %zu bytes @"
			" 0x%lx\n", len, (unsigned long)(offset - len));
#endif
	ret = 0;

out:
	spi_release_bus(flash->spi);
	return ret;
}

static int micron_erase(struct spi_flash *flash, u32 offset, size_t len)
{
	return spi_flash_cmd_erase(flash, CMD_MICRON_SE, offset, len);
}

struct spi_flash *spi_flash_probe_micron(struct spi_slave *spi, u8 * idcode)
{
	const struct micron_spi_flash_params *params;
	struct micron_spi_flash *stm;
	unsigned int i;

	if (idcode[0] == 0xff) {
		i = spi_flash_cmd(spi, CMD_MICRON_RES,
				  idcode, 4);
		if (i)
			return NULL;
		if ((idcode[3] & 0xf0) == 0x10) {
			idcode[0] = 0x20;
			idcode[1] = 0x20;
			idcode[2] = idcode[3] + 1;
		} else
			return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(micron_spi_flash_table); i++) {
		params = &micron_spi_flash_table[i];
		if (params->idcode1 == idcode[2]) {
			break;
		}
	}

	if (i == ARRAY_SIZE(micron_spi_flash_table)) {
		printk(BIOS_WARNING, "SF: Unsupported Micron ID %02x\n", idcode[1]);
		return NULL;
	}

	stm = malloc(sizeof(struct micron_spi_flash));
	if (!stm) {
		printk(BIOS_WARNING, "SF: Failed to allocate memory\n");
		return NULL;
	}

	stm->params = params;
	stm->flash.spi = spi;
	stm->flash.name = params->name;

	stm->flash.write = micron_write;
	stm->flash.erase = micron_erase;
	stm->flash.read = spi_flash_cmd_read_fast;
	stm->flash.sector_size = params->page_size * params->pages_per_sector;
	stm->flash.size = stm->flash.sector_size * params->nr_sectors;

	return &stm->flash;
}
