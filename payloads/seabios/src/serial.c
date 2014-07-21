// 16bit code to handle serial and printer services.
//
// Copyright (C) 2008,2009  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2002  MandrakeSoft S.A.
// Copyright (C) 2014 Sage Electronic Engineering, Inc.
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "biosvar.h" // SET_BDA
#include "bregs.h" // struct bregs
#include "hw/serialio.h" // SEROFF_IER
#include "output.h" // debug_enter
#include "romfile.h" // romfile_loadint
#include "stacks.h" // yield
#include "util.h" // serial_setup


/****************************************************************
 * COM ports
 ****************************************************************/

static u16
detect_serial(u16 port, u8 timeout, u8 count)
{
    if (CONFIG_DEBUG_SERIAL && port == CONFIG_DEBUG_SERIAL_PORT
        && !romfile_loadint("etc/advertise-serial-debug-port", 1))
        return 0;
    outb(0x02, port+SEROFF_IER);
    u8 ier = inb(port+SEROFF_IER);
    if (ier != 0x02)
        return 0;
    u8 iir = inb(port+SEROFF_IIR);
    if ((iir & 0x3f) != 0x02)
        return 0;

    outb(0x00, port+SEROFF_IER);
    SET_BDA(port_com[count], port);
    SET_BDA(com_timeout[count], timeout);
    return 1;
}

void
serial_setup(void)
{
    if (! CONFIG_SERIAL)
        return;
    dprintf(3, "init serial\n");

    u16 count = 0;
    count += detect_serial(PORT_SERIAL1, 0x0a, count);
    count += detect_serial(PORT_SERIAL2, 0x0a, count);
    count += detect_serial(PORT_SERIAL3, 0x0a, count);
    count += detect_serial(PORT_SERIAL4, 0x0a, count);
    dprintf(1, "Found %d serial ports\n", count);

    // Equipment word bits 9..11 determing # serial ports
    set_equipment_flags(0xe00, count << 9);
}

static u16
getComAddr(struct bregs *regs)
{
    if (regs->dx >= 4) {
        set_invalid(regs);
        return 0;
    }
    u16 addr = GET_BDA(port_com[regs->dx]);
    if (! addr)
        set_invalid(regs);
    return addr;
}

// SERIAL - INITIALIZE PORT
static void
handle_1400(struct bregs *regs)
{
    u16 addr = getComAddr(regs);
    if (!addr)
        return;

    if (CONFIG_KEEP_DEBUG_CONSOLE_SETTINGS) {
        /* Don't let the OS change the baud rate of the debug */
        /* console. Just indicate success and return */
        if (addr == CONFIG_DEBUG_SERIAL_PORT) {
            set_success(regs);
            return;
        }
    }

    outb(inb(addr+SEROFF_LCR) | 0x80, addr+SEROFF_LCR);
    if ((regs->al & 0xE0) == 0) {
        outb(0x17, addr+SEROFF_DLL);
        outb(0x04, addr+SEROFF_DLH);
    } else {
        u16 val16 = 0x600 >> ((regs->al & 0xE0) >> 5);
        outb(val16 & 0xFF, addr+SEROFF_DLL);
        outb(val16 >> 8, addr+SEROFF_DLH);
    }
    outb(regs->al & 0x1F, addr+SEROFF_LCR);
    regs->ah = inb(addr+SEROFF_LSR);
    regs->al = inb(addr+SEROFF_MSR);
    set_success(regs);
}

// SERIAL - WRITE CHARACTER TO PORT
static void
handle_1401(struct bregs *regs)
{
    u16 addr = getComAddr(regs);
    if (!addr)
        return;
    u32 end = irqtimer_calc_ticks(GET_BDA(com_timeout[regs->dx]));
    for (;;) {
        u8 lsr = inb(addr+SEROFF_LSR);
        if ((lsr & 0x60) == 0x60) {
            // Success - can write data
            outb(regs->al, addr+SEROFF_DATA);
            // XXX - reread lsr?
            regs->ah = lsr;
            break;
        }
        if (irqtimer_check(end)) {
            // Timed out - can't write data.
            regs->ah = lsr | 0x80;
            break;
        }
        yield();
    }
    set_success(regs);
}

