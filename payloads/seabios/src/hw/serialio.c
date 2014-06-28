// Low-level serial (and serial-like) device access.
//
// Copyright (C) 2008-1013  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2013 Sage Electronic Engineering, LLC
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "config.h" // CONFIG_DEBUG_SERIAL
#include "fw/paravirt.h" // RunningOnQEMU
#include "output.h" // dprintf
#include "serialio.h" // serial_debug_preinit
#include "x86.h" // outb
#include "hw/pci.h"
#include "hw/pci_ids.h"
#include "hw/pci_regs.h"

/****************************************************************
 * Serial port debug output
 ****************************************************************/

#define DEBUG_TIMEOUT 100000

// Setup the debug serial port for output.
void
serial_debug_preinit(void)
{
    if (!CONFIG_DEBUG_SERIAL)
        return;
    // setup for serial logging: 8N1
    u8 oldparam, newparam = 0x03;
    oldparam = inb(CONFIG_DEBUG_SERIAL_PORT+SEROFF_LCR);
    outb(newparam, CONFIG_DEBUG_SERIAL_PORT+SEROFF_LCR);
    // Disable irqs
    u8 oldier, newier = 0;
    oldier = inb(CONFIG_DEBUG_SERIAL_PORT+SEROFF_IER);
    outb(newier, CONFIG_DEBUG_SERIAL_PORT+SEROFF_IER);

    if (oldparam != newparam || oldier != newier)
        dprintf(1, "Changing serial settings was %x/%x now %x/%x\n"
                , oldparam, oldier, newparam, newier);
}

// Write a character to the serial port.
static void
serial_debug(char c)
{
    if (!CONFIG_DEBUG_SERIAL)
        return;
    int timeout = DEBUG_TIMEOUT;
    while ((inb(CONFIG_DEBUG_SERIAL_PORT+SEROFF_LSR) & 0x20) != 0x20)
        if (!timeout--)
            // Ran out of time.
            return;
    outb(c, CONFIG_DEBUG_SERIAL_PORT+SEROFF_DATA);
}

void
serial_debug_putc(char c)
{
    if (c == '\n')
        serial_debug('\r');
    serial_debug(c);
}

// Make sure all serial port writes have been completely sent.
void
serial_debug_flush(void)
{
    if (!CONFIG_DEBUG_SERIAL)
        return;
    int timeout = DEBUG_TIMEOUT;
    while ((inb(CONFIG_DEBUG_SERIAL_PORT+SEROFF_LSR) & 0x60) != 0x60)
        if (!timeout--)
            // Ran out of time.
            return;
}


/****************************************************************
 * QEMU debug port
 ****************************************************************/

u16 DebugOutputPort VARFSEG = 0x402;

// Write a character to the special debugging port.
void
qemu_debug_putc(char c)
{
    if (CONFIG_DEBUG_IO && runningOnQEMU())
        // Send character to debug port.
        outb(c, GET_GLOBAL(DebugOutputPort));
}

/****************************************************************
 * I2C Console
 ****************************************************************/

/* variables/struct used by i2c console */
/* TODO: These are defined as global in order to get it to build.
 *       They are only used locally.
 */
#define SMB_BFD_0_20_0 0xA0 /* the AMD SMB controller is on bus=0 device=20 ftn=0 */
#define BASE_IN_PM_SPACE 1
#define BASE_IN_PCI_SPACE 2

int i2c_amd_iobase VARFSEG = 0;
struct amd_i2c_list{
    u16 sb_vendor;
    u16 sb_device;
    u8  sb_revision;
    u16  sb_access; /* 1=PM_reg 2=PCI_cfg */
};
struct amd_i2c_list amd_smbus_table[] VARFSEG = {
    { PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, 0x3C, BASE_IN_PCI_SPACE }, /* SB700 */
    { PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SBX00_SMBUS, 0x42, BASE_IN_PM_SPACE },  /* SB800 */
    { PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_SB900_SM,    0x14, BASE_IN_PM_SPACE },  /* SB900 */
};

void i2c_debug_preinit(void)
{
	if (!CONFIG_DEBUG_I2C)
		return;

	u16 i, sb_base_access = 0;
	u16 rd_vendor, rd_device, rd_revision;

	rd_vendor = pci_config_readw(SMB_BFD_0_20_0, PCI_VENDOR_ID);
	rd_device = pci_config_readw(SMB_BFD_0_20_0, PCI_DEVICE_ID);
	rd_revision = pci_config_readw(SMB_BFD_0_20_0, 0x08);
	/* check for the AMD/ATI southbridges */
	for ( i = 0 ; i < sizeof(amd_smbus_table)/sizeof(amd_smbus_table[0]); ++i) {
		if (amd_smbus_table[i].sb_vendor == rd_vendor) {
			if (amd_smbus_table[i].sb_device == rd_device) {
				if((amd_smbus_table[i].sb_revision & 0xf8) == (rd_revision & 0xf8)) {
					sb_base_access = amd_smbus_table[i].sb_access;
					break;
				}
			}
		}
	}

	switch (sb_base_access) {
	case BASE_IN_PM_SPACE:
		if (CONFIG_DEBUG_I2C_BUS == 0)
			i = 0x2C; /* smbus pm index */
		else if (CONFIG_DEBUG_I2C_BUS == 1)
			i = 0x28; /* ASF pm index */
		else
			break;
		outb (i + 1, 0xCD6);
		i2c_amd_iobase = inb(0xCD7) << 8;
		outb (i, 0xCD6);
		i2c_amd_iobase |= inb(0xCD7);
		break;
	case BASE_IN_PCI_SPACE:
		i2c_amd_iobase = pci_config_readw(SMB_BFD_0_20_0, 0x90);
		break;
	default:
		i2c_amd_iobase = 0;
		break;
	}

	if ((i2c_amd_iobase != 0) && ((i2c_amd_iobase & 0x01) == 0x01)) {
		i2c_amd_iobase &= 0xffe0; /* Found a valid address */
	}
	else { /* can not find a valid base address */
		i2c_amd_iobase = 0;
	}
	/* add checks for other vendors SMBUS controllers here */
}

/* send a debug character to the i2c port */
void i2c_debug_putc(char c)
{
	/* check to see if we already tried init and it failed */
	if (!CONFIG_DEBUG_I2C || (i2c_amd_iobase == 0))
		return;

	/* this sequence will send out addr/data on the AMD SBx00 smbus */
	/* check to see if the h/w is idle */
	int timeout = DEBUG_TIMEOUT;
	do {
		if (!timeout--)
			return; // Ran out of time.
	} while ((inb(i2c_amd_iobase) & 0x01) != 0x00);

	outb (0xFF, i2c_amd_iobase + 0);     // clear error status
	outb (0x1F, i2c_amd_iobase + 1);     // clear error status
	outb (c, i2c_amd_iobase + 3);     // tx index
	outb (CONFIG_DEBUG_I2C_DEVICE_ADDR, i2c_amd_iobase + 4);  // slave address and write bit
	outb (0x44, i2c_amd_iobase + 2);     // command it to go
	/* check to see if the h/w is done */
	do {
		if (!timeout--)
			return; // Ran out of time.
	} while ((inb(i2c_amd_iobase) & 0x01) != 0x00);
}
