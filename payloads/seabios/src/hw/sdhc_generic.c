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

#include <stdint.h>
#include <stdbool.h>
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
#include "sd.h"
#include "sdhc_generic.h"

// host controller functions
static void sdhc_readResponse( sdHc_t* pSdCtrl, sdXfer_t* pXfer );
static bool sdhc_pollIntrStatus( sdHc_t* pSdCtrl, sdXfer_t* pXfer, uint32_t intrNum, uint32_t pollCnt, uint32_t uSecTmo );
static void sdhc_getVerInfo( sdHc_t* pSdCtrl );
static void sdhc_setClock( sdHc_t* pSdCtrl, uint32_t clockVal );
static void sdhc_setPower( sdHc_t* pSdCtrl, uint32_t pwrMode );
static bool sdhc_reset( sdHc_t* pSdCtrl, uint8_t resetFlags );
static void sdhc_readBlock( sdHc_t* pSdCtrl, uint8_t* pBuf, uint32_t count );

// callback function for performing command/response transactions
static bool sdhc_Cmd( sdHc_t* pSdCtrl, sdXfer_t* pXfer );


/**
 * @brief    sdhc_readResponse - helper function to read different response types
 *              based on a transfer request
 *
 * @param    sdHc_t* pSdCtrl - pointer to the host controller abstraction structure
 * @param    sdXfer_t* pXfer - pointer to the host style transfer structure
 *
 * @return   none
 */
static void sdhc_readResponse( sdHc_t* pSdCtrl, sdXfer_t* pXfer )
{
    memset( pXfer->response, 0, sizeof( pXfer->response[0] * 4 ) );
    pXfer->rspValid = false;

    switch( pXfer->rspType )
    {
        case eRsp136:
            pXfer->response[0] = barRead32(pSdCtrl->barAddress, SDHCI_RESPONSE );
            pXfer->response[1] = barRead32(pSdCtrl->barAddress, SDHCI_RESPONSE + 0x04 );
            pXfer->response[2] = barRead32(pSdCtrl->barAddress, SDHCI_RESPONSE + 0x08 );
            pXfer->response[3] = barRead32(pSdCtrl->barAddress, SDHCI_RESPONSE + 0x0C );
            pXfer->rspValid = true;
            break;

        case eRsp48:
            pXfer->response[0] = barRead32(pSdCtrl->barAddress, SDHCI_RESPONSE );
            pXfer->rspValid = true;
            break;

        case eRsp48_busy:
            pXfer->response[0] = barRead32(pSdCtrl->barAddress, SDHCI_RESPONSE + 0x0C );
            pXfer->rspValid = true;
            break;

        case eRspNone:
        default:
            break;
    }
}

/**
 * @brief    sdhc_pollIntrStatus - interrupt status polling routine
 *
 * @param    sdHc_t* pSdCtrll - pointer to the host controller struct
 * @param    uint32_t intrNum - the requested interrupt number(s) to check
 * @param    uint32_t pollCnt - the number of iterations to poll
 * @param    uint32_t uSecTmo - microsecond delay per polling iteration
 *
 * @return   bool - true if the requested interrupt was set during this
 *              request cycle, false otherwise
 */
static bool sdhc_pollIntrStatus( sdHc_t* pSdCtrl, sdXfer_t* pXfer, uint32_t intrNum, uint32_t pollCnt, uint32_t uSecTmo )
{
    bool status = false;
    uint32_t regVal32= 0;

    // check at least once
    if( !pollCnt )
    {
        pollCnt = 1;
    }

    // check for the interrupt flag
    while( pollCnt-- )
    {
        regVal32 = barRead32(pSdCtrl->barAddress, SDHCI_INT_STATUS );
        // check that all requested interrupts were set
        if( ( intrNum & regVal32 ) == intrNum )
        {
            // in the case of response to command, save the response
            if( intrNum == SDHCI_INT_RESPONSE )
            {
                sdhc_readResponse( pSdCtrl, pXfer);
            }

            // clear the interrupt flag(s)
            // @NOTE: some interrupts will not be cleared by writing them to 0
            regVal32 &= ~intrNum;
            barWrite32( pSdCtrl->barAddress, SDHCI_INT_STATUS, regVal32 );
            status = true;
            dprintf( DEBUG_HDL_SD, "SD: requested interrupt occured: %u\n", intrNum );
            break;
        }
        else if( regVal32 & SDHCI_INT_ERROR_MASK )
        {
            // notify debug of timeout error
            if( regVal32 & SDHCI_INT_TIMEOUT )
            {
                dprintf( DEBUG_HDL_SD, "SD: ERROR Timeout\n" );
            }
            // reset the card on fatal errors
            else
            {
                dprintf( DEBUG_HDL_SD,
                    "SD: ERROR interrupt occured, clearing interrupt and resetting card\n" );
                    sdhc_reset( pSdCtrl, SDHCI_RESET_CMD | SDHCI_RESET_DATA );
            }
            barWrite32( pSdCtrl->barAddress, SDHCI_INT_STATUS, ~SDHCI_INT_ERROR_MASK );
            status = false;
            break;
        }

        udelay( uSecTmo );
    }
    // in the case of errors, reset the card and clear out the error interrupts
    dprintf( DEBUG_HDL_SD, "SD: Current interrupt status register: 0x%08x\n", regVal32 );

    return status;
}

