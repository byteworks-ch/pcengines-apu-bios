/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,  MA 02110-1301 USA
 */

#include <AGESA.h>
#include <Lib/amdlib.h>
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/cache.h>
#include <cbmem.h>
#include <device/device.h>
#include <arch/io.h>
#include <arch/acpi.h>
#include <string.h>
#include "Porting.h"
#include "BiosCallOuts.h"
#include "s3_resume.h"
#include "agesawrapper.h"

#ifndef __PRE_RAM__
#include <spi-generic.h>
#include <spi_flash.h>
#endif

const u32 mtrr_table_fixed_MTRR[] = {
	0x250, 0x258, 0x259, 0x268,
	0x269, 0x26A, 0x26B, 0x26C,
	0x26D, 0x26E, 0x26F
};

const u32 mtrr_table_var_MTRR[] = {
	0x200, 0x201, 0x202, 0x203,
	0x204, 0x205, 0x206, 0x207,
	0x208, 0x209, 0x20A, 0x20B,
	0x20C, 0x20D, 0x20E, 0x20F,
	SYS_CFG,
	TOP_MEM,
	TOP_MEM2
};

void restore_mtrr(void)
{
	volatile UINT32 *msrPtr = (volatile UINT32 *)S3_DATA_MTRR_POS;
	msr_t msr_data;
	u32 i;

	printk(BIOS_SPEW, "%s\n", __func__);

	disable_cache();

	/* Enable access to AMD RdDram and WrDram extension bits */
	msr_data = rdmsr(SYS_CFG);
	msr_data.lo |= SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYS_CFG, msr_data);

	/* Restore the Fixed MTRRs */
	for (i = 0; i < (u32)sizeof(mtrr_table_fixed_MTRR)/sizeof(u32); i++) {
		msr_data.lo = *msrPtr;
		msrPtr ++;
		msr_data.hi = *msrPtr;
		msrPtr ++;
		wrmsr(mtrr_table_fixed_MTRR[i], msr_data);
	}

	/* Disable access to AMD RdDram and WrDram extension bits */
	msr_data = rdmsr(SYS_CFG);
	msr_data.lo &= ~SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYS_CFG, msr_data);

	/* Restore the Variable MTRRs, SYS_CFG, TOM, TOM2 */
	for (i = 0; i < (u32)sizeof(mtrr_table_var_MTRR)/sizeof(u32); i++) {
		msr_data.lo = *msrPtr;
		msrPtr ++;
		msr_data.hi = *msrPtr;
		msrPtr ++;
		wrmsr(mtrr_table_var_MTRR[i], msr_data);
	}
}

void *backup_resume(void)
{
	void *resume_backup_memory;

	if (cbmem_recovery(1))
		return NULL;

	resume_backup_memory = cbmem_find(CBMEM_ID_RESUME);
	if (((u32) resume_backup_memory == 0)
	    || ((u32) resume_backup_memory == -1)) {
		printk(BIOS_ERR, "Error: resume_backup_memory: %x\n",
		       (u32) resume_backup_memory);
		for (;;) ;
	}

	return resume_backup_memory;
}

void move_stack_high_mem(void)
{
	void *high_stack;

	high_stack = cbmem_find(CBMEM_ID_RESUME_SCRATCH);
	memcpy(high_stack, (void *)BSP_STACK_BASE_ADDR,
		(CONFIG_HIGH_SCRATCH_MEMORY_SIZE - BIOS_HEAP_SIZE));

	__asm__
	    volatile ("add	%0, %%esp; add %0, %%ebp; invd"::"g"
		      (high_stack - BSP_STACK_BASE_ADDR)
		      :);
}

#ifndef __PRE_RAM__
void write_mtrr(struct spi_flash *flash, u32 *p_nvram_pos, unsigned idx)
{
	msr_t  msr_data;
	msr_data = rdmsr(idx);

#if CONFIG_AMD_SB_SPI_TX_LEN >= 8
	flash->write(flash, *p_nvram_pos, 8, &msr_data);
	*p_nvram_pos += 8;
#else
	flash->write(flash, *p_nvram_pos, 4, &msr_data.lo);
	*p_nvram_pos += 4;
	flash->write(flash, *p_nvram_pos, 4, &msr_data.hi);
	*p_nvram_pos += 4;
#endif
}

