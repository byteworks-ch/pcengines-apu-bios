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

/* variables */
u32 gpio_mmio_base = 0;

/*===========================================================
 * This function finds the GPIO MMIO base address
 *=========================================================*/
void gpio_init_base_address( void ) {
	u8  pm_index;
	/* Find the ACPImmioAddr base address */
	for ( pm_index = PMREG_GPIO_HI; pm_index >= PMREG_GPIO_LO; pm_index-- ) {
		gpio_mmio_base = gpio_mmio_base << 8;
		gpio_mmio_base |= (unsigned long int)pmreg_read( pm_index );
	}
	gpio_mmio_base &= GPIO_BASE_MASK;
}

/*===========================================================
 * This ftn reads a GPIO pin. This function doesn't depend
 * on the GPIO being either a input or output.
 *=========================================================*/
u8 gpio_read( int gpiox )
{
	if (gpio_mmio_base == 0) {
		printf("gpio_mmio_base is not initialized\n");
		return 0;
	}
	return ((memory_read_byte(gpio_mmio_base + GPIO_OFFSET + gpiox) & 0x80) >> 7);
}

/*===========================================================
 * This ftn writes a GPIO output value. The GPIO must be
 * configured to be an output elsewhere
 *=========================================================*/
void gpio_write( int value, int gpiox )
{
	u8 bdata;

	if (gpio_mmio_base == 0) {
		printf("gpio_mmio_base is not initialized\n");
		return;
	}

	bdata = memory_read_byte( gpio_mmio_base + GPIO_OFFSET + gpiox );
	bdata &= 0x3F;
	bdata |= (value << 6);
	memory_write_byte( gpio_mmio_base + GPIO_OFFSET + gpiox, bdata );
}

/*===========================================================
 * this ftn turns on a gpios pullup or pulldown
 *=========================================================*/
void gpio_pull( int mode, int gpio )
{
	u8 bdata; /* mode: 1=pullup 0=pulldown */

	if (gpio_mmio_base == 0) {
		printf("gpio_mmio_base is not initialized\n");
		return;
	}

	bdata = memory_read_byte( gpio_mmio_base + GPIO_OFFSET + gpio );
	bdata &= 0xE7; /* mask up/down bits */
	if (mode == 1)    /* pullup */
		bdata |= 0x00;
	else            /* pulldown */
		bdata |= 0x18;
	memory_write_byte( gpio_mmio_base + GPIO_OFFSET + gpio, bdata );
}

/*===========================================================
 * This ftn configures a particular GPIOs IOMUX setting for
 * either its primary (0) or alternate usages
 *=========================================================*/
void gpio_configure_iomux( int function, int gpiox )
{
	if (gpio_mmio_base == 0 ) {
		printf("gpio_mmio_base is not initialized\n");
		return;
	}
	memory_write_byte( gpio_mmio_base + IOMUX_OFFSET + gpiox, (u8)function );
}

/*===========================================================
 * This ftn configures a GPIO to be either and input
 * or output. mode: 1=GPIO_IN 0=GPIO_OUT
 *========================================================*/
int gpio_configure( int mode, int gpiox )
{
	u8 bdata;

	if (gpio_mmio_base == 0) {
		printf("gpio_mmio_base is not initialized\n");
		return 1;
	}

	bdata = memory_read_byte( gpio_mmio_base + GPIO_OFFSET + gpiox );
	if (bdata & 0x01)
		printf("ERROR: GPIO is owned by IMC!\n");
	bdata &= 0x07; /* clr bits */
	if (mode == GPIO_OUT)
		bdata |= 0x08; /* set bits */
	else
		bdata |= 0x28; /* set bits */
	memory_write_byte( gpio_mmio_base + GPIO_OFFSET + gpiox, bdata);
	return 0;
}