// SERIAL - READ CHARACTER FROM PORT
static void
handle_1402(struct bregs *regs)
{
    u16 addr = getComAddr(regs);
    if (!addr)
        return;
    u32 end = irqtimer_calc_ticks(GET_BDA(com_timeout[regs->dx]));
    for (;;) {
        u8 lsr = inb(addr+SEROFF_LSR);
        if (lsr & 0x01) {
            // Success - can read data
            regs->al = inb(addr+SEROFF_DATA);
            regs->ah = lsr;
            break;
        }
        if (irqtimer_check(end)) {
            // Timed out - can't read data.
            regs->ah = lsr | 0x80;
            break;
        }
        yield();
    }
    set_success(regs);
}

// SERIAL - GET PORT STATUS
static void
handle_1403(struct bregs *regs)
{
    u16 addr = getComAddr(regs);
    if (!addr)
        return;
    regs->ah = inb(addr+SEROFF_LSR);
    regs->al = inb(addr+SEROFF_MSR);
    set_success(regs);
}

static void
handle_14XX(struct bregs *regs)
{
    set_unimplemented(regs);
}

// INT 14h Serial Communications Service Entry Point
void VISIBLE16
handle_14(struct bregs *regs)
{
    debug_enter(regs, DEBUG_HDL_14);
    if (! CONFIG_SERIAL) {
        handle_14XX(regs);
        return;
    }

    switch (regs->ah) {
    case 0x00: handle_1400(regs); break;
    case 0x01: handle_1401(regs); break;
    case 0x02: handle_1402(regs); break;
    case 0x03: handle_1403(regs); break;
    default:   handle_14XX(regs); break;
    }
}

// XXX - Baud Rate Generator Table
u8 BaudTable[16] VAR16FIXED(0xe729);


/****************************************************************
 * LPT ports
 ****************************************************************/

static u16
detect_parport(u16 port, u8 timeout, u8 count)
{
    // clear input mode
    outb(inb(port+2) & 0xdf, port+2);

    outb(0xaa, port);
    if (inb(port) != 0xaa)
        // Not present
        return 0;
    SET_BDA(port_lpt[count], port);
    SET_BDA(lpt_timeout[count], timeout);
    return 1;
}

void
lpt_setup(void)
{
    if (! CONFIG_LPT)
        return;
    dprintf(3, "init lpt\n");

    u16 count = 0;
    count += detect_parport(PORT_LPT1, 0x14, count);
    count += detect_parport(PORT_LPT2, 0x14, count);
    dprintf(1, "Found %d lpt ports\n", count);

    // Equipment word bits 14..15 determing # parallel ports
    set_equipment_flags(0xc000, count << 14);
}

static u16
getLptAddr(struct bregs *regs)
{
    if (regs->dx >= 3) {
        set_invalid(regs);
        return 0;
    }
    u16 addr = GET_BDA(port_lpt[regs->dx]);
    if (! addr)
        set_invalid(regs);
    return addr;
}

// INT 17 - PRINTER - WRITE CHARACTER
static void
handle_1700(struct bregs *regs)
{
    u16 addr = getLptAddr(regs);
    if (!addr)
        return;

    u32 end = irqtimer_calc_ticks(GET_BDA(lpt_timeout[regs->dx]));

    outb(regs->al, addr);
    u8 val8 = inb(addr+2);
    outb(val8 | 0x01, addr+2); // send strobe
    udelay(5);
    outb(val8 & ~0x01, addr+2);

    for (;;) {
        u8 v = inb(addr+1);
        if (!(v & 0x40)) {
            // Success
            regs->ah = v ^ 0x48;
            break;
        }
        if (irqtimer_check(end)) {
            // Timeout
            regs->ah = (v ^ 0x48) | 0x01;
            break;
        }
        yield();
    }

    set_success(regs);
}

// INT 17 - PRINTER - INITIALIZE PORT
static void
handle_1701(struct bregs *regs)
{
    u16 addr = getLptAddr(regs);
    if (!addr)
        return;

    u8 val8 = inb(addr+2);
    outb(val8 & ~0x04, addr+2); // send init
    udelay(5);
    outb(val8 | 0x04, addr+2);

    regs->ah = inb(addr+1) ^ 0x48;
    set_success(regs);
}

// INT 17 - PRINTER - GET STATUS
static void
handle_1702(struct bregs *regs)
{
    u16 addr = getLptAddr(regs);
    if (!addr)
        return;
    regs->ah = inb(addr+1) ^ 0x48;
    set_success(regs);
}

