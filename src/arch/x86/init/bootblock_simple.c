/*
 * This file is part of the coreboot project.
 *
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

#include <smp/node.h>
#include <bootblock_common.h>

static void main(unsigned long bist)
{
	if (boot_cpu()) {
		bootblock_mainboard_init();

#if CONFIG_USE_OPTION_TABLE
		sanitize_cmos();
#endif

#if CONFIG_SAVE_EARLY_TIMESTAMPS_TO_CMOS
	save_basetime_to_cmos();
#endif

#if CONFIG_CMOS_POST
		cmos_post_init();
#endif
	}

	const char* target1 = "fallback/romstage";
	unsigned long entry;
	entry = findstage(target1);
	if (entry) call(entry, bist);
	asm volatile ("1:\n\thlt\n\tjmp 1b\n\t");
}

