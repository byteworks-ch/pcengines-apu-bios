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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdint.h>
#include <build.h>
#include <console/console.h>
#include <pc80/mc146818rtc.h>
#include <boot/coreboot_tables.h>
#include <string.h>
#if IS_ENABLED(CONFIG_USE_OPTION_TABLE)
#include "option_table.h"
#include <cbfs.h>
#endif
#if IS_ENABLED(CONFIG_HAVE_ACPI_RESUME)
#include <arch/acpi.h>
#endif

void rtc_update_cmos_date(u8 has_century)
{
	/* Now setup a default date equals to the build date */
	cmos_write(0, RTC_CLK_SECOND);
	cmos_write(0, RTC_CLK_MINUTE);
	cmos_write(1, RTC_CLK_HOUR);
	cmos_write(COREBOOT_BUILD_WEEKDAY_BCD + 1, RTC_CLK_DAYOFWEEK);
	cmos_write(COREBOOT_BUILD_DAY_BCD, RTC_CLK_DAYOFMONTH);
	cmos_write(COREBOOT_BUILD_MONTH_BCD, RTC_CLK_MONTH);
	cmos_write(COREBOOT_BUILD_YEAR_BCD, RTC_CLK_YEAR);
	if (has_century) cmos_write(COREBOOT_BUILD_CENTURY_BCD, RTC_CLK_ALTCENTURY);
}

#ifndef __SMM__
static int rtc_checksum_valid(int range_start, int range_end, int cks_loc)
{
	int i;
	u16 sum, old_sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += cmos_read(i);
	}
	old_sum = ((cmos_read(cks_loc)<<8) | cmos_read(cks_loc+1))&0x0ffff;
	return sum == old_sum;
}

static void rtc_set_checksum(int range_start, int range_end, int cks_loc)
{
	int i;
	u16 sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += cmos_read(i);
	}
	cmos_write(((sum >> 8) & 0x0ff), cks_loc);
	cmos_write(((sum >> 0) & 0x0ff), cks_loc+1);
}
#endif

#if IS_ENABLED(CONFIG_ARCH_X86)
#define RTC_CONTROL_DEFAULT (RTC_24H)
#define RTC_FREQ_SELECT_DEFAULT (RTC_REF_CLCK_32KHZ | RTC_RATE_1024HZ)
#else
#if IS_ENABLED(CONFIG_ARCH_ALPHA)
#define RTC_CONTROL_DEFAULT (RTC_SQWE | RTC_24H)
#define RTC_FREQ_SELECT_DEFAULT (RTC_REF_CLCK_32KHZ | RTC_RATE_1024HZ)
#endif
#endif

#ifndef __SMM__
void rtc_init(int invalid)
{
	int cmos_invalid = 0;
	int checksum_invalid = 0;
	unsigned char x;

#ifndef __PRE_RAM__
#if IS_ENABLED(CONFIG_HAVE_ACPI_RESUME)
	/*
	 * Avoid clearing pending interrupts and resetting the RTC control
	 * register in the resume path because the Linux kernel relies on
	 * this to know if it should restart the RTC timer queue if the wake
	 * was due to the RTC alarm.
	 */
	if (acpi_slp_type == 3)
		return;
#endif
#endif

	printk(BIOS_DEBUG, "RTC Init\n");

#if IS_ENABLED(CONFIG_USE_OPTION_TABLE)
#define CLEAR_CMOS 0
#else
#define CLEAR_CMOS 1
#endif

	/* See if there has been a CMOS power problem. */
	x = cmos_read(RTC_VALID);
	cmos_invalid = !(x & RTC_VRT);

	/* See if there is a CMOS checksum error */
	checksum_invalid = !rtc_checksum_valid(PC_CKS_RANGE_START,
			PC_CKS_RANGE_END,PC_CKS_LOC);


	if (invalid || cmos_invalid || checksum_invalid) {
#if CLEAR_CMOS
		int i;

		cmos_write(0, RTC_CLK_SECOND_ALARM);
		cmos_write(0, RTC_CLK_MINUTE_ALARM);
		cmos_write(0, RTC_CLK_HOUR_ALARM);
		for(i = 10; i < 128; i++) {
			cmos_write(0, i);
		}
#endif
		if (cmos_invalid) {
			rtc_update_cmos_date(RTC_HAS_NO_ALTCENTURY);
		}

		printk(BIOS_WARNING, "RTC:%s%s%s%s\n",
			invalid?" Clear requested":"",
			cmos_invalid?" Power Problem":"",
			checksum_invalid?" Checksum invalid":"",
			CLEAR_CMOS?" zeroing cmos":"");
	}

	/* Setup the real time clock */
	cmos_write(RTC_CONTROL_DEFAULT, RTC_CONTROL);
	/* Setup the frequency it operates at */
	cmos_write(RTC_FREQ_SELECT_DEFAULT, RTC_FREQ_SELECT);
	/* Ensure all reserved bits are 0 in register D */
	cmos_write(RTC_VRT, RTC_VALID);

#if IS_ENABLED(CONFIG_USE_OPTION_TABLE)
	/* See if there is a LB CMOS checksum error */
	checksum_invalid = !rtc_checksum_valid(LB_CKS_RANGE_START,
			LB_CKS_RANGE_END,LB_CKS_LOC);
	if(checksum_invalid)
		printk(BIOS_DEBUG, "RTC: coreboot checksum invalid\n");
#endif

	/* Make certain we have a valid checksum */
	rtc_set_checksum(PC_CKS_RANGE_START, PC_CKS_RANGE_END, PC_CKS_LOC);

	/* Clear any pending interrupts */
	(void) cmos_read(RTC_INTR_FLAGS);
}
#endif