/**
 * @brief    sdhc_getVerInfo - read the host controller vendor specific id and host
 *                 sd specification supported by the controller
 *
 * @param    sdHc_t* pSdCtrll - pointer to the host controller struct
 *
 * @return   none
 */
static void sdhc_getVerInfo( sdHc_t* pSdCtrl )
{
    uint16_t regVal16 = 0;

    regVal16 = barRead16( pSdCtrl->barAddress, SDHCI_HOST_VERSION );
    pSdCtrl->hostVendorId =
            (uint8_t)( ( regVal16 & SDHCI_VENDOR_VER_MASK ) >> SDHCI_VENDOR_VER_SHIFT );
    pSdCtrl->hostSpecId =
            (uint8_t)( ( regVal16 & SDHCI_SPEC_VER_MASK ) >> SDHCI_SPEC_VER_SHIFT );
}

/**
 * @brief    sdhc_setPower - setup the power state of the host controller based on its
 *                 reported capabilities.  The first time this is called, the power is setup
 *                 in the highest condition to support all types of cards.  After card enumeration
 *                 the voltage can be lowered.
 *
 * @param     sdHc_t* pSdCtrll - pointer to the host controller abstraction structure
 * @param     uint32_t pwrMode - the requested voltage setting for the power
 *
 * @return    none
 */
static void sdhc_setPower( sdHc_t* pSdCtrl, uint32_t pwrMode )
{
    uint8_t pwr = 0;

    pSdCtrl->pwrMode = pwrMode;

    /* Turn off the POWER. */
    barWrite8(pSdCtrl->barAddress, SDHCI_POWER_CONTROL, pwr);

    if (pwrMode == 0)
        return;

    /* Set voltage. */
    switch( pwrMode )
    {
        case SDHCI_CAN_VDD_180:
            pwr |= SDHCI_POWER_180;
            break;
        case SDHCI_CAN_VDD_300:
            pwr |= SDHCI_POWER_300;
            break;
        case SDHCI_CAN_VDD_330:
            pwr |= SDHCI_POWER_330;
            break;
    }

    barWrite8(pSdCtrl->barAddress, SDHCI_POWER_CONTROL, pwr);
    /* Turn on the POWER. */
    pwr |= SDHCI_POWER_ON;
    barWrite8(pSdCtrl->barAddress, SDHCI_POWER_CONTROL, pwr);
}

/**
 * @brief    sdhc_setClock - set the clock of the host controller.  This function executes the
 *                 clock divisor algorithm described in the "SD Host Controller Simplified
 *                 Specification Version 3.00" to generate the highest clock frequency that the
 *                 card host can handle based on the requested clock.
 *
 * @param    sdHc_t* pSdCtrll - pointer to the host controller abstraction structure
 * @param    uint32_t clockVal - desired clock frequency
 *
 * @return   none.
 */
