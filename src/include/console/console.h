/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2003 Eric Biederman
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

#ifndef CONSOLE_CONSOLE_H_
#define CONSOLE_CONSOLE_H_

#include <stdint.h>
#include <rules.h>
#include <console/post_codes.h>

#ifndef __ROMCC__
void post_code(u8 value);
#if CONFIG_CMOS_POST_EXTRA
void post_log_extra(u32 value);
struct device;
void post_log_path(struct device *dev);
void post_log_clear(void);
#else
#define post_log_extra(x) do {} while (0)
#define post_log_path(x) do {} while (0)
#define post_log_clear() do {} while (0)
#endif
/* this function is weak and can be overridden by a mainboard function. */
void mainboard_post(u8 value);
void __attribute__ ((noreturn)) die(const char *msg);

#define __CONSOLE_ENABLE__ \
	((ENV_BOOTBLOCK && CONFIG_BOOTBLOCK_CONSOLE) || \
	ENV_ROMSTAGE || ENV_RAMSTAGE || (ENV_SMM && CONFIG_DEBUG_SMI))

#if __CONSOLE_ENABLE__
void console_init(void);
int console_log_level(int msg_level);
int do_printk(int msg_level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void do_putchar(unsigned char byte);

#define printk(LEVEL, fmt, args...)	\
	do { do_printk(LEVEL, fmt, ##args); } while(0)

#else
static inline void console_init(void) {}
static inline int console_log_level(int msg_level) { return 0; }
static inline void printk(int LEVEL, const char *fmt, ...) {}
static inline void do_putchar(unsigned char byte) {}
#endif

#if IS_ENABLED(CONFIG_CHROMEOS)
/* FIXME: Collision of varargs with AMD headers without guard. */
#include <console/vtxprintf.h>
#if __CONSOLE_ENABLE__
void do_vtxprintf(const char *fmt, va_list args);
#else
static inline void do_vtxprintf(const char *fmt, va_list args) {};
#endif
#endif

/* A lot of code still uses print_debug() et al. while use of printk()
 * would be preferred.
 */
#include <console/early_print.h>

#else /* __ROMCC__ */

#include "arch/x86/lib/romcc_console.c"

#endif /* __ROMCC__ */

#endif /* CONSOLE_CONSOLE_H_ */
