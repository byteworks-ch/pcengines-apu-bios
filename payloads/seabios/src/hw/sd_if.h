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

#ifndef __SD_IF_H
#define __SD_IF_H

#include <stdint.h>
#include "block.h"
#include "config.h"
#include "pci.h"
#include "sd.h"

/** @file sd_if.h*/

// SeaBIOS to SD driver interface (to allow portability for reuse of sd driver

typedef struct
{
    struct drive_s          drive;
    int32_t                 bootPriority;
    const char*             desc;
    struct pci_device*      pPci;
    sdHc_t*                 pHostCtrl;
}sdDiskIf_t;

void sd_setup (void );
int sd_cmd_data( struct disk_op_s *op, void *cdbcmd, uint16_t blocksize );
int process_sd_op( struct disk_op_s *op );
void SD_DEBUG(const char* func, unsigned int line);
#define SD_STOP( ) SD_DEBUG( __FUNCTION__, __LINE__ )
#endif // __SD_IF_H