#if IS_ENABLED(CONFIG_USE_OPTION_TABLE)
/* This routine returns the value of the requested bits
	input bit = bit count from the beginning of the cmos image
	      length = number of bits to include in the value
	      ret = a character pointer to where the value is to be returned
	output the value placed in ret
	      returns CB_SUCCESS = successful, cb_err code if an error occurred
*/
static enum cb_err get_cmos_value(unsigned long bit, unsigned long length,
				  void *vret)
{
	unsigned char *ret;
	unsigned long byte,byte_bit;
	unsigned long i;
	unsigned char uchar;

	/* The table is checked when it is built to ensure all
		values are valid. */
	ret = vret;
	byte=bit/8;	/* find the byte where the data starts */
	byte_bit=bit%8; /* find the bit in the byte where the data starts */
	if(length<9) {	/* one byte or less */
		uchar = cmos_read(byte); /* load the byte */
		uchar >>= byte_bit;	/* shift the bits to byte align */
		/* clear unspecified bits */
		ret[0] = uchar & ((1 << length) -1);
	}
	else {	/* more that one byte so transfer the whole bytes */
		for(i=0;length;i++,length-=8,byte++) {
			/* load the byte */
			ret[i]=cmos_read(byte);
		}
	}
	return CB_SUCCESS;
}

enum cb_err get_option(void *dest, const char *name)
{
	struct cmos_option_table *ct;
	struct cmos_entries *ce;
	size_t namelen;
	int found=0;

	/* Figure out how long name is */
	namelen = strnlen(name, CMOS_MAX_NAME_LENGTH);

	/* find the requested entry record */
	ct = cbfs_get_file_content(CBFS_DEFAULT_MEDIA, "cmos_layout.bin",
				   CBFS_COMPONENT_CMOS_LAYOUT, NULL);
	if (!ct) {
		printk(BIOS_ERR, "RTC: cmos_layout.bin could not be found. "
						"Options are disabled\n");
		return CB_CMOS_LAYOUT_NOT_FOUND;
	}
	ce=(struct cmos_entries*)((unsigned char *)ct + ct->header_length);
	for(;ce->tag==LB_TAG_OPTION;
		ce=(struct cmos_entries*)((unsigned char *)ce + ce->size)) {
		if (memcmp(ce->name, name, namelen) == 0) {
			found=1;
			break;
		}
	}
	if(!found) {
		printk(BIOS_DEBUG, "WARNING: No CMOS option '%s'.\n", name);
		return CB_CMOS_OPTION_NOT_FOUND;
	}

	if(get_cmos_value(ce->bit, ce->length, dest) != CB_SUCCESS)
		return CB_CMOS_ACCESS_ERROR;
	if(!rtc_checksum_valid(LB_CKS_RANGE_START, LB_CKS_RANGE_END,LB_CKS_LOC))
		return CB_CMOS_CHECKSUM_INVALID;
	return CB_SUCCESS;
}

static enum cb_err set_cmos_value(unsigned long bit, unsigned long length,
				  void *vret)
{
	unsigned char *ret;
	unsigned long byte,byte_bit;
	unsigned long i;
	unsigned char uchar, mask;
	unsigned int chksum_update_needed = 0;

	ret = vret;
	byte = bit / 8;			/* find the byte where the data starts */
	byte_bit = bit % 8;		/* find the bit in the byte where the data starts */
	if(length <= 8) {		/* one byte or less */
		mask = (1 << length) - 1;
		mask <<= byte_bit;

		uchar = cmos_read(byte);
		uchar &= ~mask;
		uchar |= (ret[0] << byte_bit);
		cmos_write(uchar, byte);
		if (byte >= LB_CKS_RANGE_START && byte <= LB_CKS_RANGE_END)
			chksum_update_needed = 1;
	} else {			/* more that one byte so transfer the whole bytes */
		if (byte_bit || length % 8)
			return CB_ERR_ARG;

		for(i=0; length; i++, length-=8, byte++)
			cmos_write(ret[i], byte);
			if (byte >= LB_CKS_RANGE_START && byte <= LB_CKS_RANGE_END)
				chksum_update_needed = 1;
	}

	if (chksum_update_needed) {
		rtc_set_checksum(LB_CKS_RANGE_START,
			LB_CKS_RANGE_END,LB_CKS_LOC);
	}
	return CB_SUCCESS;
}


