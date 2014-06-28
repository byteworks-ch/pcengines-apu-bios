/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */

#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#ifndef __ASSEMBLER__
#if IS_ENABLED(CONFIG_SAVE_EARLY_TIMESTAMPS_TO_CMOS)

#include <cpu/x86/tsc.h>

void save_timestamp_to_cmos(uint8_t start_cmos_addr, tsc_t time);
void get_timestamp_from_cmos(uint8_t start_cmos_addr, tsc_t *time);

#endif /* CONFIG_SAVE_EARLY_TIMESTAMPS_TO_CMOS */


#if defined(__GNUC__)

struct timestamp_entry {
	uint32_t	entry_id;
	uint64_t	entry_stamp;
} __attribute__((packed));

struct timestamp_table {
	uint64_t	base_time;
	uint32_t	max_entries;
	uint32_t	num_entries;
	struct timestamp_entry entries[0]; /* Variable number of entries */
} __attribute__((packed));

enum timestamp_id {
	TS_BASETIME = 0,
	TS_START_ROMSTAGE = 1,
	TS_BEFORE_INITRAM = 2,
	TS_AFTER_INITRAM = 3,
	TS_END_ROMSTAGE = 4,
	TS_START_VBOOT = 5,
	TS_END_VBOOT = 6,
	TS_START_COPYRAM = 8,
	TS_END_COPYRAM = 9,
	TS_START_RAMSTAGE = 10,
	TS_BEFORE_CAR_INIT = 15,
	TS_AFTER_CAR_INIT = 16,
	TS_DEVICE_ENUMERATE = 30,
	TS_FSP_BEFORE_ENUMERATE,
	TS_FSP_AFTER_ENUMERATE,
	TS_DEVICE_CONFIGURE = 40,
	TS_DEVICE_ENABLE = 50,
	TS_DEVICE_INITIALIZE = 60,
	TS_DEVICE_DONE = 70,
	TS_CBMEM_POST = 75,
	TS_WRITE_TABLES = 80,
	TS_FSP_BEFORE_FINALIZE,
	TS_FSP_AFTER_FINALIZE,
	TS_LOAD_PAYLOAD = 90,
	TS_ACPI_WAKE_JUMP = 98,
	TS_SELFBOOT_JUMP = 99,
};

#if CONFIG_COLLECT_TIMESTAMPS && (CONFIG_EARLY_CBMEM_INIT || !defined(__PRE_RAM__))
#include <cpu/x86/tsc.h>
void timestamp_init(tsc_t base);
void timestamp_add(enum timestamp_id id, tsc_t ts_time);
void timestamp_add_now(enum timestamp_id id);
void timestamp_reinit(void);
tsc_t get_initial_timestamp(void);
void timestamp_show_entry(uint32_t entry);
void timestamp_show_id(enum timestamp_id id);
#else
#define timestamp_init(base)
#define timestamp_add(id, time)
#define timestamp_add_now(id)
#define timestamp_reinit()
#define timestamp_show_entry(entry)
#define timestamp_show_id(id)
#endif /* CONFIG_COLLECT_TIMESTAMPS && (CONFIG_EARLY_CBMEM_INIT || !defined...*/
#endif /* #if defined(__GNUC__) */
#endif /* #ifndef __ASSEMBLER__ */


#if CONFIG_SAVE_EARLY_TIMESTAMPS_TO_CMOS

#define TSC_CMOS_INDEX_VAL		0x72
#define SIZE_OF_CMOS1			0x80
#define TOP_OF_CMOS2			0x7F
#define TOP_OF_BOTH_CMOS		0xFF
#define SIZE_OF_TSC				8
#define CMOS_BASETIME_WRITE_ADDR	(TOP_OF_CMOS2 - (SIZE_OF_TSC * 5))
#define CMOS_PRE_CAR_WRITE_ADDR		(TOP_OF_CMOS2 - (SIZE_OF_TSC * 4))
#define CMOS_POST_CAR_WRITE_ADDR	(TOP_OF_CMOS2 - (SIZE_OF_TSC * 3))
#define CMOS_MAIN_START_ADDR		(TOP_OF_BOTH_CMOS - (SIZE_OF_TSC * 2))
#define CMOS_PRE_INITRAM_ADDR		(TOP_OF_BOTH_CMOS - (SIZE_OF_TSC * 1))
#define CMOS_BASETIME_READ_ADDR		(SIZE_OF_CMOS1 + CMOS_BASETIME_WRITE_ADDR)
#define CMOS_PRE_CAR_READ_ADDR		(SIZE_OF_CMOS1 + CMOS_PRE_CAR_WRITE_ADDR)
#define CMOS_POST_CAR_READ_ADDR		(SIZE_OF_CMOS1 + CMOS_POST_CAR_WRITE_ADDR)

#ifndef __ASSEMBLER__

#include <arch/io.h>
/**
 * save_basetime_to_cmos
 * This is in the .h file for the bootblock
 */
static inline void save_basetime_to_cmos(void)
{
	tsc_t time = rdtsc();
	uint8_t i;
	for (i = 0; i < SIZE_OF_TSC; i++) {

		if (i < 4) {
			outb(CMOS_BASETIME_WRITE_ADDR + i, TSC_CMOS_INDEX_VAL);
			outb(time.lo & 0xff, TSC_CMOS_INDEX_VAL + 1);
			time.lo >>= 8;
		} else {
			outb(CMOS_BASETIME_WRITE_ADDR + i, TSC_CMOS_INDEX_VAL);
			outb(time.hi & 0xff, TSC_CMOS_INDEX_VAL + 1);
			time.hi >>= 8;
		}
	}
}

#endif /* #ifndef __ASSEMBLER__ */
#endif /* CONFIG_SAVE_EARLY_TIMESTAMPS_TO_CMOS */

#endif	/* __TIMESTAMP_H__ */
