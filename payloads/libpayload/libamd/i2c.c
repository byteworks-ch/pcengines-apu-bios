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

/* includes */
#include <x86/arch/io.h>
#include <libamd/i2c.h>
#include <libamd/pmreg.h>

u16 i2c_base = 0;

/*===========================================================
 * This function finds the I2C I/O base address
 *=========================================================*/
void i2c_init_base_address( void ) {
	u8  pm_index;
	/* Find the SM controller base address */
	for (pm_index = PMREG_SM_HI; pm_index >= PMREG_SM_LO; pm_index-- ) {
		i2c_base = i2c_base << 8;
		i2c_base |= (unsigned int)pmreg_read( pm_index );
	}
	if ((i2c_base == 0) || ((i2c_base & 0x01) == 0))
		i2c_base = 0;
	i2c_base &= SM_BASE_MASK;
}

/*===========================================================
 * This function sends bytes from the I2C controller
 *=========================================================*/
u8 i2c_tx_bytes(u8 i2c_addr, u8 cntrl, u8 data, u8 cmd) {
	u32 timeout = MAX_I2C_LOOPS;

	/* make sure h/w is idle */
	do {
		if (!timeout--)
			return (inb(i2c_base + SMBUS_STATUS));
	} while (inb(i2c_base + SMBUS_STATUS) & SMBUS_STATUS_HOSTBUSY);

	/* clear status bits */
	outb(0xFF, i2c_base + SMBUS_STATUS);
	outb(0x1F, i2c_base + SMBUS_SLAVE_STATUS);
	/* set the data/control */
	outb(cntrl, i2c_base + SMBUS_HOST_COMMAND);
	outb(data,  i2c_base + SMBUS_DATA0);
	/* set the device address and r/w direction */
	outb( i2c_addr, i2c_base + SMBUS_ADDRESS);
	/* command h/w to go 44h=(addr-control) 48h=(addr-control-data) */
	outb(cmd, i2c_base + SMBUS_CONTROL);

	/* wait until the byte is sent */
	timeout = MAX_I2C_LOOPS;
	do {
		if (!timeout--)
			break;
	} while (inb(i2c_base + SMBUS_STATUS) & SMBUS_STATUS_HOSTBUSY);
	return (inb(i2c_base + SMBUS_STATUS) & 0xfd);
}

/*===========================================================
 * This function receives bytes from the I2C controller
 *=========================================================*/
u8 i2c_rx_bytes(u8 i2c_addr, u8 *data, u8 cmd) {
	u32 timeout;

	/* make sure h/w is idle */
	timeout = MAX_I2C_LOOPS;
	do {
		if (!timeout--)
			return (inb(i2c_base + SMBUS_STATUS));
	} while (inb(i2c_base + SMBUS_STATUS) & SMBUS_STATUS_HOSTBUSY);

	/* clear status bits */
	outb(0xFF, i2c_base + SMBUS_STATUS);
	outb(0x1F, i2c_base + SMBUS_SLAVE_STATUS);
	/* set the device address and r/w direction */
	outb( i2c_addr | 0x01, i2c_base + SMBUS_ADDRESS);
	/* command h/w to go 44h=(addr-control) 48h=(addr-control-data) */
	outb(cmd, i2c_base + SMBUS_CONTROL);

	/* wait until the byte is sent */
	timeout = MAX_I2C_LOOPS;
	do {
		if (!timeout--)
			break;
	} while (inb(i2c_base + SMBUS_STATUS) & SMBUS_STATUS_HOSTBUSY);

	data[0] = inb(i2c_base + SMBUS_DATA0);
	data[1] = inb(i2c_base + SMBUS_DATA0); /* are these just reading the same value? */
	data[2] = inb(i2c_base + SMBUS_DATA0);
	data[3] = inb(i2c_base + SMBUS_DATA0);
	return (inb(i2c_base + SMBUS_STATUS) & 0xfd);
}
