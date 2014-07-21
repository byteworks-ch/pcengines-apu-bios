/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC
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
#include <device/device.h>
#include <device/pci.h>
#include <arch/io.h>
#include <cpu/x86/msr.h>
#include <device/pci_def.h>
#include <southbridge/amd/cimx/cimx_util.h>
#include <arch/acpi.h>
#include "BiosCallOuts.h"
#include <cpu/amd/agesa/s3_resume.h>
#include <cpu/amd/mtrr.h>
#include "SBPLATFORM.h"
#include <delay.h>
#include <string.h>
#include <cbfs.h>
#include <cbmem.h>
#include <southbridge/amd/cimx/sb800/pci_devs.h>
#include <northbridge/amd/agesa/family14/pci_devs.h>
#include "gpio_ftns.h"
#include <pc80/mc146818rtc.h>

void set_pcie_reset(void);
void set_pcie_dereset(void);
const char *smbios_mainboard_serial_number(void);
const char *smbios_mainboard_sku(void);

char tmp[10];
int cmos_console_setting = 0;

/***********************************************************
 * These arrays set up the FCH PCI_INTR registers 0xC00/0xC01.
 * This table is responsible for physically routing the PIC and
 * IOAPIC IRQs to the different PCI devices on the system.  It
 * is read and written via registers 0xC00/0xC01 as an
 * Index/Data pair.  These values are chipset and mainboard
 * dependent and should be updated accordingly.
 *
 * These values are used by the PCI configuration space,
 * MP Tables.  TODO: Make ACPI use these values too.
 *
 * The PCI INTA/B/C/D pins are connected to
 * FCH pins INTE/F/G/H on the schematic so these need
 * to be routed as well.
 */
u8 mainboard_picr_data[FCH_INT_TABLE_SIZE] = {
	[0x00] = 0x0A,0x0B,0x0A,0x0B,0x0A,0x0B,0x0A,0x0B,	/* INTA# - INTH# */
	[0x08] = 0x00,0xF1,0x00,0x00,0x1F,0x1F,0x1F,0x1F,	/* Misc-nil,0,1,2, INT from Serial irq */
	[0x10] = 0x1F,0x1F,0x1F,0x0A,0x1F,0x1F,0x1F,		/* SCI, SMBUS0, ASF, HDA, FC, GEC, PerMon */
	[0x20] = 0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,				/* IMC INT0 - 5 */
	[0x30] = 0x0A,0x0B,0x0A,0x0B,0x1F,0x1F,0x0A,		/* USB Devs 18/19/20/22 INTA-C */
	[0x40] = 0x0B,0x0B,									/* IDE, SATA */
	[0x50] = 0x0A,0x0B,0x0A,0x0B						/* GPPInt0 - 3 */
};

u8 mainboard_intr_data[FCH_INT_TABLE_SIZE] = {
	[0x00] = 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* INTA# - INTH# */
	[0x08] = 0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,	/* Misc-nil,0,1,2, INT from Serial irq */
	[0x10] = 0x09,0x1F,0x1F,0x10,0x1F,0x12,0x1F,		/* SCI, SMBUS0, ASF, HDA, FC, GEC, PerMon */
	[0x20] = 0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,				/* IMC INT0 - 5 */
	[0x30] = 0x12,0x11,0x12,0x11,0x12,0x11,0x12,		/* USB Devs 18/19/22/20 INTA-C */
	[0x40] = 0x11,0x13,									/* IDE, SATA */
	[0x50] = 0x10,0x11,0x12,0x13						/* GPPInt0 - 3 */
};

/*
 * This table defines the index into the picr/intr_data
 * tables for each device.  Any enabled device and slot
 * that uses hardware interrupts should have an entry
 * in this table to define its index into the FCH
 * PCI_INTR register 0xC00/0xC01.  This index will define
 * the interrupt that it should use.  Putting PIRQ_A into
 * the PIN A index for a device will tell that device to
 * use PIC IRQ 10 if it uses PIN A for its hardware INT.
 */
/*
 * The PCI slot INTA/B/C/D connected to PIRQE/F/G/H
 * but because of PCI INT_PIN swizzle isnt implemented to match
 * the IDSEL (dev 3) of the slot, the table is adjusted for the
 * swizzle and INTA is connected to PIRQH so PINA/B/C/D on
 * off-chip devices should get mapped to PIRQH/E/F/G.
 */
