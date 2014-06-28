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
#include "util.h"
#include "malloc.h"
#include "output.h"
#include "config.h"
#include "sd.h"



// Host controller access functions cmd/data and acmd
static bool sdHostXfer( sdHc_t* pHost, sdXfer_t* pXfer );
static bool sdAppSpecificHostXfer( sdHc_t* pHost, sdXfer_t* pXfer );

// Card control functions
static bool sdSelectDeselectCard( sdCard_t* pCard );

// Card intialization functions
static bool sdIdle( sdCard_t* pCard );
static bool sdSendIfCond( sdCard_t* pCard );
static bool sdSendOpCond( sdCard_t* pCard );
static bool sdSendCSD( sdCard_t* pCard );
static bool sdAllSendCID( sdCard_t* pCard );
static bool sdSendRelativeAddr( sdCard_t* pCard );
static bool sdCardIdentificationMode( sdCard_t* pCard );


/**
 * @brief     sdHostXfer - execute the host controller callback to perform
 *                 a transfer from the host to the card and read responses and
 *                 data from the card
 *
 * @param     sdHc_t* pHost - pointer to the host controller abstraction structure
 * @param     sdXfer_t* pXfer - pointer to the host style transfer structure
 *
 * @return    bool - true if successful, false otherwise
 */
static bool sdHostXfer( sdHc_t* pHost, sdXfer_t* pXfer )
{
    bool status = false;

    dprintf( DEBUG_HDL_SD,
            "SD card: Sending CMD%u with arg1 0x%08x\n", pXfer->cmdIdx, pXfer->arg1 );

    status = pHost->sdhcCmd( pHost, pXfer );
    if( status )
    {
        switch( pXfer->rspType )
        {
            case eRsp136:
                dprintf( DEBUG_HDL_SD,
                        "SD card: Response 136: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
                        pXfer->response[0],
                        pXfer->response[1],
                        pXfer->response[2],
                        pXfer->response[3] );
                break;

            case eRsp48:
                dprintf( DEBUG_HDL_SD, "SD card: Response 48: 0x%08x\n",
                        pXfer->response[0] );
                break;

            case eRsp48_busy:
                dprintf( DEBUG_HDL_SD, "SD card: Response 48 with busy signal: 0x%08x\n",
                        pXfer->response[0] );
                break;

            case eRspNone:
            default:
                break;

        }
    }
    else
    {
        dprintf( DEBUG_HDL_SD, "SD card:  Failed to respond to CMD%u\n", pXfer->cmdIdx );
    }

    return status;
}

/**
 * @brief    sdAppSpecificHostXfer - this function performs the necessary steps to
 *                 allow the sd card to accept application specific commands (namely, it
 *                 sends CMD55 prior to sending the ACMD<XX>.  CMD55 puts the card into
 *                 application specific mode.
 *
 * @param     sdHc_t* pHost - pointer to the host controller abstraction structure
 * @param     sdXfer_t* pXfer - pointer to the host style transfer structure
 *
 * @return    bool - true if successful, false otherwise
 */
static bool sdAppSpecificHostXfer( sdHc_t* pHost, sdXfer_t* pXfer )
{
    bool status = false;
    sdXfer_t xfer;

    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_APP_CMD55;
    xfer.arg1 = 0;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp48;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;

    // send CMD55 to place card in APP specific mode
    status = sdHostXfer( pHost, &xfer );
    if( status )
    {
        // send APP specific cmd
        status = sdHostXfer( pHost, pXfer );
    }

    return status;
}

/**
 * @brief    sdSelectDeselectCard - this function impelements CMD7 SELECT/DESELECT_CARD
 *                 to toggle a card between standby and transfer state
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdSelectDeselectCard( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;

    // set up a transaction structure
    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_SELECT_CARD_CMD7;
    xfer.arg1 = (uint32_t)(pCard->rca) << 16;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp48_busy;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;

    // invoke the host controller callback
    status = sdHostXfer( pCard->pHost, &xfer );

    if(status)
    {
        pCard->isSelected = pCard->isSelected == true ? false : true;
    }

    return status;
}

/**
 * @brief    sdReadSingleBlock - read a single block from the card
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 * @param    uint8_t* pData - pointer to the data buffer to read into
 * @param    uint32_t addr - logical block address to read
 *
 * @return   bool - true on success false otherwise
 */
