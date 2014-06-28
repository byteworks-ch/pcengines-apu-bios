/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Sage Electronic Engineering, LLC
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#define __SIMPLE_DEVICE__

#include <stdint.h>
#include <arch/io.h>
#include <bios_reset.h>

#define	HT_INIT_CONTROL 0x6c
#define HTIC_BIOSR_Detect  (1<<5)

void __attribute__ ((weak)) set_bios_reset(void)
{
	u32 htic;
	htic = pci_read_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL);
	htic &= ~HTIC_BIOSR_Detect;
	pci_write_config32(PCI_DEV(0, 0x18, 0), HT_INIT_CONTROL, htic);
}
