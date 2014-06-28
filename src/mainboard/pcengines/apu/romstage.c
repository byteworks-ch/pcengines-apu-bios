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

#include <stdint.h>
#include <string.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include <arch/stages.h>
#include <device/pnp_def.h>
#include <arch/cpu.h>
#include <cpu/x86/lapic.h>
#include <console/console.h>
#include <console/loglevel.h>
#include <cpu/x86/mtrr.h>
#include "agesawrapper.h"
#include "cpu/x86/bist.h"
#include "drivers/pc80/i8254.c"
#include "drivers/pc80/i8259.c"
#include <cpu/x86/cache.h>
#include <sb_cimx.h>
#include "SBPLATFORM.h"
#include "cbmem.h"
#include "cpu/amd/mtrr.h"
#include "cpu/amd/agesa/s3_resume.h"
#include "superio/nuvoton/nct5104d/early_init.c"
#include "src/southbridge/amd/cimx/cimx_util.h"
#include <cbfs.h>
#include "gpio_ftns.h"

void disable_cache_as_ram(void); /* cache_as_ram.inc */
void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx);

#define MSR_MTRR_VARIABLE_BASE6  0x020C
#define MSR_MTRR_VARIABLE_MASK6  0x020D
#define MSR_PSTATE_CONTROL       0xC0010062
#define SERIAL_DEV               PNP_DEV(CONFIG_SIO_PORT, NCT5104D_SP1)
#define SIO_UNLOCK               0x87
#define SIO_LOCK                 0xAA
#define SIO_LDN_SEL              0x07
#define SIO_COM1_LDN             0x02
#define SIO_ENB_REG              0x30
#define SIO_LDN_DIS              0x00

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	u32 val;
	u32 mmio_base;

	/* PC Engines requires system boot when power is applied. This feature is
	 * controlled in PM_REG 5Bh register. "Always Power On" works by writing a
	 * value of 05h.
	 */
	u8 bdata = pm_ioread(SB_PMIOA_REG5B);
	bdata &= 0xf8; //clear bits 0-2
	bdata |= 0x05; //set bits 0,2
	pm_iowrite(SB_PMIOA_REG5B, bdata);

	/* Multi-function pins switch to GPIO0-35, these pins are shared with PCI pins */
	bdata = pm_ioread(SB_PMIOA_REGEA);
	bdata &= 0xfe; //clear bit 0
	bdata |= 0x01; //set bit 0
	pm_iowrite(SB_PMIOA_REGEA, bdata);

	//configure required GPIOs
	mmio_base = find_gpio_base();
	configure_gpio(mmio_base, GPIO_10,  GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_HIGH);
	configure_gpio(mmio_base, GPIO_11,  GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_HIGH);
	configure_gpio(mmio_base, GPIO_15,  GPIO_FTN_1, GPIO_INPUT);
	configure_gpio(mmio_base, GPIO_16,  GPIO_FTN_1, GPIO_INPUT);
	configure_gpio(mmio_base, GPIO_17,  GPIO_FTN_1, GPIO_INPUT);
	configure_gpio(mmio_base, GPIO_18,  GPIO_FTN_1, GPIO_INPUT);
	configure_gpio(mmio_base, GPIO_187, GPIO_FTN_1, GPIO_INPUT);
	configure_gpio(mmio_base, GPIO_189, GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_LOW);
	configure_gpio(mmio_base, GPIO_190, GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_LOW);
	configure_gpio(mmio_base, GPIO_191, GPIO_FTN_1, GPIO_OUTPUT | GPIO_DATA_LOW);

#if CONFIG_HAVE_ACPI_RESUME
	void *resume_backup_memory;
