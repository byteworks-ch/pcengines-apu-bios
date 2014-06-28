//*****************************************************************************
//
//
// Copyright (c) 2012 Sage Electronic Engineering.  All rights reserved.
// Software License Agreement
//
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// Sage Electronic Engineering SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR
// SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************

#ifndef __BAR_H
#define __BAR_H

#include <stdint.h>

// write functions
static inline void barWrite8( uint32_t bar, uint32_t offset, uint8_t value )
{
	volatile uint8_t* pReg = (volatile uint8_t*)(bar | ( offset & 0xFF ) );
	*pReg = value;
}

static inline void barWrite16( uint32_t bar, uint32_t offset, uint16_t value )
{
	volatile uint16_t* pReg = (volatile uint16_t*)(bar | ( offset & 0xFF ) );
	*pReg = value;
}

static inline void barWrite32( uint32_t bar, uint32_t offset, uint32_t value )
{
	volatile uint32_t* pReg = (volatile uint32_t*)(bar | ( offset & 0xFF ) );
	*pReg = value;
}

// read functions
static inline uint8_t barRead8( uint32_t bar, uint32_t offset )
{
	volatile uint8_t* pReg = (volatile uint8_t*)(bar | ( offset & 0xFF ) );
	return( *pReg );
}

static inline uint16_t barRead16( uint32_t bar, uint32_t offset )
{
	volatile uint16_t* pReg = (volatile uint16_t*)(bar | ( offset & 0xFF ) );
	return( *pReg );
}

static inline uint32_t barRead32( uint32_t bar, uint32_t offset )
{
	volatile uint32_t* pReg = (volatile uint32_t*)(bar | ( offset & 0xFF ) );
	return( *pReg );
}

#endif /* __BAR_H */
