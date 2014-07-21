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
#include "hw/rtc.h"
#include "string.h"

/****************************************************************
 * Serial port debug output
 ****************************************************************/

#define DEBUG_TIMEOUT 100000

#if CONFIG_CHECK_CMOS_SETTING_FOR_CONSOLE_ENABLE
int cmos_serial_console_debug_level = 1;
#endif

// Setup the debug serial port for output.
void
serial_debug_preinit(void)
{
    if (!CONFIG_DEBUG_SERIAL)
        return;
#if CONFIG_CHECK_CMOS_SETTING_FOR_CONSOLE_ENABLE
    if (CONFIG_CHECK_CMOS_SETTING_FOR_CONSOLE_ENABLE)
        check_cmos_debug_level();
#endif

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
#if CONFIG_CHECK_CMOS_SETTING_FOR_CONSOLE_ENABLE
    if (CONFIG_CHECK_CMOS_SETTING_FOR_CONSOLE_ENABLE && !cmos_serial_console_debug_level)
        return;
#endif
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

#if CONFIG_CHECK_CMOS_SETTING_FOR_CONSOLE_ENABLE
/*
 * Find the index/offset/width settings of the "debug_level" value in
 * the cmos_layout.bin file in CBFS. Read the setting out of CMOS.
 * If the setiing is zero all serial console output will be prevented.
 */
void VISIBLE32FLAT
check_cmos_debug_level(void)
{
    const char *rom_address = (char *)0xFF000000;
    const char string[] = {"debug_level"};
    u8 cmos_byte_address = 0;
    u8 cmos_byte_remainder = 0;
    u8 cmos_byte_mask = 0;
    u8 i;
    struct cmos_entries *cmos_table;

    // first find where there is data in the rom
    while ( *(u32 *)rom_address == 0xffffffff)
        rom_address = (char *)(((u32)rom_address >> 1) | 0x80000000);

    while (strnicmp(rom_address, string, strlen(string))) {
        rom_address++;
        if (rom_address == (char *)0xffffffff)
            return;
    }

    cmos_table = (struct cmos_entries *)((u32)rom_address - sizeof(struct cmos_entries) + CMOS_MAX_NAME_LENGTH);
    cmos_byte_address = cmos_table->bit / 8;
    cmos_byte_remainder = cmos_table->bit % 8;
    for (i = 0; i < cmos_table->length; i++)
        cmos_byte_mask = (cmos_byte_mask << 1) | 1;
    cmos_serial_console_debug_level = (rtc_read(cmos_byte_address) >> cmos_byte_remainder) & cmos_byte_mask;
}
#endif
