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

#ifndef __SEABIOS_STDINT_H
#define __SEABIOS_STDINT_H

#include "types.h"

/* minimal stdint types for seabios non-specific portability */

typedef u8       uint8_t;
typedef s8       int8_t;

typedef u16      uint16_t;
typedef s16      int16_t;

typedef u32      uint32_t;
typedef s32      int32_t;

typedef u64      uint64_t;
typedef s64      int64_t;

#endif /* __SEABIOS_STDINT_H */