static void sdhc_setClock( sdHc_t* pSdCtrl, uint32_t clockVal )
{
    uint32_t res = 0;
    uint16_t clk = 0;
    int32_t timeout = 0;

    pSdCtrl->curClk = clockVal;

    // disable the clock
    barWrite16(pSdCtrl->barAddress, SDHCI_CLOCK_CONTROL, 0 );

    // calculate the highest possible frequency <= the maximum clock frequency reported by the card
    res = pSdCtrl->maxClk;
    for (clk = 1; clk < 256; clk <<= 1)
    {
        if( res <= clockVal )
        {
            break;
        }
        res >>= 1;
    }

    // adjust the Divider, 1:1 is 0x00, 2:1 is 0x01, 256:1 is 0x80 ...
    clk >>= 1;
    clk <<= SDHCI_DIVIDER_SHIFT;
    clk |= SDHCI_CLOCK_INT_EN;
    barWrite16( pSdCtrl->barAddress, SDHCI_CLOCK_CONTROL, clk);

    // Wait up to 10 ms until it stabilizes
    timeout = 10;
    while( !((clk = barRead16(pSdCtrl->barAddress, SDHCI_CLOCK_CONTROL) )
            & SDHCI_CLOCK_INT_STABLE) )
    {
        if( timeout == 0 )
        {
            dprintf( DEBUG_HDL_SD, "SD: card internal clock never stabilized\n");
            break;
        }
        timeout--;
        udelay(1000);
    }

    if( timeout > 0)
    {
        // clock is stable so enable the clock signal for the card bus
        dprintf( DEBUG_HDL_SD, "SD: card internal clock stabilized at %u Hz\n", pSdCtrl->curClk );
        clk |= SDHCI_CLOCK_CARD_EN;
        barWrite16( pSdCtrl->barAddress, SDHCI_CLOCK_CONTROL, clk );
    }
}

/**
 * @brief    sdhc_reset - issue the SD reset sequence described in the SD Host Controller Spec V3.00
 *
 * @param    sdHc_t* pSdCtrl - pointer to the host controller struct
 * @param    uint8_t resetFlags - a logical OR mask of the three possible reset types:
 *                                     SDHCI_RESET_ALL - reset entire host controller
 *                                     SDHCI_RESET_CMD - reset command circuit
 *                                     SDHCI_RESET_DATA - reset data & dma circuit
 *
 * @return   bool - true if the request did not timeout, false otherwise
 */
static bool sdhc_reset( sdHc_t* pSdCtrl, uint8_t resetFlags )
{
    uint8_t resetResult = 0;
    uint32_t timeout = 0;
    bool status = false;

    // send the reset command
    barWrite8( (uint32_t)pSdCtrl->barAddress, SDHCI_SOFTWARE_RESET, resetFlags );

    // wait for all of the requested reset flags to clear until the timeout occurs
    timeout = 10;
    while( ( resetResult = barRead8( pSdCtrl->barAddress, SDHCI_SOFTWARE_RESET) & resetFlags ) )
    {
        if( timeout == 0 )
        {
            dprintf( DEBUG_HDL_SD,
                    "SDHC Reset Timeout for reset request type: 0x%02x\n", resetFlags );
        }
        timeout--;
        udelay(100);
    }
    if( timeout > 0)
    {
        dprintf( DEBUG_HDL_SD,
                    "SDHC Reset Successful for reset request type: 0x%02x\n", resetFlags );
        status = true;
    }
    return status;
}

/**
 * @brief    sdhc_Cmd - This is the call-back entry point for the card bus driver to access
 *                 the bus via the host controller.  This function must be registered with the
 *                 card-bus driver in order for it to be able to function properly.  It is the
 *                 entry point for both command and data transfers.
 *
 * @param    sdHc_t* pSdCtrl - pointer to the host controller abstraction
 * @param    sdXfer_t* pXfer - pointer to the command/data transfer, filled out by the
 *                 card-bus driver.
 * @return   bool true on success, false otherwise
 */
