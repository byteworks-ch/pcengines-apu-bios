/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
 * Copyright (C) 2014 Sage Electronic Engineering, LLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <console/console.h>
#include <device/pci.h>
#include <arch/io.h>
#include <string.h>
#include "cimx_util.h"
#include <pc80/i8259.h>

#ifndef __PRE_RAM__
#if IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_CIMX_SB800) || \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_CIMX_SB900)
const char * intr_types[] = {
	[0x00] = "INTA#\t", "INTB#\t", "INTC#\t", "INTD#\t", "INTE#\t", "INTF#\t", "INTG#\t", "INTH#\t",
	[0x08] = "Misc\t", "Misc0\t", "Misc1\t", "Misc2\t", "Ser IRQ INTA", "Ser IRQ INTB", "Ser IRQ INTC", "Ser IRQ INTD",
	[0x10] = "SCI\t", "SMBUS0\t", "ASF\t", "HDA\t", "FC\t\t", "GEC\t", "PerMon\t",
	[0x20] = "IMC INT0\t", "IMC INT1\t", "IMC INT2\t", "IMC INT3\t", "IMC INT4\t", "IMC INT5\t",
	[0x30] = "Dev18.0 INTA", "Dev18.2 INTB", "Dev19.0 INTA", "Dev19.2 INTB", "Dev22.0 INTA", "Dev22.2 INTB", "Dev20.5 INTC",
	[0x40] = "IDE\t", "SATA\t",
	[0x50] = "GPPInt0\t", "GPPInt1\t", "GPPInt2\t", "GPPInt3\t"
};
#elif IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_CIMX_SB700)
const char * intr_types[] = {
	[0x00] = "INTA#\t", "INTB#\t", "INTC#\t", "INTD#\t",
	[0x04] = "ACPI\t", "SMBUS\t", "RSVD\t", "RSVD\t", "RSVD\t",
	[0x09] = "INTE#\t", "INTF#\t", "INTG#\t", "INTH#\t",
};
#endif

const struct pirq_struct * pirq_data_ptr = NULL;
u32 pirq_data_size = 0;
const u8 * intr_data_ptr = NULL;
const u8 * picr_data_ptr = NULL;

/*
 * Read the FCH PCI_INTR registers 0xC00/0xC01 at a
 * given index and a given PIC (0) or IOAPIC (1) mode
 */
u8 read_pci_int_idx(u8 index, int mode)
{
	outb((mode << 7) | index, PCI_INTR_INDEX);
	return inb(PCI_INTR_DATA);
}

/*
 * Write a value to the FCH PCI_INTR registers 0xC00/0xC01
 * at a given index and PIC (0) or IOAPIC (1) mode
 */
void write_pci_int_idx(u8 index, int mode, u8 data)
{
	outb((mode << 7) | index, PCI_INTR_INDEX);
	outb(data, PCI_INTR_DATA);
}

/*
 * Write the FCH PCI_INTR registers 0xC00/0xC01 with values
 * given in global variables intr_data and picr_data.
 * These variables are defined in mainboard.c
 */
void write_pci_int_table (void)
{
	u8 byte;

	if(picr_data_ptr == NULL || intr_data_ptr == NULL){
		printk(BIOS_ERR, "Warning: Can't write PCI_INTR 0xC00/0xC01 registers because\n"
				"'mainboard_picr_data' or 'mainboard_intr_data' tables are NULL\n");
		return;
	}

	/* PIC IRQ routine */
	printk(BIOS_DEBUG, "PCI_INTR tables: Writing registers C00/C01 for PIC mode PCI IRQ routing:\n"
			"\tPCI_INTR_INDEX\t\tPCI_INTR_DATA\n");
	for (byte = 0; byte < FCH_INT_TABLE_SIZE; byte++) {
		if (intr_types[byte]) {
			write_pci_int_idx(byte, 0, (u8) picr_data_ptr[byte]);
			printk(BIOS_DEBUG, "\t0x%02X %s\t: 0x%02X\n",
					byte, intr_types[byte], read_pci_int_idx(byte, 0));
		}
	}

	/* APIC IRQ routine */
	printk(BIOS_DEBUG, "PCI_INTR tables: Writing registers C00/C01 for APIC mode PCI IRQ routing:\n"
			"\tPCI_INTR_INDEX\t\tPCI_INTR_DATA\n");
	for (byte = 0; byte < FCH_INT_TABLE_SIZE; byte++) {
		if (intr_types[byte]) {
			write_pci_int_idx(byte, 1, (u8) intr_data_ptr[byte]);
			printk(BIOS_DEBUG, "\t0x%02X %s\t: 0x%02X\n",
					byte, intr_types[byte], read_pci_int_idx(byte, 1));
		}
	}
}

/*
 * Function to write the PCI config space Interrupt
 * registers based on the values given in PCI_INTR
 * table at I/O port 0xC00/0xC01
 */