static int check_mtrr(u32 *p_nvram_pos, unsigned idx)
{
	msr_t *spi_data = (msr_t *)*p_nvram_pos;
	msr_t msr_data = rdmsr(idx);
	*p_nvram_pos += 8;
	return (msr_data.hi != spi_data->hi) || (msr_data.lo != spi_data->lo);
}

/*
 * Check to see if the MTRR data that exists in the nvram matches
 * what the current MTRR settings are. If they match return 0.
 * If they do not match return 1 and they will be written.
 */
static int check_saved_mtrr_data(u32 spi_data)
{
	msr_t msr_data;
	int i;

	/* Enable access to AMD RdDram and WrDram extension bits */
	msr_data = rdmsr(SYS_CFG);
	msr_data.lo |= SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYS_CFG, msr_data);

	/* Fixed MTRRs */
	for (i = 0; i < (u32)sizeof(mtrr_table_fixed_MTRR)/sizeof(u32); i++) {
		if (check_mtrr(&spi_data, mtrr_table_fixed_MTRR[i])) {
			/*
			 * The RdDram/WrDram extension bits will need to be configured
			 * by the calling function prior to reading any MTRR registers.
			 */
			return 1;
		}
	}

	/* Disable access to AMD RdDram and WrDram extension bits */
	msr_data = rdmsr(SYS_CFG);
	msr_data.lo &= ~SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYS_CFG, msr_data);

	/* Variable MTRRs, SYS_CFG, TOM, TOM2 */
	for (i = 0; i < (u32)sizeof(mtrr_table_var_MTRR)/sizeof(u32); i++) {
		if (check_mtrr(&spi_data, mtrr_table_var_MTRR[i]))
			return 1;
	}

	printk(BIOS_DEBUG, "S3: MTRR nvram already written\n");
	return 0; // All comparisons passed
}
#endif

void OemAgesaSaveMtrr(void)
{
#ifndef __PRE_RAM__
	msr_t  msr_data;
	u32 nvram_pos = S3_DATA_MTRR_POS;
	u32 i;
	struct spi_flash *flash;

	if (!check_saved_mtrr_data(nvram_pos))
		return;

	spi_init();

	flash = spi_flash_probe(0, 0, 0, 0);
	if (!flash) {
		printk(BIOS_DEBUG, "Could not find SPI device\n");
		return;
	}

	flash->spi->rw = SPI_WRITE_FLAG;
	spi_claim_bus(flash->spi);

	flash->erase(flash, S3_DATA_MTRR_POS, S3_DATA_MTRR_SIZE);

	/* Enable access to AMD RdDram and WrDram extension bits */
	msr_data = rdmsr(SYS_CFG);
	msr_data.lo |= SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYS_CFG, msr_data);

	/* Fixed MTRRs */
	for (i = 0; i < (u32)sizeof(mtrr_table_fixed_MTRR)/sizeof(u32); i++)
		write_mtrr(flash, &nvram_pos, mtrr_table_fixed_MTRR[i]);

	/* Disable access to AMD RdDram and WrDram extension bits */
	msr_data = rdmsr(SYS_CFG);
	msr_data.lo &= ~SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYS_CFG, msr_data);

	/* Variable MTRRs, SYS_CFG, TOM, TOM2 */
	for (i = 0; i < (u32)sizeof(mtrr_table_var_MTRR)/sizeof(u32); i++)
		write_mtrr(flash, &nvram_pos, mtrr_table_var_MTRR[i]);

	flash->spi->rw = SPI_WRITE_FLAG;
	spi_release_bus(flash->spi);

#endif
}

void OemAgesaGetS3Info(S3_DATA_TYPE S3DataType, u32 *DataSize, void **Data)
{
	AMD_CONFIG_PARAMS StdHeader;
	if (S3DataType == S3DataTypeNonVolatile) {
		*Data = (void *)S3_DATA_NONVOLATILE_POS;
		*DataSize = *(UINTN *) (*Data);
		*Data += 4;
	} else {
		*DataSize = *(UINTN *) S3_DATA_VOLATILE_POS;
		*Data = (void *) GetHeapBase(&StdHeader);
		memcpy((void *)(*Data), (void *)(S3_DATA_VOLATILE_POS + 4), *DataSize);
	}
}

