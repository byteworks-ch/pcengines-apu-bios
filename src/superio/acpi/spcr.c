/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Sage Electronic Engineering, LLC.
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

/*
 * ACPI - create the Serial Port Console Redirection Table (SPCR)
 */


#include <string.h>
#include <console/console.h>
#include <arch/acpi.h>
#include <arch/io.h>
#include <device/device.h>

#if IS_ENABLED(CONFIG_GENERATE_SPCR_ACPI_TABLE)

#if IS_ENABLED(CONFIG_CONSOLE_SERIAL_115200)
#define	SPCR_BAUD	SPCR_115200_BAUD
#elif IS_ENABLED(CONFIG_CONSOLE_SERIAL_57600)
#define	SPCR_BAUD	SPCR_57600_BAUD
#elif IS_ENABLED(CONFIG_CONSOLE_SERIAL_19200)
#define SPCR_BAUD	SPCR_19200_BAUD
#elif IS_ENABLED(CONFIG_CONSOLE_SERIAL_9600)
#define SPCR_BAUD	SPCR_9600_BAUD
#else
#error "ERROR: Unknown baud rate.  Please select 115200, 57600, 19200, or 9600."
#endif

#if IS_ENABLED(CONFIG_DRIVERS_UART_OXPCIE)
/*
 * Support for Oxford OXPCIe952 serial port PCIe cards.
 */

#define SPCR_BAR_TYPE ACPI_ADDRESS_SPACE_MEMORY

//TODO - fix the oxford driver - this base address is... not well done.
#define SPCR_BAR_ADDRESS_LOW CONFIG_OXFORD_OXPCIE_BASE_ADDRESS + 0x1000

#define SPCR_PCI_DID 0xc158 //TODO - this needs to be determined runtime
#define SPCR_PCI_VID 0x1415
#define SPCR_PCI_BUS CONFIG_OXFORD_OXPCIE_BRIDGE_BUS
#define SPCR_PCI_DEV CONFIG_OXFORD_OXPCIE_BRIDGE_DEVICE
#define SPCR_PCI_FUNC CONFIG_OXFORD_OXPCIE_BRIDGE_FUNCTION
#define SPCR_PCI_FLAGS SPCR_PCI_FLAG_NONE
#define SPCR_INTERRUPT_TYPE SPCR_NO_INTERRUPT_SUPPORTED
#define SPCR_IRQ SPCR_NO_IRQ
#elif IS_ENABLED(CONFIG_DRIVERS_UART_8250IO)
/*
 * Use a standard 8250/16450/16550 uart configuration
 */

#define SPCR_BAR_TYPE ACPI_ADDRESS_SPACE_IO
#define SPCR_BAR_ADDRESS_LOW CONFIG_TTYS0_BASE

#define SPCR_PCI_DID SPCR_DID_NOT_A_PCI_DEVICE
#define SPCR_PCI_VID SPCR_VID_NOT_A_PCI_DEVICE
#define SPCR_PCI_BUS SPCR_BUS_NOT_A_PCI_DEVICE
#define SPCR_PCI_DEV SPCR_DEV_NOT_A_PCI_DEVICE
#define SPCR_PCI_FUNC SPCR_FUNC_NOT_A_PCI_DEVICE
#define SPCR_PCI_FLAGS SPCR_PCI_FLAG_NOT_A_PCI_DEVICE

#define SPCR_INTERRUPT_TYPE SPCR_PIC_INTERRUPT_SUPPORTED

//TODO - figure out a better way than just assuming the irqs
#if CONFIG_UART_FOR_CONSOLE == 0
#define SPCR_IRQ 4
#elif CONFIG_UART_FOR_CONSOLE == 1
#define SPCR_IRQ 3
#elif CONFIG_UART_FOR_CONSOLE == 2
#define SPCR_IRQ 4
#elif CONFIG_UART_FOR_CONSOLE == 3
#define SPCR_IRQ 3
#else
#pragma message "WARNING:  SPCR IRQ is not known.  Defaulting to NONE."
#define SPCR_IRQ SPCR_NO_IRQ
#endif

#else
#error "ERROR: unknown uart type"
#endif


/*
 * Reference Serial Port Console Redirection Table
 * Version 1.00 - January 11, 2002
 *
 * http://msdn.microsoft.com/en-us/windows/hardware/gg487465
 */
void acpi_create_spcr(acpi_spcr_t *spcr)
{
	acpi_header_t *header = &(spcr->header);

	/* Prepare the header */
	memset((void *)spcr, 0, sizeof(acpi_spcr_t));
	memcpy(header->signature, SPCR_SIG, 4);
	header->length = sizeof(acpi_spcr_t);
	header->revision = ACPI_SPCR_REV;
	memcpy(header->oem_id, OEM_ID, 6);
	memcpy(header->oem_table_id, ACPI_TABLE_CREATOR, 8);
	memcpy(header->asl_compiler_id, ASLC, 4);
	header->asl_compiler_revision = 0;

	/* TODO: defaulting to 16550 type for now */
	spcr->interface_type = SPCR_16550_INTERFACE;
	spcr->reserved_1 = SPCR_RESERVED_MUST_BE_ZERO;
	spcr->reserved_2 = SPCR_RESERVED_MUST_BE_ZERO;
	spcr->reserved_3 = SPCR_RESERVED_MUST_BE_ZERO;

	spcr->base_address.space_id = SPCR_BAR_TYPE;
	spcr->base_address.bit_width = 8;
	spcr->base_address.bit_offset = 0;
	spcr->base_address.access_size = ACPI_ACCESS_SIZE_UNDEFINED;
	spcr->base_address.addrl = SPCR_BAR_ADDRESS_LOW;
	spcr->base_address.addrh = 0x0;

	spcr->interrupt_type = SPCR_INTERRUPT_TYPE;
	spcr->irq = SPCR_IRQ;
	spcr->global_system_interrupt = SPCR_NO_GSI;
	spcr->baud_rate = SPCR_BAUD;
	spcr->parity = SPCR_NO_PARITY;
	spcr->stop_bits = SPCR_1_STOP_BIT;
	spcr->flow_control = SPCR_NO_FLOW_CONTROL;
	spcr->terminal_type = SPCR_TERM_VT100; //TODO - what do we want?

	spcr->reserved_4 = SPCR_RESERVED_MUST_BE_ZERO;
	spcr->pci_device_id = SPCR_PCI_DID;
	spcr->pci_vendor_id = SPCR_PCI_VID;
	spcr->pci_bus_number = SPCR_PCI_BUS;
	spcr->pci_device_number = SPCR_PCI_DEV;
	spcr->pci_function_number = SPCR_PCI_FUNC;
	spcr->pci_flags = SPCR_PCI_FLAGS;
	spcr->pci_segment = SPCR_PCI_SEGMENT_0;
	spcr->reserved_5 = SPCR_RESERVED_MUST_BE_ZERO;

	header->checksum = acpi_checksum((void *)spcr, sizeof(acpi_spcr_t));
}
#endif /* IS_ENABLED(CONFIG_GENERATE_ACPI_TABLES) */
