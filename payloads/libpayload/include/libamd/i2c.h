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

#include <x86/arch/types.h>

/* defines */
#define PMREG_SM_HI   0x2D
#define PMREG_SM_LO   0x2C
#define SM_BASE_MASK  0xFFE0
#define MAX_I2C_LOOPS 1000000
#define I2C_1_BYTE    0x44
#define I2C_2_BYTES   0x48

#define SMBUS_STATUS        0
#define SMBUS_SLAVE_STATUS  1
#define SMBUS_CONTROL       2
#define SMBUS_HOST_COMMAND  3
#define SMBUS_ADDRESS       4
#define SMBUS_DATA0         5
#define SMBUS_DATA1         6

#define SMBUS_STATUS_HOSTBUSY 0x01

/* prototypes */
void i2c_init_base_address( void );
u8 i2c_tx_bytes(u8 i2c_addr, u8 cntrl, u8 data, u8 cmd);
u8 i2c_rx_bytes(u8 i2c_addr, u8 *data, u8 cmd);