#ifndef __PRE_RAM__
u32 OemAgesaSaveS3Info(S3_DATA_TYPE S3DataType, u32 DataSize, void *Data)
{
	u32 pos;
	struct spi_flash *flash;
	u8 *new_data;
	u32 bytes_to_process;
	u32 nvram_pos;

	if (S3DataType == S3DataTypeNonVolatile)
		pos = S3_DATA_NONVOLATILE_POS;
	else
		pos = S3_DATA_VOLATILE_POS;

	spi_init();
	flash = spi_flash_probe(0, 0, 0, 0);
	if (!flash) {
		printk(BIOS_DEBUG, "%s: Could not find SPI device\n", __func__);
		/* Don't make flow stop. */
		return AGESA_SUCCESS;
	}

	flash->spi->rw = SPI_WRITE_FLAG;
	spi_claim_bus(flash->spi);

	// initialize the incoming data array
	new_data = (u8 *)malloc(DataSize + (u32)sizeof(DataSize));
	memcpy(new_data, &DataSize, (u32)sizeof(DataSize)); // the size gets written first
	memcpy(new_data + (u32)sizeof(DataSize), Data, DataSize);
	DataSize += (u32)sizeof(DataSize); // add in the size of the data

	for ( ; DataSize > 0; DataSize -= bytes_to_process) {
		bytes_to_process = ( DataSize >= flash->sector_size) ? flash->sector_size : DataSize;
		if (memcmp((u8 *)pos, (u8 *)new_data, bytes_to_process)) {
			printk(BIOS_DEBUG, "%s: Data mismatch - write the data\n", __func__);
			flash->erase(flash, pos, flash->sector_size);
			for (nvram_pos = 0; \
			     nvram_pos < bytes_to_process - (bytes_to_process % CONFIG_AMD_SB_SPI_TX_LEN); \
			     nvram_pos += CONFIG_AMD_SB_SPI_TX_LEN) {
				flash->write(flash, pos + nvram_pos, CONFIG_AMD_SB_SPI_TX_LEN, \
				             (u8 *)(new_data + nvram_pos));
			}
			flash->write(flash, pos + nvram_pos, bytes_to_process % CONFIG_AMD_SB_SPI_TX_LEN, \
			             (u8 *)(new_data + nvram_pos));
		}
		else
			printk(BIOS_DEBUG, "%s: existing nvram data matched\n", __func__);

		new_data += bytes_to_process;
		pos += bytes_to_process;
	}
	free(new_data);
	flash->spi->rw = SPI_WRITE_FLAG;
	spi_release_bus(flash->spi);

	return AGESA_SUCCESS;
}
#endif

void set_resume_cache(void)
{
	msr_t msr;

	/* disable fixed mtrr for now,  it will be enabled by mtrr restore */
	msr = rdmsr(SYSCFG_MSR);
	msr.lo &= ~(SYSCFG_MSR_MtrrFixDramEn | SYSCFG_MSR_MtrrFixDramModEn);
	wrmsr(SYSCFG_MSR, msr);

	/* Enable caching for 0 - coreboot ram using variable mtrr */
	msr.lo = 0 | MTRR_TYPE_WRBACK;
	msr.hi = 0;
	wrmsr(MTRRphysBase_MSR(0), msr);
	msr.lo = ~(CONFIG_RAMTOP - 1) | MTRRphysMaskValid;
	msr.hi = (1 << (CONFIG_CPU_ADDR_BITS - 32)) - 1;
	wrmsr(MTRRphysMask_MSR(0), msr);

	/* Set the default memory type and disable fixed and enable variable MTRRs */
	msr.hi = 0;
	msr.lo = (1 << 11);
	wrmsr(MTRRdefType_MSR, msr);

	enable_cache();
}

void s3_resume(void)
{
	int status;

	printk(BIOS_DEBUG, "agesawrapper_amds3laterestore ");
	status = agesawrapper_amds3laterestore();
	if (status)
		printk(BIOS_DEBUG, "error level: %x \n", (u32) status);
	else
		printk(BIOS_DEBUG, "passed.\n");
}