bool sdReadSingleBlock( sdCard_t* pCard, uint8_t* pData, uint32_t addr )
{
    bool status = false;
    sdXfer_t xfer;

    if( !pCard->isSelected )
    {
        // select the card (CMD7)
        sdSelectDeselectCard( pCard );
    }

    // set up a transaction structure
    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_READ_SINGLE_BLOCK_CMD17;
    xfer.arg1 = addr;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp48;
    xfer.xferType = eRdXfer;
    xfer.pData = pData;
    dprintf( DEBUG_HDL_SD, "---- pData address: 0x%08x ----\n", (uint32_t)pData);

    // invoke the host controller callback
    status = sdHostXfer( pCard->pHost, &xfer );

    return status;
}

bool sdReadMultipleBlock( sdCard_t* pCard )
{
    bool status = false;
    // stub
    return status;
}

bool sdStopTransmission( sdCard_t* pCard )
{
    bool status = false;
    // stub
    return status;
}

/**
 * @brief    sdIdle - this function implements the CMD0 GO_IDLE_MODE,
 *                 which resets the sd card and prepares it for intalization
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdIdle( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;

    // set up a transaction structure
    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_GO_IDLE_STATE_CMD0;
    xfer.arg1 = 0;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRspNone;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;

    // invoke the host controller callback
    status = sdHostXfer( pCard->pHost, &xfer );

    return status;
}

/**
 * @brief    sdSendIfCond - this function executes CMD8 SEND_IF_COND, to determine
 *                 whether or not the host controller is compatible with the sd card.
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdSendIfCond( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;
    uint32_t cycleTries = 1;

    /* Send CMD8 to determine whether or not the host controller's voltage setting
     * is correct for the card
     * NOTE: it should always be correct for SD cards, as there is really only one
     * voltage range currently supported as of 2013.  This may change in the future
     * though.  If this fails, the card is not an SD, SDCH or SDXC card and the card
     * is unusable, or the host controller is not setup correctly for the card.
     */
    // set up a transaction structure
    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = SD_SEND_IF_COND_CMD8;

    // voltage range 2.7 to 3.6 v and test pattern = 0xAA
    xfer.arg1 = SD_VOLTAGE_RANGE_270_360 | SD_IF_COND_ECHO;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp48;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;

    // check the echo response
    while( cycleTries-- )
    {
        if( sdHostXfer( pCard->pHost, &xfer ) )
        {
            if( xfer.response[0] == xfer.arg1 )
            {
                status = true;
                break;
            }
            usleep(100);
        }
    }
    if( !status )
    {
        dprintf( 6, "SD: card not present or not supported in the present operating conditions\n" );
    }
    return status;
}