enum cb_err set_option(const char *name, void *value)
{
	struct cmos_option_table *ct;
	struct cmos_entries *ce;
	unsigned long length;
	size_t namelen;
	int found=0;

	/* Figure out how long name is */
	namelen = strnlen(name, CMOS_MAX_NAME_LENGTH);

	/* find the requested entry record */
	ct = cbfs_get_file_content(CBFS_DEFAULT_MEDIA, "cmos_layout.bin",
				   CBFS_COMPONENT_CMOS_LAYOUT, NULL);
	if (!ct) {
		printk(BIOS_ERR, "cmos_layout.bin could not be found. Options are disabled\n");
		return CB_CMOS_LAYOUT_NOT_FOUND;
	}
	ce=(struct cmos_entries*)((unsigned char *)ct + ct->header_length);
	for(;ce->tag==LB_TAG_OPTION;
		ce=(struct cmos_entries*)((unsigned char *)ce + ce->size)) {
		if (memcmp(ce->name, name, namelen) == 0) {
			found=1;
			break;
		}
	}
	if(!found) {
		printk(BIOS_DEBUG, "WARNING: No CMOS option '%s'.\n", name);
		return CB_CMOS_OPTION_NOT_FOUND;
	}

	length = ce->length;
	if (ce->config == 's') {
		length = MAX(strlen((const char *)value) * 8, ce->length - 8);
		/* make sure the string is null terminated */
		if (set_cmos_value(ce->bit + ce->length - 8, 8, &(u8[]){0})
		    != CB_SUCCESS)
			return (CB_CMOS_ACCESS_ERROR);
	}

	if (set_cmos_value(ce->bit, length, value) != CB_SUCCESS)
		return (CB_CMOS_ACCESS_ERROR);

	return CB_SUCCESS;
}
#endif /* CONFIG_USE_OPTION_TABLE */

/*
 * If the CMOS is cleared, the rtc_reg has the invalid date. That
 * hurts some OSes. Even if we don't set USE_OPTION_TABLE, we need
 * to make sure the date is valid.
 */
void rtc_check_update_cmos_date(u8 has_century)
{
	u8 year, century, month, dayofmonth, dayofweek;
	u8 rtc_error_type = 0;

	/* See if the RTC has lost power */
	if ((cmos_read(RTC_VALID) & RTC_VRT) == 0x00) {
		printk(BIOS_ERR, "RTC lost power! Update the date to builddate\n");
		rtc_update_cmos_date(has_century);
	}
	century    = has_century ? cmos_read(RTC_CLK_ALTCENTURY) : 0;
	year       = cmos_read(RTC_CLK_YEAR);
	month      = cmos_read(RTC_CLK_MONTH);
	dayofmonth = cmos_read(RTC_CLK_DAYOFMONTH);
	dayofweek  = cmos_read(RTC_CLK_DAYOFWEEK);

	printk(BIOS_NOTICE, "RTC: %02x/%02x/%02x%02x day of week=%x\n",
		month, dayofmonth, century, year, dayofweek);

	/* Check to see that the date values are valid. */
	/* First check to see if any of the BCD values have a non base 10 value */
	if ( test_bcd_encoding( century )
	  || test_bcd_encoding( year )
	  || test_bcd_encoding( month )
	  || test_bcd_encoding( dayofmonth )
	  || test_bcd_encoding( dayofweek ) ) {
		rtc_error_type |= RTC_BCD_ERROR;
	}

	/* Now check to see that the date values are in the correct range */
	if ( (bcd_to_hex(century) != bcd_to_hex(0x20) )
	  || (bcd_to_hex(year) > 99 )
	  || (bcd_to_hex(month) > 12 )
	  || (bcd_to_hex(dayofmonth) > 31 )
	  || (bcd_to_hex(dayofweek) > 7 ) ) {
		rtc_error_type |= RTC_RANGE_ERROR;
	}

	if ( rtc_error_type ) {
		if( rtc_error_type & RTC_BCD_ERROR )
			printk(BIOS_ERR, "RTC has an invalid BCD value\n");
		if( rtc_error_type & RTC_RANGE_ERROR )
			printk(BIOS_ERR, "RTC has an invalid range\n");
		rtc_update_cmos_date(has_century);
		printk(BIOS_ERR, "RTC: %02x/%02x/%02x%02x day of week=%x\n",
			cmos_read(RTC_CLK_MONTH),
			cmos_read(RTC_CLK_DAYOFMONTH),
			has_century ? cmos_read(RTC_CLK_ALTCENTURY) : 0,
			cmos_read(RTC_CLK_YEAR),
			cmos_read(RTC_CLK_DAYOFWEEK));
	}
}
