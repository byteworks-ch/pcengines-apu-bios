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

#ifndef __SDHC_GENERIC_H
#define __SDHC_GENERIC_H

/** @file sdhc_generic.h */
/*
 * @brief SD PCI host controller driver header file.  This driver is intended
 *           to be a generic driver for use with booting from SD cards. It
 *           only supports the minimum controls necessary to boot.
 */
#include <stdint.h>
#include "block.h"
#include "config.h"
#include "sd.h"


bool sdhc_init( sdHc_t* pSdCtrl );
void sdhc_prepBoot( sdHc_t* pSdCtrl );
bool sdhc_isInitialized( sdHc_t* pSdCtrl );

#endif /* __SDHC_GENERIC_H */