static bool sdhc_Cmd( sdHc_t* pSdCtrl, sdXfer_t* pXfer )
{
    uint32_t curState = 0;
    uint32_t stateMsk = 0;
    uint8_t regVal8 = 0;
    uint8_t tmo = 10;
    uint16_t mode = 0;
    bool status = false;
    uint32_t intFlags = 0;

    // setup the state mask for the transfer
    stateMsk = SDHCI_CMD_INHIBIT;
    if( pXfer->rspType == eRsp48_busy )
    {
        stateMsk |= SDHCI_DAT_INHIBIT;
    }

    // wait for the state mask to clear
    while( curState & stateMsk )
    {
        if( tmo == 0 )
        {
            dprintf( DEBUG_HDL_SD,
                    "SD: unable to access cmd bus, its always busy\n");
            break;
        }
        tmo--;
        udelay(1000);
        curState = barRead32( pSdCtrl->barAddress, SDHCI_PRESENT_STATE );
    }

    if( tmo > 0 )
    {
        //Set command argument
        barWrite32(pSdCtrl->barAddress, SDHCI_ARGUMENT, pXfer->arg1);

        if( pXfer->pData && pXfer->xferType == eRdXfer )
        {
            //Set data transfer mode for reading data
            mode = barRead16( pSdCtrl->barAddress, SDHCI_TRANSFER_MODE );
            mode |= ( SDHCI_TRNS_READ );
            mode &= ~( SDHCI_TRNS_MULTI | SDHCI_TRNS_DMA );
            barWrite16(pSdCtrl->barAddress, SDHCI_TRANSFER_MODE, mode );
        }
        else
        {
            //Set data transfer mode for commanding
            mode = barRead16( pSdCtrl->barAddress, SDHCI_TRANSFER_MODE );
            mode &= ~( SDHCI_TRNS_READ | SDHCI_TRNS_MULTI | SDHCI_TRNS_DMA );
            barWrite16(pSdCtrl->barAddress, SDHCI_TRANSFER_MODE, mode );
        }

        // build the command transaction type
        regVal8 = (pSdCtrl->crcCheckEnable) ? SDHCI_CMD_CRC : 0;
        regVal8 |= (pSdCtrl->indexCheckEnable) ? SDHCI_CMD_INDEX : 0;
        regVal8 |= (pXfer->pData != NULL) ? SDHCI_CMD_DATA : 0;
        regVal8 |= (uint8_t)(pXfer->cmdType);
        regVal8 |= (uint8_t)(pXfer->rspType);
        barWrite8( pSdCtrl->barAddress, SDHCI_COMMAND_FLAGS, regVal8 );

        // initiate the transfer
        barWrite8(pSdCtrl->barAddress, SDHCI_COMMAND, pXfer->cmdIdx);

        tmo = 10;
        if( pXfer->rspType != eRspNone )
        {
            intFlags = SDHCI_INT_RESPONSE;
            if( (pXfer->pData) && (pXfer->xferType == eRdXfer) )
            {
                // wait for the transfer to be complete in the case of a read command
                intFlags |= SDHCI_INT_DATA_AVAIL;
            }

            while( !sdhc_pollIntrStatus( pSdCtrl, pXfer, intFlags, 100, 1000) )
            {
                if( tmo == 0 )
                {
                    dprintf( DEBUG_HDL_SD, "SD: failed to receive response to command\n");
                    break;
                }
                tmo--;
            }

            // if this is a read block transaction break out here to get the data
            if( (pXfer->pData) && (pXfer->xferType == eRdXfer) )
            {
                sdhc_readBlock( pSdCtrl, pXfer->pData, BLOCK_SIZE8 );
            }
        }

    }

    status = (tmo > 0);
    return status;
}

/**
 * @brief    sdhc_readBlock - read a block from the SD card, this function is only
 *                 executed if the data pointer and read flag are set in the sdhc_Cmd
 *                 request
 *
 * @param    sdHc_t* pSdCtrl - pointer to the host controller abstraction
 * @param    uint8_t* pBuf - buffer to fill in with data (typically a block)
 * @param    uint32_t count - number of bytes to read in the transaction
 *
 * @return   none
 */
static void sdhc_readBlock( sdHc_t* pSdCtrl, uint8_t* pBuf, uint32_t count )
{
    uint32_t lim = 0;
    uint32_t dataReg = 0;
    uint8_t* pBufLocal = NULL;

    pBufLocal = pBuf;

    //lim = min(BLOCK_SIZE8, count);
    lim = BLOCK_SIZE8;

    // ensure that there is data available
    //usleep(10000);
    // @NOTE: This usleep call was to throttle transactions during development,
    //        it can be commented back in to throttle block read transactions
    //        while modifying the driver
    if( barRead32( pSdCtrl->barAddress, SDHCI_PRESENT_STATE ) & SDHCI_DATA_AVAILABLE )
    {
        dprintf( DEBUG_HDL_SD, "SD: reading %d bytes of data\n", lim );

        // calculate the number of 32 bit values to read by converting the limit to
        // dwords (additional bytes handled later)
        lim >>= 2;
        while( lim > 0 )
        {
            dataReg = barRead32(pSdCtrl->barAddress, SDHCI_BUFFER);
            pBufLocal[0] = (uint8_t)(dataReg);
            pBufLocal[1] = (uint8_t)(dataReg >> 8);
            pBufLocal[2] = (uint8_t)(dataReg >> 16);
            pBufLocal[3] = (uint8_t)(dataReg >> 24);
            pBufLocal += 4;
            lim--;
        }

        // handle the remainder
        lim = count & 0x03;
        if( lim > 0 )
        {
            dataReg = barRead32(pSdCtrl->barAddress, SDHCI_BUFFER);
            while( lim > 0 )
            {
                *(pBufLocal++) = (uint8_t)dataReg;
                dataReg >>= 8;
                lim--;
            }
        }
    }
#if( CONFIG_DEBUG_LEVEL > 9 )
    hexdump( pBuf, BLOCK_SIZE8 );
#endif

}

