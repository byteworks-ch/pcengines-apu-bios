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
#define PCI_VENDOR_ID_AMD             0x1022
#define PCI_VENDOR_ID_ATI             0x1002
#define PCI_DEVICE_ID_ATI_SB900_LPC   0x780E
#define PCI_DEVICE_ID_ATI_SB700_LPC   0x439D
#define SPI_CNTRL_BASE_ADDR_REG       0xA0

#define SPI_STATUS_BUSY    (1 << 7)
#define SPI_FIFO_PTR_CLR   (1 << 4)
#define SPI_TRANSMIT       (1 << 0)
#define SPI_FIFO_PTR_MASK  0x07

#define SPI_CNTRL0_0  0x00
#define SPI_CNTRL0_1  0x01
#define SPI_CNTRL0_2  0x02
#define SPI_CNTRL0_3  0x03
#define SPI_CNTRL1_C  0x0C
#define SPI_CNTRL1_D  0x0D
#define SPI_ALT_CS    0x1D

/* prototypes */
void spi_output_byte( u8 *dbuf, int bytes );
void spi_init_address( void );
u8 spi_read_reg( u8 reg );
void spi_write_reg( u8 reg, u8 data );