/**
 * @brief    sdSendOpCond - this function executes the ACMD41 SEND_OP_COND, this function
 *                 transitions the card from the idle state to the ready state.  This transition
 *                 requires a minimum of one second to complete, and is required for sdxc and sdhc
 *                 card enumeration.
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdSendOpCond( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;
    uint32_t cycleTries = SD_STATE_CHANGE_ATTEMPTS;

    // Send Application specific command ACMD41 in inquiry mode
    // (no voltage bits set) to read the OCR
    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = SD_APP_SEND_OP_COND_CMD41;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp48;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;

    // setup the voltages based on what is supported by the host
    xfer.arg1 |= pCard->pHost->cardCapabilities.cap1 & SDHCI_CAN_VDD_180 ? VDD_S18A : 0;
    xfer.arg1 |= pCard->pHost->cardCapabilities.cap1 & SDHCI_CAN_VDD_300 ? VDD_RANGE_27_30 : 0;
    xfer.arg1 |= pCard->pHost->cardCapabilities.cap1 & SDHCI_CAN_VDD_330 ? VDD_RANGE_30_33 : 0;

    // normal sd cards ignore the HCS bit, so set it for hcxc types
    xfer.arg1 |= HCS;

    while( cycleTries-- )
    {
        status = sdAppSpecificHostXfer( pCard->pHost, &xfer );
        if( status )
        {
            // initialization takes 1 second to complete, and we query the busy bit
            // in the response to know when to stop
            if( xfer.response[0] & OCR_DONE_BSY_N )
            {
                pCard->ocr = xfer.response[0];
                pCard->xchcCard = (HCS == (pCard->ocr & HCS));
                break;
            }
            else
            {
                udelay(100);
            }
        }
    }
    return status;
}

/**
 * @brief    sdSendCSD - request the values from CSD register
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdSendCSD( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;
    uint32_t cycleTries = SD_TRY_AGAIN;

    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_SEND_CSD_CMD9;
    xfer.arg1 =  (uint32_t)(pCard->rca) << 16;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp136;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;
    while( cycleTries-- )
    {
        status = sdHostXfer( pCard->pHost, &xfer );
        if( status )
        {
            // The Host Response places bits [127:8] in bits [119:0] of the response (hence the shift)
            memmove( xfer.response, (void*)(((uint8_t*)(xfer.response)) - 1), sizeof(xfer.response ) );
            pCard->csd[0] = xfer.response[3];
            pCard->csd[1] = xfer.response[2];
            pCard->csd[2] = xfer.response[1];
            pCard->csd[3] = xfer.response[0];
            decode_csd_sd( pCard->csd, &pCard->decode.csdDecode );
            break;
        }

    }
    return status;
}

/**
 * @brief    sdAllSendCID - this function executes CMD2 ALL_SEND_CID, this transitions
 *                 the card from the ready state to the identification state, and allows the
 *                 card to transmit the contents of the CID register
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdAllSendCID( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;
    uint32_t cycleTries = SD_TRY_AGAIN;

    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_ALL_SEND_CID_CMD2;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp136;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;
    while( cycleTries-- )
    {
        status = sdHostXfer( pCard->pHost, &xfer );
        if( status )
        {
            // The Host Response places bits [127:8] in bits [119:0] of the response (hence the shift)
            memmove( xfer.response, (void*)(((uint8_t*)(xfer.response)) - 1), sizeof(xfer.response ) );

            // the utility functions swap the result prior to use
            pCard->cid[0] = xfer.response[3];
            pCard->cid[1] = xfer.response[2];
            pCard->cid[2] = xfer.response[1];
            pCard->cid[3] = xfer.response[0];
            decode_cid_sd( pCard->cid, &pCard->decode.cidDecode );
            break;
        }

    }
    return status;

}

/**
 * @brief    sdSendRelativeAddr - this function executes CMD3 - SEND_RELATIVE_ADDRESS, of the
 *                 card initialization sequence.  After successful completion, the relative card
 *                 address (RCA) of the card is known, and the SD card enters standby mode from
 *                 card identification mode
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdSendRelativeAddr( sdCard_t* pCard )
{
    bool status = false;
    sdXfer_t xfer;
    uint32_t cycleTries = SD_TRY_AGAIN;
    sdCardState_e currentState = eInvalid;

    memset( &xfer, 0, sizeof( sdXfer_t ) );
    xfer.cmdIdx = MMC_SET_RELATIVE_ADDR_CMD3;
    xfer.cmdType = eCmdNormal;
    xfer.rspType = eRsp48;
    xfer.xferType = eWrXfer;
    xfer.pData = NULL;
    while( cycleTries-- )
    {
        status = sdHostXfer( pCard->pHost, &xfer );
        if( status )
        {
            // the response is the R6 response containing the relative card
            // address, and current status info
            pCard->rca = (uint16_t)(xfer.response[0] >> 16);
            if( xfer.response[0] & RCAERROR_MSK )
            {
                dprintf( DEBUG_HDL_SD,
                        "SD card: RCA request responded with error status: 0x%08x\n",
                        xfer.response[0] & RCAERROR_MSK );
                status = false;
                // here there should probably be a full status check
            }

            // now the card should be in stby mode (this might not be reflected until
            // the next command (the offset is bit 9 : 12 for state info)
            currentState = (sdCardState_e)((xfer.response[0] & RCASTATUS_CURRENT_STATE) >> 9);
            if( currentState == eStby )
            {
                dprintf( DEBUG_HDL_SD, "SD card: Current state is stby: 0x%02x - \n", currentState );
                break;
            }
        }
    }
    return status;

}

/**
 * @brief    sdCardIdentificationMode - this function executes the call sequence/state
 *                 machine outlined in section 4.2 of "Physical Layer Simplified Specification
 *                 Version 4.10"
 *
 * @param    sdCard_t* pCard - pointer to the sd card abstraction structure
 *
 * @return   bool - true if successful, false otherwise
 */