static const struct pirq_struct mainboard_pirq_data[] = {
	/* {PCI_devfn,        {PIN A, PIN B, PIN C, PIN D}}, */
	{GFX_DEVFN,           {PIRQ_A, PIRQ_B, PIRQ_NC, PIRQ_NC}},      /* VGA:       01.0 */
	{NB_PCIE_PORT1_DEVFN, {PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D}},        /* NIC:       04.0 */
	{NB_PCIE_PORT2_DEVFN, {PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D}},        /* NIC:       05.0 */
	{NB_PCIE_PORT3_DEVFN, {PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D}},        /* NIC:       06.0 */
	{NB_PCIE_PORT4_DEVFN, {PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D}},        /* miniPCIe:  07.0 */
	{SATA_DEVFN,          {PIRQ_SATA, PIRQ_NC, PIRQ_NC, PIRQ_NC}},  /* SATA:      11.0 */
	{OHCI1_DEVFN,         {PIRQ_OHCI1, PIRQ_NC, PIRQ_NC, PIRQ_NC}}, /* OHCI1:     18.0 */
	{EHCI1_DEVFN,         {PIRQ_NC, PIRQ_EHCI1, PIRQ_NC, PIRQ_NC}}, /* EHCI1:     18.2 */
	{OHCI2_DEVFN,         {PIRQ_OHCI2, PIRQ_NC, PIRQ_NC, PIRQ_NC}}, /* OHCI2:     19.0 */
	{EHCI2_DEVFN,         {PIRQ_NC, PIRQ_EHCI2, PIRQ_NC, PIRQ_NC}}, /* EHCI2:     19.2 */
	{SMBUS_DEVFN,         {PIRQ_SMBUS, PIRQ_NC, PIRQ_NC, PIRQ_NC}}, /* SMBUS:     20.0 */
	{IDE_DEVFN,           {PIRQ_NC, PIRQ_IDE, PIRQ_NC, PIRQ_NC}},   /* IDE:       20.1 */
	{HDA_DEVFN,           {PIRQ_HDA, PIRQ_NC, PIRQ_NC, PIRQ_NC}},   /* HDA:       20.2 */
	{SB_PCI_PORT_DEVFN,   {PIRQ_H, PIRQ_E, PIRQ_F, PIRQ_G}},        /* PCI bdg:   20.4 */
	{OHCI4_DEVFN,         {PIRQ_NC, PIRQ_NC, PIRQ_OHCI4, PIRQ_NC}}, /* OHCI4:     20.5 */
	{SB_PCIE_PORT1_DEVFN, {PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D}},        /* miniPCIe:  21.0 */
	{OHCI3_DEVFN,         {PIRQ_OHCI3, PIRQ_NC, PIRQ_NC, PIRQ_NC}}, /* OHCI3:     22.0 */
	{EHCI3_DEVFN,         {PIRQ_NC, PIRQ_EHCI3, PIRQ_NC, PIRQ_NC}}, /* EHCI3:     22.2 */
};

/* PIRQ Setup */
extern const struct pirq_struct * pirq_data_ptr;
extern u32 pirq_data_size;
extern u8 * intr_data_ptr;
extern u8 * picr_data_ptr;

static void pirq_setup(void)
{
	pirq_data_ptr = mainboard_pirq_data;
	pirq_data_size = sizeof(mainboard_pirq_data) / sizeof(struct pirq_struct);	/* Get the number of entries */
	intr_data_ptr = mainboard_intr_data;
	picr_data_ptr = mainboard_picr_data;
}

// defines
#define SIO_UNLOCK      0x87
#define SIO_LOCK        0xAA
#define SIO_LDN_SEL     0x07
#define SIO_COM1_LDN    0x02
#define SIO_ENB_REG     0x30
#define SIO_LDN_DIS     0x00
#define BOOTORDER_SIZE  4096

/**
 * TODO
 * SB CIMx callback
 */
void set_pcie_reset(void)
{
}

/**
 * TODO
 * mainboard specific SB CIMx callback
 */
void set_pcie_dereset(void)
{
}

/**********************************************
 * Enable the dedicated functions of the board.
 **********************************************/
static void mainboard_enable(device_t dev)
{
	printk(BIOS_INFO, "Mainboard " CONFIG_MAINBOARD_PART_NUMBER " Enable.\n");

	/*
	 * The mainboard is the first place that we get control in ramstage. Check
	 * for S3 resume and call the appropriate AGESA/CIMx resume functions.
	 */
#if CONFIG_HAVE_ACPI_RESUME
	acpi_slp_type = acpi_get_sleep_type();
#endif

	/* Initialize the PIRQ data structures for consumption */
	pirq_setup();
}

#if IS_ENABLED(CONFIG_MAINBOARD_BOOTORDER)
/*
 * If the COM1 serial console was enabled in romstage.c we need to allow the
 * sortbootorder payload (aka "setup") to be a bootable device. If COM1 is
 * disabled the sortbootorder payload needs to be marked as unbootable. The
 * bootorder file will be read from CBFS. The string to add the sortbootorder
 * payload (aka setup) will be added to the end of that file. That altered
 * bootorder file will then be written into a CBMEM table for seabios to use.
 */
