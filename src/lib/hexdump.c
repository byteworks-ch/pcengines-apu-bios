/*
 * Copyright 2013 Google Inc.
 * Copyright 2014 Sage Electronic Engineering, LLC.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <console/console.h>
#include <lib.h>

/*
 * Return false if the character is not printable.
 */
static int isprint(int c)
{
	return (c >= ' ' && c <= '~');
}

/*
 * Log memory contents at a specified print level
 */
void hexdump(int8_t level, const void *memory, size_t length)
{
	uint32_t i;
	uint8_t *m;
	uint32_t all_zero = 0;
	uint32_t all_ff = 0;

	m = (uint8_t *)memory;

	printk(level, "Displaying memory from %p to %p\n",m,m+length);
	/* Loop through memory contents to the length requested */
	for (i = 0; i < length; i += 16) {
		uint8_t j;

		/* See if the line is all 0s or 0xff so we can filter them */
		all_zero++;
		all_ff++;
		for (j = 0; j < 16; j++) {
			if (i + j >= length)
				break;
			if (m[i + j] != 0)
				all_zero = 0;
			if (m[i + j] != 0xff)
				all_ff = 0;
			if (all_ff + all_zero == 0)
				break;
		}

		/*
		 * Decide whether to print out the line or not:
		 * - Do not filter lines that are not all 0x00 or 0xff
		 * - Do not filter the first lines of all 0x00 or 0xff
		 * - Do not filter the last line, even if it is all 0x00 or 0xff
		 */
		if (((all_zero < 2) && (all_ff < 2)) || (i + 16 >= length)) {
			printk(level, "%08lx:", (unsigned long)memory + i);
			for (j = 0; j < 16; j++) {
				/* don't read past the requested length */
				if (i + j < length)
					printk(level, " %02x", m[i+j]);
				else
					printk(level, "   ");
			}
			printk(level, "  ");
			for (j = 0; j < 16; j++) {
				if (i + j < length)
					printk(level, "%c", isprint(m[i+j]) ? m[i+j] : '.');
				else
					printk(level, " ");
			}
			printk(level, "\n");
		} else if (all_zero == 2) { /* Display a notice on the 2nd line of zeros */
			printk(level, "          00 ...\n");
		} else if (all_ff == 2) { /* Display a notice on the 2nd line of 0xFFs */
			printk(level, "          ff ...\n");
		}
	}
}

void hexdump32(char LEVEL, const void *d, size_t len)
{
	int count = 0;

	while (len > 0) {
		if (count % 8 == 0) {
			printk(LEVEL, "\n");
			printk(LEVEL, "%p:", d);
		}
		printk(LEVEL, " 0x%08lx", *(unsigned long *)d);
		count++;
		len--;
		d += 4;
	}

	printk(LEVEL, "\n\n");
}