static bool sdCardIdentificationMode( sdCard_t* pCard )
{
    bool status = false;
    uint32_t cycleTries = NUM_INIT_ATTEMPTS;

    ASSERT32FLAT();
     while( cycleTries-- )
     {
        // CMD8 - SEND_IF_COND
        if( !(status = sdSendIfCond( pCard )) )
            continue;

        // ACMD41 - SEND_OP_COND to properly initialize SDHC and SDXC cards...
        if( !(status = sdSendOpCond( pCard )) )
            continue;

        // If the voltage needs to change based on the response for ACMD41, change it using CMD11
        // @TODO: implement CMD11 voltage switch if necessary... probably don't need to support this
        // for a while

        // Else Send CMD2 to get the CID, the card should be in identification state
        if( !(status = sdAllSendCID( pCard )) )
            continue;

        // Send CMD3 to get the relative card address (RCA) used for broad cast commands
        if( !(status = sdSendRelativeAddr( pCard )) )
            continue;

        // Send CMD9 to get the CSD register info
        if( !(status = sdSendCSD( pCard )) )
            continue;

        // the card will now be in standby state, and is ready for data transfers
        if( status == true )
        {
            dprintf( DEBUG_HDL_SD, "SD card: Initialization of sd card is complete:\n" );
            dprintf( DEBUG_HDL_SD, "  - currrent card state:  standby\n");
            dprintf( DEBUG_HDL_SD, "  - RCA: 0x%04x\n", pCard->rca );
            dprintf( DEBUG_HDL_SD, "  - OCR: 0x%08x\n", pCard->ocr );
            dprintf( DEBUG_HDL_SD,
                    "  - CID: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
                    pCard->cid[0], pCard->cid[1], pCard->cid[2], pCard->cid[3] );
            dprintf( DEBUG_HDL_SD,
                    "  - CSD: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
                    pCard->csd[0], pCard->csd[1], pCard->csd[2], pCard->csd[3] );
            dprintf( DEBUG_HDL_SD, "  - card type is ");
            if( pCard->xchcCard )
            {
                dprintf( DEBUG_HDL_SD, "SDXC/SDHC\n" );
            }
            else
            {
                dprintf( DEBUG_HDL_SD, "Normal SD\n" );
            }


            break;
        }
     }
    return status;
}

/**
 * @brief    sdCardBusInit - public function to initialize the SD card-bus.
 *                 This function is called by the host controller after the host
 *                 controller has completed its initial "pessimistic" configuration.
 *                 This function implements the steps in the sequence diagram of figure
 *                 4-1 of the "Physical Layer Simplified Specification Version 4.10" of the
 *                 SD card specification documents.
 *
 * @param    sdHc_t* hc - pointer to the host controller structure
 *
 * @return   bool - status, true = success, false = failure of the card bus initialization
 */
bool sdCardBusInit( sdHc_t* pHc )
{
    bool status = false;

    // allocate the card
    sdCard_t* pCard = malloc_fseg( sizeof( sdCard_t ) );
    if( !pCard )
    {
        dprintf( DEBUG_HDL_SD, "SD: card failed to allocate\n" );
        return status;
    }
    dprintf( DEBUG_HDL_SD, "SD:  pCard address: 0x%08x\n", (uint32_t)pCard);

    // set the host pointer
    pCard->pHost = pHc;

    // Set Idle Mode with CMD0
    status = sdIdle( pCard );

    if( status )
    {
        // execute the initialization/identification procedure
        pCard->cardInitialized = sdCardIdentificationMode( pCard );
    }

    if( !pCard->cardInitialized )
    {
        dprintf( DEBUG_HDL_SD, "SD: Card init failed, check card...\n");
        status = pCard->cardInitialized;
        free( pCard );
    }
    else
    {
        // If we get here, the card is in standby mode, so give a
        // reference to the host controller
        pHc->pCard = pCard;
    }
    return status;
}