void get_mainboard_bootorder_table( struct bootorder_container *bootorder_ptr, char **data_ptr)
{
	char filename[] = {"bootorder"};
	char string[] = {"!/rom@img/setup\n"};
	static char data_buf[BOOTORDER_SIZE]; // this is the true size of the bootorder file
	char *mem_ptr = data_buf;
	char *str_ptr = string;
	int length;

	printk(BIOS_NOTICE, "PC-Engines/APU: adding bootorder to CBMEM\n");

	read_bootorder_from_cbfs(filename, bootorder_ptr, data_ptr);

	/* first copy the bootorder file into a buffer that can be altered */
	length = strlen(*data_ptr);
	if (length > BOOTORDER_SIZE) // this is in case someone edits bootorder file...
		length = BOOTORDER_SIZE; // and doesn't resize it to 4096
	memcpy(mem_ptr, *data_ptr, length);

	/* Find the '0' indicating the end of the data and add in the new string */
	if (mem_ptr) {
		for ( ; ; ) {
			if (*mem_ptr == 0) { // Check for end of data
				get_option(&cmos_console_setting, "debug_level");
				if (cmos_console_setting)
					str_ptr++; // if enabled don't add the '!'
				strncpy(mem_ptr, str_ptr, strlen(str_ptr));
				mem_ptr[strlen(str_ptr)] = 0; // Mark the new end of data
				break;
			}
			mem_ptr++;
		}
	}
	*data_ptr = data_buf; // update the data_ptr to point to the new data instead of CBFS
}
#endif

/*
 * We will stuff a modified version of the first NICs (BDF 1:0.0) MAC address
 * into the smbios serial number location.
 */
const char *smbios_mainboard_serial_number(void)
{
	device_t nic_dev;
	u32 BAR18, MAC_ADDR = 0;
	u8 *memptr, i;

	nic_dev = dev_find_slot(1, PCI_DEVFN(0, 0));
	BAR18 = pci_read_config32(nic_dev, 0x18);
	BAR18 &= 0xFFFFFC00;
	memptr = (u8 *) BAR18;
	// read in the 6 bytes that make up the NIC's MAC address
	for (i = 0; i <= 5; i++) {
		MAC_ADDR <<= 8;
		MAC_ADDR |= *(memptr + i);
	}
	MAC_ADDR &= 0x00FFFFFF; // we only want the 3 LSB of the MAC
	MAC_ADDR /= 4;
	MAC_ADDR -= 64;
	snprintf(tmp, sizeof (tmp),"%d", MAC_ADDR);
	return tmp;
}

/*
 * We will stuff the memory size into the smbios sku location.
 */
const char *smbios_mainboard_sku(void)
{
	// make sure GPIO/PCIB is set to GPIO
	u8 bdata = pm_ioread(SB_PMIOA_REGEA);
	bdata |= 0x01; //set bit 0
	pm_iowrite(SB_PMIOA_REGEA, bdata);

	if (!get_spd_offset())
		snprintf(tmp, sizeof (tmp),"2 GB");
	else
		snprintf(tmp, sizeof (tmp),"4 GB");
	return tmp;
}

static void mainboard_final(void *chip_info)
{
	u32 mmio_base;

	printk(BIOS_INFO, "Mainboard " CONFIG_MAINBOARD_PART_NUMBER " Final.\n");

	/*
	 * LED1/D7/GPIO_189 should be 0
	 * LED2/D6/GPIO_190 should be 1
	 * LED3/D5/GPIO_191 should be 1
	 */
	mmio_base = find_gpio_base();
	configure_gpio(mmio_base, GPIO_189, GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_LOW);
	configure_gpio(mmio_base, GPIO_190, GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_HIGH);
	configure_gpio(mmio_base, GPIO_191, GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_HIGH);

	/* show how much system memory there is */
	msr_t mem_tom, mem_tom2;
	mem_tom = rdmsr(TOP_MEM_MSR);
	mem_tom2 = rdmsr(TOP_MEM2_MSR);
	u64 tom_size = ((u64)mem_tom.hi << 32) + mem_tom.lo;
	u64 tom2_size = ((u64)mem_tom2.hi << 32) + mem_tom2.lo;
	u32 mem_total = (u32)(tom_size / 0x100000);
	if (tom2_size)
		mem_total += (u32)((tom2_size - 0x100000000) / 0x100000);
	printk(BIOS_CRIT, "System memory size: %d MB\n", mem_total);
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};