void write_pci_cfg_irqs(void)
{
	device_t dev = NULL;		/* Our current device to route IRQs to */
	device_t target_dev = NULL;	/* The bridge that a device may be connected to */
	int int_pin = 0;	/* Value of the INT_PIN register 0x3D */
	int target_pin = 0;	/* Pin we will search our tables for */
	int int_line = 0;	/* IRQ number read from PCI_INTR table and programmed to INT_LINE reg 0x3C */
	int pci_intr_idx = 0;	/* Index into PCI_INTR table, 0xC00/0xC01 */
	int bus = 0;		/* A PCI Device Bus number */
	int devfn = 0;		/* A PCI Device and Function number */
	int bridged_device = 0;	/* This device is on a PCI bridge */
	int i = 0;

	if (pirq_data_ptr == NULL) {
		printk(BIOS_WARNING, "Warning: Can't write PCI IRQ assignments because"
				" 'mainboard_pirq_data' structure does not exist\n");
		return;
	}

	/* Populate the PCI cfg space header with the IRQ assignment */
	printk(BIOS_DEBUG, "PCI_CFG IRQ: Write PCI config space IRQ assignments\n");

	for (dev = all_devices; dev; dev = dev->next) {
		/*
		 * Step 1: Get the INT_PIN and device structure to look for in the
		 * PCI_INTR table pirq_data
		 */
		target_dev = NULL;
		target_pin = get_irq_pins(dev, &target_dev);
		if (target_dev == NULL)
			continue;

		if (target_pin < 1)
			continue;

		/* Get the original INT_PIN for record keeping */
		int_pin = pci_read_config8(dev, PCI_INTERRUPT_PIN);

		bus   = target_dev->bus->secondary;
		devfn = target_dev->path.pci.devfn;

		/*
		 * Step 2: Use the INT_PIN and DevFn number to find the PCI_INTR
		 * register (0xC00) index for this device
		 */
		pci_intr_idx = 0xBAD;	/* Will check to make sure it changed */
		for (i = 0; i <= pirq_data_size - 1; i++) {
			if (pirq_data_ptr[i].devfn != devfn)
				continue;

			/* PIN_A is index 0 in pirq_data array but 1 in PCI cfg reg */
			pci_intr_idx = pirq_data_ptr[i].PIN[target_pin - 1];
			printk(BIOS_SPEW, "\tFound this device in pirq_data table entry %d\n", i);
			break;
		}

		/*
		 * Step 3: Make sure we got a valid index and use it to get
		 * the IRQ number from the PCI_INTR register table
		 */
		if (pci_intr_idx == 0xBAD) {	/* Not on a bridge or in pirq_data table, skip it */
			printk(BIOS_SPEW, "PCI Devfn (0x%x) not found in pirq_data table\n", devfn);
			continue;
		} else if (pci_intr_idx == 0x1F) {	/* Index found is not defined */
			printk(BIOS_SPEW, "Got index 0x1F (Not Connected), perhaps this device was defined wrong?\n");
			continue;
		} else if (pci_intr_idx > FCH_INT_TABLE_SIZE) {	/* Index out of bounds */
			printk(BIOS_ERR, "%s: got 0xC00/0xC01 table index 0x%x, max is 0x%x\n",
					__func__, pci_intr_idx, FCH_INT_TABLE_SIZE);
			continue;
		}

		/* Find the value to program into the INT_LINE register from the PCI_INTR registers */
		int_line = read_pci_int_idx(pci_intr_idx, 0);
		if (int_line == PIRQ_NC) {	/* The IRQ found is not disabled */
			printk(BIOS_SPEW, "Got IRQ 0x1F (disabled), perhaps this device was defined wrong?\n");
			continue;
		} else if ((1 << int_line) & IRQ_RES) {	/* Found an IRQ that is reserved */
			printk(BIOS_WARNING, "WARNING: PCI IRQ %d is reserved, check the mainboard_picr_data table\n"
					"Skip writing it to PCI config space to prevent instability\n", int_line);
			continue;
		}

		/*
		 * Step 4: Program the INT_LINE register in this device's
		 * PCI config space with the IRQ number we found in step 3
		 * and make it Level Triggered
		 */
		pci_write_config8(dev, PCI_INTERRUPT_LINE, int_line);

		/* Set this IRQ to level triggered since it is used by a PCI device */
		i8259_configure_irq_trigger(int_line, IRQ_LEVEL_TRIGGERED);

		/*
		 * Step 5: Print out debug info and move on to next device
		 */
		printk(BIOS_SPEW, "\tOrig INT_PIN\t: %d (%s)\n",
						int_pin, pin_to_str(int_pin));
		if (bridged_device)
			printk(BIOS_SPEW, "\tSwizzled to\t: %d (%s)\n",
							target_pin, pin_to_str(target_pin));
		printk(BIOS_SPEW, "\tPCI_INTR idx\t: 0x%02x (%s)\n"
						"\tINT_LINE\t: 0x%X (IRQ %d)\n",
						pci_intr_idx, intr_types[pci_intr_idx], int_line, int_line);
	}	/* for (dev = all_devices) */
	printk(BIOS_DEBUG, "PCI_CFG IRQ: Finished writing PCI config space IRQ assignments\n");
}
#endif /* __PRE_RAM__ */

static void pmio_write_index(u16 port_base, u8 reg, u8 value)
{
	outb(reg, port_base);
	outb(value, port_base + 1);
}

static u8 pmio_read_index(u16 port_base, u8 reg)
{
	outb(reg, port_base);
	return inb(port_base + 1);
}

void pm_iowrite(u8 reg, u8 value)
{
	pmio_write_index(PM_INDEX, reg, value);
}

u8 pm_ioread(u8 reg)
{
	return pmio_read_index(PM_INDEX, reg);
}

void pm2_iowrite(u8 reg, u8 value)
{
	pmio_write_index(PM2_INDEX, reg, value);
}

u8 pm2_ioread(u8 reg)
{
	return pmio_read_index(PM2_INDEX, reg);
}
