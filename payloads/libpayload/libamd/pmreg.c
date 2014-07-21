/*
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

#include <libpayload.h>
#include <libamd/pmreg.h>

u8 pmreg_read(u8 index) {
	outb( index, PMREG_INDEX );
	return  inb( PMREG_DATA );
}

void pmreg_write(u8 index, u8 data) {
	outb( index, PMREG_INDEX );
	outb(  data, PMREG_DATA );
}

u8 pmreg2_read(u8 index) {
	outb( index, PMREG2_INDEX );
	return  inb( PMREG2_DATA );
}

void pmreg2_write(u8 index, u8 data) {
	outb( index, PMREG2_INDEX );
	outb(  data, PMREG2_DATA );
}
