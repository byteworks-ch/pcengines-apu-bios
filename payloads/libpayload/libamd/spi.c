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
#include <libpayload.h>
#include <libamd/gpio.h>
#include <libamd/pmreg.h>
#include <libamd/spi.h>

u32 spi_base = 0;

/*===========================================================
 * This ftn outputs a byte via the SPI controller. Any
 * other CSx or IOMUX is handled elsewhere.
 *==========================================================*/
void spi_output_byte( u8 *dbuf, int bytes )
{
	u8 tmp, cnt;

	if (spi_base == 0)
		spi_init_address();

	/* check for idle */
	while (1) {
		if ( memory_read_byte(spi_base + SPI_CNTRL0_3) & SPI_STATUS_BUSY)
			printf("SPI controller BUSY\n");
		else
			break;
	}

	/* clear the FIFO pointer */
	memory_write_byte(spi_base + SPI_CNTRL0_2, memory_read_byte(spi_base + SPI_CNTRL0_2) | SPI_FIFO_PTR_CLR);

	/* read the FIFO pointer */
	tmp = memory_read_byte(spi_base + SPI_CNTRL1_D) & SPI_FIFO_PTR_MASK;
	if (tmp != 0)
		printf("ERROR: FIFO PTR is %d should be 0\n",tmp);

	/* write the TX/RX byte counters */
	memory_write_byte(spi_base + SPI_CNTRL0_1, bytes - 1);

	/* write the 1st byte of data to the OPCODE reg */
	memory_write_byte(spi_base + SPI_CNTRL0_0, *(dbuf + 0) );

	if ( bytes > 8 )
		printf("ERROR: too many bytes specified [%d]\n",bytes);
	if (bytes > 1) {
		for ( cnt = 1; cnt < bytes; cnt++ )
			memory_write_byte(spi_base + SPI_CNTRL1_C, *(dbuf + cnt) );
	}

	/* read the FIFO pointer */
	tmp = memory_read_byte(spi_base + SPI_CNTRL1_D) & SPI_FIFO_PTR_MASK;
	if ((tmp + 1 ) != bytes)
		printf("ERROR: FIFO PTR is %d should be %d\n",tmp,bytes);

	/* clear the FIFO pointer */
	memory_write_byte(spi_base + SPI_CNTRL0_2, memory_read_byte(spi_base + SPI_CNTRL0_2) | SPI_FIFO_PTR_CLR);

	/* Start the SPI cycle */
	memory_write_byte(spi_base + SPI_CNTRL0_2, memory_read_byte(spi_base + SPI_CNTRL0_2) | SPI_TRANSMIT );
}

u8 spi_read_reg( u8 reg ) {

	if (spi_base == 0)
		spi_init_address();
	return memory_read_byte(spi_base + reg);
}

void spi_write_reg( u8 reg, u8 data ) {

	if (spi_base == 0)
		spi_init_address();
	memory_write_byte(spi_base + reg, data );
}

void spi_init_address( void )
{
	pcidev_t lpc_isa_dev;
	int status;

	/* Find the spi controller base address */
	status = pci_find_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SB700_LPC, &lpc_isa_dev);
	if (!status)
		status = pci_find_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_ATI_SB900_LPC, &lpc_isa_dev);
	if (!status) {
		printf("ERROR: Unable to find LPC bridge. Halting!\n");
		halt();
	}
	spi_base = pci_read_config32( lpc_isa_dev, SPI_CNTRL_BASE_ADDR_REG );
	spi_base &= 0xFFFFFFE0;
}
