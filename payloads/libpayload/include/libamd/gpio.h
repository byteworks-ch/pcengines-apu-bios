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
#define GPIO_OFFSET     0x100
#define IOMUX_OFFSET    0xD00
#define GPIO_OUT        0
#define GPIO_IN         1
#define GPIO_LO         0
#define GPIO_HI         1
#define PMREG_GPIO_HI   0x27
#define PMREG_GPIO_LO   0x24
#define GPIO_BASE_MASK  0xFFFFF000

/* prototypes */
void gpio_init_base_address( void );
u8 gpio_read( int gpiox );
void gpio_write( int value, int gpiox );
void gpio_pull( int mode, int gpio );
void gpio_configure_iomux( int function, int gpiox );
int gpio_configure( int mode, int gpiox );