#endif

	/*
	 * All cores: allow caching of flash chip code and data
	 * (there are no cache-as-ram reliability concerns with family 14h)
	 */
	__writemsr (MSR_MTRR_VARIABLE_BASE6, (0x0100000000ull - CACHE_ROM_SIZE) | 5);
	__writemsr (MSR_MTRR_VARIABLE_MASK6, (0x1000000000ull - CACHE_ROM_SIZE) | 0x800);

	/* All cores: set pstate 0 (1600 MHz) early to save a few ms of boot time */
	__writemsr (MSR_PSTATE_CONTROL, 0);

	if (!cpu_init_detectedx && boot_cpu()) {
		post_code(0x30);
		sb_Poweron_Init();

		post_code(0x31);

		/* Enable COM1 serial console if bootorder file says "scon1" OR GPIO_187 is low. */
		if (cbfs_find_string("scon1", "bootorder") || !read_gpio(mmio_base, GPIO_187))
			nct5104d_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE); /* Enable UART - COM1 port */
		else { /* disable COM1 */
			outb( SIO_UNLOCK,   CONFIG_SIO_PORT);     /* unlock SIO */
			outb( SIO_UNLOCK,   CONFIG_SIO_PORT);     /* unlock SIO */
			outb( SIO_LDN_SEL,  CONFIG_SIO_PORT);     /* select LDN */
			outb( SIO_COM1_LDN, CONFIG_SIO_PORT + 1); /* select LDN=2 */
			outb( SIO_ENB_REG,  CONFIG_SIO_PORT);     /* select ENABLE REG */
			outb( SIO_LDN_DIS,  CONFIG_SIO_PORT + 1); /* disable it */
			outb( SIO_LOCK,     CONFIG_SIO_PORT);     /* lock SIO */
		}
		console_init();
	}
	printk(BIOS_CRIT, "PC Engines APU BIOS build date: %s\n", __DATE__);

	/* Halt if there was a built in self test failure */
	post_code(0x34);
	report_bist_failure(bist);

	/* Load MPB */
	val = cpuid_eax(1);
	printk(BIOS_DEBUG, "BSP Family_Model: %08x \n", val);
	printk(BIOS_DEBUG, "cpu_init_detectedx = %08lx \n", cpu_init_detectedx);

	post_code(0x35);
	printk(BIOS_DEBUG, "agesawrapper_amdinitmmio ");
	val = agesawrapper_amdinitmmio();
	if (val)
		printk(BIOS_DEBUG, "error level: %x \n", val);
	else
		printk(BIOS_DEBUG, "passed.\n");

	post_code(0x37);
	printk(BIOS_DEBUG, "agesawrapper_amdinitreset ");
	val = agesawrapper_amdinitreset();
	if (val)
		printk(BIOS_DEBUG, "error level: %x \n", val);
	else
		printk(BIOS_DEBUG, "passed.\n");

	post_code(0x39);
	printk(BIOS_DEBUG, "agesawrapper_amdinitearly ");
	val = agesawrapper_amdinitearly ();
	if (val)
		printk(BIOS_DEBUG, "error level: %x \n", val);
	else
		printk(BIOS_DEBUG, "passed.\n");

#if CONFIG_HAVE_ACPI_RESUME
	if (!acpi_is_wakeup_early()) { /* Check for S3 resume */
#endif
		post_code(0x40);
		printk(BIOS_DEBUG, "agesawrapper_amdinitpost ");
		val = agesawrapper_amdinitpost ();
		if (val)
			printk(BIOS_DEBUG, "error level: %x \n", val);
		else
			printk(BIOS_DEBUG, "passed.\n");

		post_code(0x42);
		printk(BIOS_DEBUG, "agesawrapper_amdinitenv ");
		val = agesawrapper_amdinitenv ();
		if (val)
			printk(BIOS_DEBUG, "error level: %x \n", val);
		else
			printk(BIOS_DEBUG, "passed.\n");

#if CONFIG_HAVE_ACPI_RESUME
	} else { 			/* S3 detect */
		printk(BIOS_INFO, "S3 detected\n");

		post_code(0x60);
		printk(BIOS_DEBUG, "agesawrapper_amdinitresume ");
		val = agesawrapper_amdinitresume();
		if (val)
			printk(BIOS_DEBUG, "error level: %x \n", val);
		else
			printk(BIOS_DEBUG, "passed.\n");

		printk(BIOS_DEBUG, "agesawrapper_amds3laterestore ");
		val = agesawrapper_amds3laterestore ();
		if (val)
			printk(BIOS_DEBUG, "error level: %x \n", val);
		else
			printk(BIOS_DEBUG, "passed.\n");

		post_code(0x61);
		printk(BIOS_DEBUG, "Find resume memory location\n");
		resume_backup_memory = backup_resume();

		post_code(0x62);
		printk(BIOS_DEBUG, "Move CAR stack.\n");
		move_stack_high_mem();
		printk(BIOS_DEBUG, "stack moved to: 0x%x\n", (u32) (resume_backup_memory + HIGH_MEMORY_SAVE));

		post_code(0x63);
		disable_cache_as_ram();
		printk(BIOS_DEBUG, "CAR disabled.\n");
		set_resume_cache();

		/*
		 * Copy the system memory that is in the ramstage area to the
		 * reserved area.
		 */
		if (resume_backup_memory)
			memcpy(resume_backup_memory, (void *)(CONFIG_RAMBASE), HIGH_MEMORY_SAVE);

		printk(BIOS_DEBUG, "System memory saved. OK to load ramstage.\n");
	}
#endif

	/* Initialize i8259 pic */
	post_code(0x43);
	setup_i8259 ();

	/* Initialize i8254 timers */
	post_code(0x44);
	setup_i8254 ();

	post_code(0x50);
	copy_and_run();
	printk(BIOS_ERR, "Error: copy_and_run() returned!\n");

	post_code(0x54);	/* Should never see this post code. */
}