static void
handle_17XX(struct bregs *regs)
{
    set_unimplemented(regs);
}

// INT17h : Printer Service Entry Point
void VISIBLE16
handle_17(struct bregs *regs)
{
    debug_enter(regs, DEBUG_HDL_17);
    if (! CONFIG_LPT) {
        handle_17XX(regs);
        return;
    }

    switch (regs->ah) {
    case 0x00: handle_1700(regs); break;
    case 0x01: handle_1701(regs); break;
    case 0x02: handle_1702(regs); break;
    default:   handle_17XX(regs); break;
    }
}

#if CONFIG_INT16_SERIAL_KEYBOARD
static u8 UartToScanCode[] VAR16 = {
//    x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xa    xb    xc    xd    xe    xf
//  =====================================================================================================
    0x0f, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, // 0x
    0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x01, 0x00, 0x00, 0x00, 0x00, // 1x
    0x39, 0x02, 0x28, 0x04, 0x05, 0x06, 0x08, 0x28, 0x0a, 0x0b, 0x09, 0x0d, 0x33, 0x0c, 0x34, 0x35, // 2x
    0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x27, 0x27, 0x33, 0x0d, 0x34, 0x35, // 3x
    0x03, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, // 4x
    0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x1a, 0x2b, 0x1b, 0x07, 0x0c, // 5x
    0x29, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, // 6x
    0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x1a, 0x2b, 0x1b, 0x29, 0x00, // 7x
};

