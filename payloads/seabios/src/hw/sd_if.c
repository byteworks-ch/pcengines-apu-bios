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

#include "stacks.h"
#include "string.h"
#include "malloc.h"
#include "util.h"
#include "output.h"
#include "biosvar.h"
#include "pci.h"
#include "pci_ids.h"
#include "std/disk.h"
#include "bar.h"
#include "sdhci.h"
#include "sdhc_generic.h"
#include "sd_if.h"


sdDiskIf_t* g_pDev;

static int sd_disk_read( struct disk_op_s* op );
static int sd_disk_read_aligned( struct disk_op_s* op );

/**
 * @brief    sd_disk_init - finalize the SeaBIOS drive initialization and register the drive
 *
 * @param    sdDiskIf_t* pSdIf - pointer to sd disk interface structure
 *
 * @return   none
 */
static void sd_disk_init( sdDiskIf_t* pSdIf )
{
    pSdIf->drive.blksize = pSdIf->pHostCtrl->blkSize;
    pSdIf->drive.type = DTYPE_SD;
    pSdIf->drive.cntl_id = 0;  // @TODO: Presently only one SD card is supported at a time!
    pSdIf->drive.removable = false;
    pSdIf->drive.translation = TRANSLATION_LBA;
    // shift by 9 (divide by 512 block size)
    pSdIf->drive.sectors = pSdIf->pHostCtrl->pCard->decode.csdDecode.capacity >> 9;
    dprintf( DEBUG_HDL_SD, "SD: num sectors: %d\n", (uint32_t)pSdIf->drive.sectors );

    // generate host vendor/spec string
    pSdIf->desc = znprintf( MAXDESCSIZE,
                          "SD Card Vendor ID: %d",
                          pSdIf->pHostCtrl->hostVendorId );

    pSdIf->bootPriority = bootprio_find_pci_device( (struct pci_device*)pSdIf->pPci );
    dprintf( DEBUG_HDL_SD, "SD card boot priority: 0x%08x\n", pSdIf->bootPriority );

    // register the device as a hard disk
    boot_add_hd(&pSdIf->drive, pSdIf->desc, (int)pSdIf->bootPriority );
}

/**
 * @brief    sd_host_setup - setup the host controller driver, if the host
 *                 is successfully initialized, setup the sd card initialization
 *
 * @param    sdDiskIf_t* pSdIf - pointer to sd disk interface structure
 *
 * @return   bool true if the card was successfully initialized and prepared for boot
 *                  fasle otherwise
 */
static bool sd_host_setup( sdDiskIf_t* pSdIf)
{
    bool status = false;

    // perform the sd card initialization sequence
    if( sdhc_init( pSdIf->pHostCtrl ) )
    {
        // if the card passes initialization and goes to standby mode, it is ready for boot, so setup the disk info
        if( sdCardBusInit( pSdIf->pHostCtrl ) )
        {
            sd_disk_init( pSdIf );

            // the card is now enumerated, prepare it for boot (operational mode)
            sdhc_prepBoot( pSdIf->pHostCtrl );
            status = true;
        }
    }
    return status;
}

/**
 * @brief    sd_card_detect - check to see if the card detect indicates that a card is present
 *                 if there is a card, setup the host and intialize the underlying card
 *
 * @param    sdDiskIf_t* pSdIf - pointer to sd disk interface structure
 *
 * @return   bool status of the card detect bit
 */
static bool sd_card_detect( sdDiskIf_t* pSdIf )
{
    bool status = false;
    uint32_t regVal32 = 0;

    // check to see if the card is present
    regVal32 = barRead32(pSdIf->pHostCtrl->barAddress, SDHCI_PRESENT_STATE);
    status = (regVal32 & SDHCI_CARD_PRESENT) == SDHCI_CARD_PRESENT ? true : false;

    // if the card is present, register it in the boot sequence
    if( status )
    {
        status = sd_host_setup( pSdIf );
    }

    return status;
}


/**
 * @brief    sd_config_setup - setup the sd host controller driver and allocate resources.
 *
 * @param    struct pci_device* pci - pointer to the sdhci controller pci device.
 *
 * @return   none
 */