/**
 * @brief    sdhc_prepBoot - post initialization function to perform changes
 *                 to the operating mode of the card prior to boot, and to
 *                 take it out of enumeration mode.
 *
 * @param    sdHc_t* pSdCtrl - pointer to the host controller struct
 *
 * @return   none
 */
void sdhc_prepBoot( sdHc_t* pSdCtrl )
{
    // boost the clock speed for boot
    //@TODO:  should check if this speed is supported first, (most newer cards do)
    sdhc_setClock( pSdCtrl, 25 * MHZ );

    //@TODO: do other changes to card operating mode here
}

/**
 * @brief   sdhc_isInitialized - check to see if the host controller is initialized
 *
 * @param   sdHc_t* pSdCtrl - pointerto the host controller sturcture
 *
 * @return  bool true if host initialized, false otherwise
 */
bool sdhc_isInitialized( sdHc_t* pSdCtrl )
{
    return pSdCtrl->isInitialized;
}

/**
 * @brief    sdhc_init - performs the minimum necessary steps outlined in the SD
 *                 host controller specification to prepare the sd card/host
 *                 controller for use
 *
 * @param    sdHc_t* pSdCtrl - pointer to the host controller struct
 *
 * @return   bool true if reset succeeded, false otherwise
 */
bool sdhc_init( sdHc_t* pSdCtrl )
{
    uint32_t regVal32 = 0;
    uint8_t regVal8 = 0;

    // reset the the host controller
    if( sdhc_reset( pSdCtrl, SDHCI_RESET_ALL ) )
    {
        // read the capabilities register
        pSdCtrl->cardCapabilities.cap1 =
                barRead32( pSdCtrl->barAddress, SDHCI_CAPABILITIES );
        pSdCtrl->cardCapabilities.cap2 =
                barRead32( pSdCtrl->barAddress, (SDHCI_CAPABILITIES + 4) );

        regVal8 = barRead8(pSdCtrl->barAddress, SDHCI_HOST_CONTROL );
        dprintf( DEBUG_HDL_SD, "SD: Host Control register: 0x%02x\n", regVal8 );

        // check the power
        regVal8 = barRead8(pSdCtrl->barAddress, SDHCI_POWER_CONTROL );
        if( !(regVal8 & SDHCI_POWER_ON) )
        {
            dprintf( DEBUG_HDL_SD, "SD: card currently not powered, power on to");
            // setup the power for the card
            if( pSdCtrl->cardCapabilities.cap1 & SDHCI_CAN_VDD_330 )
            {
                sdhc_setPower( pSdCtrl, SDHCI_CAN_VDD_330 );
                dprintf( DEBUG_HDL_SD, ": 3.3V\n");
            }
            else if( pSdCtrl->cardCapabilities.cap1 & SDHCI_CAN_VDD_300 )
            {
                sdhc_setPower( pSdCtrl, SDHCI_CAN_VDD_300 );
                dprintf( DEBUG_HDL_SD, ": 3.0V\n");
            }
            else if( pSdCtrl->cardCapabilities.cap1 & SDHCI_CAN_VDD_180 )
            {
                sdhc_setPower( pSdCtrl, SDHCI_CAN_VDD_180 );
                dprintf( DEBUG_HDL_SD, ": 1.8V\n");
            }
        }
        else
        {
            dprintf(6, "SD: card bus is powered on\n");
        }

        // determine the base clock frequency reported by the card
        pSdCtrl->maxClk =
                (pSdCtrl->cardCapabilities.cap1 & SDHCI_CLOCK_BASE_MASK)
                >> SDHCI_CLOCK_BASE_SHIFT;
        if( pSdCtrl->maxClk == 0 )
        {
            dprintf( DEBUG_HDL_SD,
                    "SD: no base clock frequency specified by card capabilities\n");

            // @TODO:  If the clock was not set, need to set it ?
        }
        else
        {
            pSdCtrl->maxClk *= MHZ;
            dprintf( DEBUG_HDL_SD, "SD: base clock frequency %u Hz\n", pSdCtrl->maxClk );
        }

        // setup the cards internal clock to always be normal speed mode to blanket support all card types
        // the sd spec defines normal speed mode as 25MHz, and enumeration at 400KHz
        sdhc_setClock( pSdCtrl, 400000 );

        // determine the base timeout frequency
        pSdCtrl->tmoClk = (pSdCtrl->cardCapabilities.cap1 & SDHCI_TIMEOUT_CLK_MASK) >> SDHCI_TIMEOUT_CLK_SHIFT;
        if( pSdCtrl->tmoClk == 0 )
        {
            dprintf( DEBUG_HDL_SD, "SD: no timeout clock frequency specified by card capabilities\n");
        }
        else
        {
            // if the units are specified in MHz adjust the frequency to reflect that
            pSdCtrl->tmoClk =
                    (pSdCtrl->cardCapabilities.cap1 & SDHCI_TIMEOUT_CLK_UNIT)
                    ? pSdCtrl->tmoClk * MHZ : pSdCtrl->tmoClk * KHZ;
            dprintf( DEBUG_HDL_SD, "SD: timeout frequency clock %u\n", pSdCtrl->tmoClk );

            // test max timeout
            barWrite8( pSdCtrl->barAddress, SDHCI_TIMEOUT_CONTROL, 0x0E );
        }

        // the "always supported" block size is 512, so force it
        regVal32 = barRead32( pSdCtrl->barAddress, SDHCI_BLOCK_SIZE );

        // mask off the block bits
        pSdCtrl->blkSize &= BLOCK_MASK;
        if( pSdCtrl->blkSize == 0 )
        {
            dprintf( DEBUG_HDL_SD, "SD: no block size set...\n");
        }
        else
        {
            dprintf( DEBUG_HDL_SD, "SD: current block size: %u\n", pSdCtrl->blkSize );
        }

        // if necessary set the block size to the default
        if( pSdCtrl->blkSize != BLOCK_SIZE8 )
        {
            pSdCtrl->blkSize = BLOCK_SIZE8;
            dprintf( DEBUG_HDL_SD, "  - setting new block size to 512 bytes\n");

            // clear the current block size bits
            regVal32 &= ~BLOCK_MASK;
            regVal32 |= pSdCtrl->blkSize;
            barWrite32( pSdCtrl->barAddress, SDHCI_BLOCK_SIZE, regVal32 );

            // check that the new value was written
            regVal32 = barRead32( pSdCtrl->barAddress, SDHCI_BLOCK_SIZE ) & BLOCK_MASK;
            if( regVal32 != BLOCK_SIZE8 )
            {
                dprintf( DEBUG_HDL_SD, "  - set new block size failed!");
                //@TODO: Probably should fail now?
            }
            else
            {
                dprintf( DEBUG_HDL_SD, "  - new block size set to: %u\n", pSdCtrl->blkSize );
            }
        }

        // test reset after config
        sdhc_reset( pSdCtrl, SDHCI_RESET_CMD | SDHCI_RESET_DATA );

        // setup the interrupts
        barWrite32( pSdCtrl->barAddress, SDHCI_INT_ENABLE, SDHCI_INT_BUS_POWER |
        SDHCI_INT_DATA_END_BIT |
        SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
        SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
        SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
        SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
        SDHCI_INT_DMA_END | SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE |
        SDHCI_INT_ACMD12ERR );

        // and signals
        barWrite32( pSdCtrl->barAddress, SDHCI_SIGNAL_ENABLE, SDHCI_INT_BUS_POWER |
        SDHCI_INT_DATA_END_BIT |
        SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
        SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
        SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
        SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
        SDHCI_INT_DMA_END | SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE |
        SDHCI_INT_ACMD12ERR );

        regVal32 = barRead32( pSdCtrl->barAddress, SDHCI_INT_ENABLE );
        dprintf(6, "SD: interrupts enabled to: 0x%08x\n", regVal32 );

        regVal32 = barRead32( pSdCtrl->barAddress, SDHCI_INT_STATUS );
        dprintf(6, "SD: Current interrupt status: 0x%08x\n", regVal32 );

        regVal32 = barRead32( pSdCtrl->barAddress, SDHCI_PRESENT_STATE );
        dprintf(6, "SD: Present State: 0x%08x\n", regVal32 );

        // record the vendor and sd spec info of the host controller
        sdhc_getVerInfo( pSdCtrl );

        // setup the callback(s) for the underlying card bus
        pSdCtrl->sdhcCmd = &sdhc_Cmd;

        pSdCtrl->isInitialized = true;
    }
    return( pSdCtrl->isInitialized );
}

