/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
 * Copyright (C) 2012 Google Inc.
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC.
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
#include <string.h>
#include <device/device.h>
#include <cpu/cpu.h>
#include <cpu/x86/name.h>

void fill_processor_name(char *processor_name)
{
	struct cpuid_result regs;
	char temp_processor_name[49];
	char *processor_name_start;
	unsigned int *name_as_ints = (unsigned int *)temp_processor_name;
	int i;

	for (i = 0; i < 3; i++) {
		regs = cpuid(EXT_CPUID_PROC_BRAND_STR1 + i);
		name_as_ints[i * 4 + 0] = regs.eax;
		name_as_ints[i * 4 + 1] = regs.ebx;
		name_as_ints[i * 4 + 2] = regs.ecx;
		name_as_ints[i * 4 + 3] = regs.edx;
	}

	temp_processor_name[48] = 0;

	/* Skip leading spaces. */
	processor_name_start = temp_processor_name;
	while (*processor_name_start == ' ')
		processor_name_start++;

	memset(processor_name, 0, 49);
	strcpy(processor_name, processor_name_start);
}

void print_cpu_info(void)
{
	struct cpuid_result cpuidr;
	u32 i;
	char cpu_string[50], *cpu_name = cpu_string; /* 48 bytes are reported */
	int vt, txt, aes;
	const char *mode[] = {"NOT ", ""};

	cpuidr = cpuid(EXT_CPUID_MAX_INPUT_VAL);
	if (cpuidr.eax < EXT_CPUID_PROC_BRAND_STR3) {
		strcpy(cpu_string, "Platform name string not available");
	} else {
		u32 *p = (u32*) cpu_string;
		for (i = EXT_CPUID_PROC_BRAND_STR1; i <= EXT_CPUID_PROC_BRAND_STR3 ; i++) {
			cpuidr = cpuid(i);
			*p++ = cpuidr.eax;
			*p++ = cpuidr.ebx;
			*p++ = cpuidr.ecx;
			*p++ = cpuidr.edx;
		}
	}

	cpuidr = cpuid(CPUID_VERSION_INFORMATION);
	printk(BIOS_DEBUG, "CPU id(%x): ", cpuidr.eax);

	/* Skip leading spaces in CPU name string */
	while (cpu_name[0] == ' ')
		cpu_name++;

	/* Only print the first space if multiple spaces are in the string */
	for (i = 0;i < strlen(cpu_name);i++) {
		if ((cpu_name[i] != ' ') ||
			((cpu_name[i] == ' ') && (cpu_name[i-1] != ' ')))
			printk(BIOS_DEBUG, "%c", cpu_name[i]);
	}
	printk(BIOS_DEBUG, "\n");

	aes = (cpuidr.ecx & ECX_CPU_SUPPORTS_AESNI) ? 1 : 0;
	txt = (cpuidr.ecx & ECX_CPU_SUPPORTS_SMX) ? 1 : 0;
	vt = (cpuidr.ecx & ECX_CPU_SUPPORTS_VMX) ? 1 : 0;
	printk(BIOS_DEBUG, "AES %ssupported, TXT %ssupported, VT %ssupported\n",
		   mode[aes], mode[txt], mode[vt]);
}