static void sd_config_setup(struct pci_device* pci)
{
    dprintf(6, "sd_config_setup: 0x%04x\n", pci->bdf );

    // allocate the pci to sd interface structure
    g_pDev = (sdDiskIf_t*)malloc_fseg( sizeof(*g_pDev) );
    if( !g_pDev )
    {
        warn_noalloc();
        return;
    }
    memset( g_pDev, 0, sizeof( *g_pDev ) );

    // allocate the host controller
    g_pDev->pHostCtrl = (sdHc_t*)malloc_fseg( sizeof(*g_pDev->pHostCtrl) );
    if( !(g_pDev->pHostCtrl) )
    {
        warn_noalloc();
        free(g_pDev);
        return;
    }
    memset( g_pDev->pHostCtrl, 0, sizeof( *g_pDev->pHostCtrl) );
    g_pDev->pHostCtrl->isInitialized = false;

    // assign the pci device and set up the host controller
    g_pDev->pPci = pci;

    // setup bar0
    g_pDev->pHostCtrl->barAddress = pci_config_readl(g_pDev->pPci->bdf, 0x10) & 0xFFFFFF00;

    // check for card detect
    if( !sd_card_detect(g_pDev) )
    {
        dprintf( DEBUG_HDL_SD, "No SD card detected\n");
        free( g_pDev->pHostCtrl );
        free( g_pDev );
    }

    else
    {
        dprintf( DEBUG_HDL_SD, "SD card is inserted\n");

        // initialize bounce buffer
        if( create_bounce_buf() < 0 )
        {
            warn_noalloc();
            free( g_pDev);
        }
    }
}


/**
 * @brief    sd_scan - seabios function to scan for the sd conroller on the pci bus
 *
 * @param    none
 *
 * @return   none
 */
static void sd_scan(void)
{
    struct pci_device *pci = NULL;
    dprintf(6, "SD: Scanning for SD Host controllers\n" );

    // Scan PCI bus for sd_mmc host controllers
    foreachpci(pci)
    {
        if( pci->class != PCI_CLASS_SYSTEM_SDHCI )
        {
            continue;
        }

        if( pci->prog_if != 1 )
        {
            continue;
        }
        dprintf(6, "Found PCI SDHCI controller\n");

        // setup the sd host controller hardware
        sd_config_setup(pci);
    }
}



/**
 * @brief    sd_setup - seabios function
 *
 * @param    none
 *
 * @return   none
 */
void sd_setup( void )
{
    ASSERT32FLAT();
    if( CONFIG_SD )
    {
        dprintf(3, "init SD drives\n");
        sd_scan();
    }
}

/**
 * @brief    sd_cmd_data - seabios entry point for command/data disk transactions
 *
 * @param    struct disk_op_s *op - pointer to the disk operations structure
 * @param    uint16_t blocksize - the size of the blocks for the block device transactions
 *
 * @return   int disk operation status
 */
int sd_cmd_data( struct disk_op_s *op, void *cdbcmd, uint16_t blocksize )
{
    int retVal = DISK_RET_EPARAM;
    dprintf(3, "sd_cmd_data\n");

    switch( op->command )
    {
        case CMD_READ:
            retVal = sd_disk_read( op );
            break;
        case CMD_WRITE:
            break;
        case CMD_RESET:
        case CMD_ISREADY:
        case CMD_FORMAT:
        case CMD_VERIFY:
        case CMD_SEEK:
            retVal = DISK_RET_SUCCESS;
            break;
        default:
            retVal = DISK_RET_EPARAM;
            break;
    }
    return retVal;
}


/**
 * @brief    sd_disk_read_aligned - read into an aligned buffer a block of data from the sd card
 *
 * @param    struct disk_op_s* op - pointer to the disk operation request
 *
 * @return   int - disk operation status
 */