void
uart_check_keystrokes(void)
{
    u8 rx_buf[5], rx_bytes = 0, ascii_code = 0, scan_code = 0;

    // check to see if there is a active serial port
    if (inb(CONFIG_DEBUG_SERIAL_PORT + SEROFF_LSR) == 0xFF)
        return;

    while (inb(CONFIG_DEBUG_SERIAL_PORT + SEROFF_LSR) & 0x01) {
        if (rx_bytes > sizeof(rx_buf)) {
            dprintf(1, "uart_check_keystrokes: error too many bytes are available\n");
            while (inb(CONFIG_DEBUG_SERIAL_PORT + SEROFF_LSR) & 0x01)
                inb(CONFIG_DEBUG_SERIAL_PORT + SEROFF_DATA);
            return;
       }
       else
           rx_buf[rx_bytes++] = inb(CONFIG_DEBUG_SERIAL_PORT + SEROFF_DATA);
    }

    if (!rx_bytes)
        return;

    if (rx_bytes == 1) {
        ascii_code = rx_buf[0];
        if (ascii_code >= ARRAY_SIZE(UartToScanCode)) {
            dprintf(3, "uart_check_keystrokes: error UartToScanCode out of bounds index\n");
            return;
        }
        scan_code = GET_GLOBAL(UartToScanCode[ascii_code]);
        enqueue_key(scan_code, ascii_code);
    }
    else if (rx_bytes == 2) { // assume it's actually 2 single-byte keystrokes
        ascii_code = rx_buf[0];
        if (ascii_code >= ARRAY_SIZE(UartToScanCode)) {
            dprintf(3, "uart_check_keystrokes: error in 1/2 byte UartToScanCode out of bounds index\n");
            return;
        }
        scan_code = GET_GLOBAL(UartToScanCode[ascii_code]);
        enqueue_key(scan_code, ascii_code);

        ascii_code = rx_buf[1];
        if (ascii_code >= ARRAY_SIZE(UartToScanCode)) {
            dprintf(3, "uart_check_keystrokes: error in 2/2 UartToScanCode out of bounds index\n");
            return;
        }
        scan_code = GET_GLOBAL(UartToScanCode[ascii_code]);
        enqueue_key(scan_code, ascii_code);
    }
    else if (rx_bytes == 3) {
        if ((rx_buf[0] == 0x1b) && (rx_buf[1] == 0x4f)) { // F1-F12
            ascii_code = 0;
            if ((rx_buf[2] >= 0x50) && (rx_buf[2] <= 0x59))
                scan_code = (rx_buf[2] - 0x50)  +0x3b;
            else if ((rx_buf[2] >= 0x5a) && (rx_buf[2] <= 0x5b))
                scan_code = (rx_buf[2] - 0x5a) + 0x85;
            else {
                dprintf(3, "uart_check_keystrokes: error in Fkey handling %x\n",rx_buf[2]);
                return;
            }
        }
        else if ((rx_buf[0] == 0x1b) && (rx_buf[1] == 0x5b)) { // cursor keys
            ascii_code = 0xe0;
            if      (rx_buf[2] == 0x41) scan_code = 0x48; // UP
            else if (rx_buf[2] == 0x42) scan_code = 0x50; // DOWN
            else if (rx_buf[2] == 0x43) scan_code = 0x4d; // LEFT
            else if (rx_buf[2] == 0x44) scan_code = 0x4b; // RIGHT
            else {
                dprintf(3, "uart_check_keystrokes: error in cursor handling %x\n",rx_buf[2]);
                return;
            }
        }
        else {
            dprintf(3, "uart_check_keystrokes: error in 3 byte key sequence\n");
            return;
        }
        enqueue_key(scan_code, ascii_code);
    }
    else if (rx_bytes == 4) {
        if ((rx_buf[0] == 0x1b) && (rx_buf[1] == 0x5b) && (rx_buf[2] == 0x33) && (rx_buf[3] == 0x7e)) { // DEL
            ascii_code = 0xe0;
            scan_code = 0x53;
            enqueue_key(scan_code, ascii_code);
        }
        else {
            dprintf(3, "uart_check_keystrokes: unhandled 4 byte keystroke ");
            dprintf(3, "%x %x %x %x\n",rx_buf[0],rx_buf[1],rx_buf[2],rx_buf[3]);
            return;
        }
    }
    /* these 5 byte scan codes are used by some terminal emulators */
    else if (rx_bytes == 5) {
        if ((rx_buf[0] == 0x1b) && (rx_buf[1] == 0x5b) && (rx_buf[2] == 0x32) && (rx_buf[4] == 0x7e)) { // F9-F12
            ascii_code = 0x00;
            if      (rx_buf[3] == 0x30) scan_code = 0x43; // F9
            else if (rx_buf[3] == 0x31) scan_code = 0x44; // F10
            else if (rx_buf[3] == 0x33) scan_code = 0x85; // F11
            else if (rx_buf[3] == 0x34) scan_code = 0x86; // F12
            else {
                dprintf(3, "uart_check_keystrokes: unhandled 5 byte keystroke F9-F12 ");
                dprintf(3, "%x %x %x %x %x\n",rx_buf[0],rx_buf[1],rx_buf[2],rx_buf[3],rx_buf[4]);
                return;
            }
        }
        else if ((rx_buf[0] == 0x1b) && (rx_buf[1] == 0x5b) && (rx_buf[2] == 0x31) && (rx_buf[4] == 0x7e)) { // F1-F8
            ascii_code = 0x00;
            if      (rx_buf[3] == 0x31) scan_code = 0x3B; // F1
            else if (rx_buf[3] == 0x32) scan_code = 0x3C; // F2
            else if (rx_buf[3] == 0x33) scan_code = 0x3D; // F3
            else if (rx_buf[3] == 0x34) scan_code = 0x3E; // F4
            else if (rx_buf[3] == 0x35) scan_code = 0x3F; // F5
            else if (rx_buf[3] == 0x37) scan_code = 0x40; // F6
            else if (rx_buf[3] == 0x38) scan_code = 0x41; // F7
            else if (rx_buf[3] == 0x39) scan_code = 0x42; // F8
            else {
                dprintf(3, "uart_check_keystrokes: unhandled 5 byte keystroke F1-F8 ");
                dprintf(3, "%x %x %x %x %x\n",rx_buf[0],rx_buf[1],rx_buf[2],rx_buf[3],rx_buf[4]);
                return;
            }
        }
        else {
            dprintf(3, "uart_check_keystrokes: unhandled 5 byte keystroke ");
            dprintf(3, "%x %x %x %x %x\n",rx_buf[0],rx_buf[1],rx_buf[2],rx_buf[3],rx_buf[4]);
            return;
        }
        enqueue_key(scan_code, ascii_code);
    }
    else {
        dprintf(3, "uart_check_keystrokes: unhandled rx_bytes = %x\n",rx_bytes);
        dprintf(3, "%x %x %x %x %x\n",rx_buf[0],rx_buf[1],rx_buf[2],rx_buf[3],rx_buf[4]);
        return;
    }
}
#endif /* CONFIG_INT16_SERIAL_KEYBOARD */