static int sd_disk_read_aligned( struct disk_op_s* op )
{
    int retVal = DISK_RET_SUCCESS;
    sdDiskIf_t* pSdIf = GET_GLOBAL( g_pDev );
    uint16_t i = 0;
    uint8_t* curPosition = (uint8_t*)op->buf_fl;

    for( i = 0; i < op->count; i++ )
    {
        if( !sdReadSingleBlock( pSdIf->pHostCtrl->pCard, curPosition, (uint32_t)(op->lba + i)) )
        {
            dprintf( DEBUG_HDL_SD, "SD Read Fail\n");
            retVal = DISK_RET_EPARAM;
            break;
        }
        else
        {
            dprintf( DEBUG_HDL_SD, "sd disk %s, lba %6x, count %3x, buf %p, rc %d\n",
                           "read", (u32)op->lba + i, op->count - i, curPosition, retVal);
            curPosition += BLOCK_SIZE8;
        }
    }
    dprintf( DEBUG_HDL_SD, "return from read retval = %u\n", retVal );
    return retVal;
}

/**
 * @brief    sd_disk_read - if the requested buffer is word aligned, performs the disk read operation
 *
 * @param    struct disk_op_s* op - pointer to the disk operation request
 *
 * @return   int - disk operation status
 */
static int sd_disk_read( struct disk_op_s* op )
{
    int retVal = DISK_RET_EPARAM;
    struct disk_op_s localOp;
    uint8_t* alignedBuf = NULL;
    uint8_t* curPosition = NULL;
    uint16_t i = 0;

    // check if the callers buffer is word aligned, if so use it directly
    if( ( (uint32_t)op->buf_fl & 1 ) == 0 )
    {
        dprintf( DEBUG_HDL_SD, "sd read: buffer already alligned\n");
        retVal = sd_disk_read_aligned( op );
    }
    else
    {
        dprintf( DEBUG_HDL_SD, "sd read: unaligned buffer, performing realligend read\n");
        // get access to an aligned buffer for the disk operation
        localOp = *op;
        alignedBuf = GET_GLOBAL( bounce_buf_fl );
        curPosition = op->buf_fl;

        // execute the aligned to unaligned access one operation at a time
        localOp.buf_fl = alignedBuf;
        localOp.count = 1;

        for( i = 0; i < op->count; i++ )
        {
            retVal = sd_disk_read_aligned( &localOp );
            if( retVal )
            {
                dprintf( DEBUG_HDL_SD, "  - aligned read fail\n");
                break;
            }
            memcpy_fl( curPosition, alignedBuf, BLOCK_SIZE8 );
            curPosition += BLOCK_SIZE8;
            localOp.lba++;
        }

    }
    return retVal;
}

/**
 * @brief    process_sd_op - entry point for disk io operations for sea bios
 *
 * @param    struct disk_op_s *op - disk io functions
 *
 * @return   int - disk return value from disk.h
 *
 * @NOTE:    The macro VISIBLE32FLAT is indicitave of a call to 32 bit mode from 16-bit mode it forces the compiler
 *             to prepend the <function name> with <_cfunc32flat_><funciton name>, so in this case if you were to perform
 *             a search for the caller of process_sd_op, you may not find it unless you search for _cfunc32flat_process_sd_op
 */

int VISIBLE32FLAT process_sd_op( struct disk_op_s *op  )
{
    int retVal = DISK_RET_EPARAM;

    // ensure the configuration exists
    if (!CONFIG_SD)
        return 0;

    dprintf( DEBUG_HDL_SD, "Executing SD disk transaction:  %d\r\n", op->command );
    // execute a command
    switch( op->command )
    {
        case CMD_READ:
            dprintf( DEBUG_HDL_SD,
                    "SD CMD_READ: lba: 0x%08x%08x\n", (uint32_t)(op->lba >> 32), (uint32_t)(op->lba) );
            dprintf( DEBUG_HDL_SD, "  op->count = %d\n", op->count );
            retVal = sd_disk_read( op );
            break;
        case CMD_WRITE:
            break;
        case CMD_RESET:
        case CMD_ISREADY:
        case CMD_FORMAT:
        case CMD_VERIFY:
        case CMD_SEEK:
            retVal = DISK_RET_SUCCESS;
            break;
        default:
            op->count = 0;
            retVal = DISK_RET_EPARAM;
            break;

    }
    return retVal;
}

void SD_DEBUG( const char* func, unsigned int line )
{
    volatile uint8_t exit = 0;
    volatile uint32_t i = 0;

    dprintf( DEBUG_HDL_SD, "%s, %d\n", func, line );

    while( !exit )
    {
        i++;
    }
}

