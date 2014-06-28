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

#ifndef __SD_H
#define __SD_H

#include <stdint.h>
#include <stdbool.h>
#include "sd_utils.h"

/** @file sd.h */

#include "sdhci.h"

/** @brief sd host capabilities registers */
typedef struct
{
    uint32_t cap1;
    uint32_t cap2;
}capabilities_t;

typedef enum
{
    eCmdNormal = SDHCI_CMD_TYPE_NORMAL,
    eCmdSuspend = SDHCI_CMD_TYPE_SUSPEND,
    eCmdResume = SDHCI_CMD_TYPE_RESUME,
    eCmdAbort = SDHCI_CMD_TYPE_ABORT
}sdCmdType_e;

typedef enum
{
    eRspNone = SDHCI_CMD_RESP_NONE,
    eRsp136 = SDHCI_CMD_RESP_LONG,
    eRsp48 = SDHCI_CMD_RESP_SHORT,
    eRsp48_busy = SDHCI_CMD_RESP_SHORT_BUSY
}sdRspType_e;

typedef enum
{
    eSdSpecV_1_00 = 0,
    eSdSpecV_2_00,
    eSdSpecV_3_00
}sdSpecVer_e;

typedef enum
{
    eRdXfer = 0,
    eWrXfer
}sdXferType_e;

typedef struct
{
    bool idxChkEn;
    bool crcChkEn;
}sdXferFlags_t;

/** @brief struct used for host to card and card to host transfers */
typedef struct
{
    uint8_t                     cmdIdx;
    uint32_t                    arg1;
    sdCmdType_e                 cmdType;
    sdRspType_e                 rspType;
    sdXferType_e                xferType;
    uint32_t                    response[4];
    bool                        rspValid;
    void*                       pData;
}sdXfer_t;

/** @brief struct to hold decoded CID and CSD registers (see SD Specification) */
typedef struct
{
    sd_cid_t                    cidDecode;
    sd_csd_t                    csdDecode;
}sdRegDecode_t;

// resolve forward declarations
struct sdCard_t;
typedef struct sdCard_t sdCard_t;

typedef struct sdHc_t
{
    uint32_t                 barAddress;
    capabilities_t           cardCapabilities;
    uint32_t                 maxClk;
    uint32_t                 tmoClk;
    uint32_t                 curClk;
    uint32_t                 blkSize;
    uint32_t                 pwrMode;
    bool                     crcCheckEnable;
    bool                     indexCheckEnable;
    bool                     xchcSupported;
    bool                     isInitialized;
    uint8_t                  hostVendorId;
    uint8_t                  hostSpecId;
    sdCard_t*                pCard;

    // call back to host for sending commands to the card via the host controller interface
    bool (*sdhcCmd)( struct sdHc_t* pSdCtrl, sdXfer_t* xfer );
}sdHc_t;

/**
 * SD Memory Card Registers
 */
typedef struct sdCard_t
{
    sdHc_t*                 pHost;
    uint32_t                cid[4];
    uint16_t                rca;
    uint16_t                dsr;
    uint32_t                csd[4];
    uint32_t                scr[2];
    uint32_t                ocr;
    uint32_t                ssr[16];
    uint32_t                csr;
    sdRegDecode_t           decode;
    bool                    xchcCard;
    bool                    cardInitialized;
    bool                    isSelected;
}sdCard_t;

#define BLOCK_SIZE8      512
#define MHZ 1000000
#define KHZ 1000
#define BLOCK_MASK 0x00000FFF

// MMC Card Commands that mostly overlap with the SD specification, some have slightly different meaning or results
#define MMC_GO_IDLE_STATE_CMD0            0
#define MMC_SEND_OP_COND_CMD1             1
#define MMC_ALL_SEND_CID_CMD2             2
#define MMC_SET_RELATIVE_ADDR_CMD3        3
#define MMC_SET_DSR_CMD4                  4
#define MMC_SWITCH_CMD6                   6
#define MMC_SELECT_CARD_CMD7              7
#define MMC_SEND_EXT_CSD_CMD8             8
#define MMC_SEND_CSD_CMD9                 9
#define MMC_SEND_CID_CMD10                10
#define MMC_STOP_TRANSMISSION_CMD12       12
#define MMC_SEND_STATUS_CMD13             13
#define MMC_SET_BLOCKLEN_CMD16            16
#define MMC_READ_SINGLE_BLOCK_CMD17       17
#define MMC_READ_MULTIPLE_BLOCK_CMD18     18
#define MMC_WRITE_SINGLE_BLOCK_CMD24      24
#define MMC_WRITE_MULTIPLE_BLOCK_CMD25    25
#define MMC_ERASE_GROUP_START_CMD35       35
#define MMC_ERASE_GROUP_END_CMD36         36
#define MMC_ERASE_CMD38                   38
#define MMC_APP_CMD55                     55
#define MMC_SPI_READ_OCR_CMD58            58
#define MMC_SPI_CRC_ON_OFF_CMD59          59

// SD card specific commands
#define SD_SEND_RELATIVE_ADDR_CMD3        3
#define SD_SWITCH_FUNC_CMD6               6
#define SD_SEND_IF_COND_CMD8              8
#define SD_APP_SET_BUS_WIDTH_CMD6         6
#define SD_ERASE_WR_BLK_START_CMD32       32
#define SD_ERASE_WR_BLK_END_CMD33         33
#define SD_APP_SEND_OP_COND_CMD41         41
#define SD_APP_SEND_SCR_CMD51             51

// other useful defines
#define SD_VOLTAGE_RANGE_270_360          0x00000100
#define SD_IF_COND_ECHO                   0x000000AA
#define SD_STATE_CHANGE_ATTEMPTS          1000
#define SD_TRY_AGAIN                      10


// ACMD41 bit shifts and masks
#define OCR_DONE_BSY_N        (1 << 31)
#define HCS_SHIFT 30
#define HCS  (1 << HCS_SHIFT)
#define XPC_SHIFT 28
#define XPC  (1 << XPC_SHIFT)
#define S18R_SHIFT 24
#define S18R (1 << S18R_SHIFT)
#define OCR_SHIFT 8
#define OCR  (0xFF << OCR_SHIFT)

// OCR register voltage ranges
#define VDD_27_28    (1 << 15)
#define VDD_28_29    (1 << 16)
#define VDD_29_30    (1 << 17)
#define VDD_30_31    (1 << 18)
#define VDD_31_32    (1 << 19)
#define VDD_32_33    (1 << 20)
#define VDD 33_34    (1 << 21)
#define VDD_34_35    (1 << 22)
#define VDD_35_36    (1 << 23)
#define VDD_S18A     (1 << 24)


#define VDD_MASK        ( VDD_27_28 | VDD_28_29 | VDD_29_30 | VDD_30_31 | VDD_31_32 | VDD_32_33 | VDD 33_34 | VDD_34_35 | VDD_35_36 )
#define VDD_RANGE_27_30 ( VDD_27_28 | VDD_28_29 | VDD_29_30 )
#define VDD_RANGE_30_33 ( VDD_30_31 | VDD_31_32 | VDD_32_33 )
#define VDD_RANGE_33_36 ( VDD 33_34 | VDD_34_35 | VDD_35_36 )

#define CARD_UHS_II_STATUS          (1 << 29)
#define CARD_CAPACITY_STATUS        (1 << 30)
#define CARD_POWER_UP_BUSY          (1 << 31)

//! Physical Layer Simplified Specification Version 4.10 "Card Status" bits from Table 4-41
#define SDCARD_OUT_OF_RANGE         (1 << 31)
#define SDCARD_ADDRESS_ERROR        (1 << 30)
#define SDCARD_BLOCK_LEN_ERROR      (1 << 29)
#define SDCARD_ERASE_SEQ_ERROR      (1 << 28)
#define SDCARD_ERASE_PARAM          (1 << 27)
#define SDCARD_WP_VIOLATION         (1 << 26)
#define SDCARD_CARD_IS_LOCKED       (1 << 25)
#define SDCARD_LOCK_UNLOCK_FAILED   (1 << 24)
#define SDCARD_COM_CRC_ERROR        (1 << 23)
#define SDCARD_ILLEGAL_COMMAND      (1 << 22)
#define SDCARD_CARD_ECC_FAILED      (1 << 21)
#define SDCARD_CC_ERROR             (1 << 20)
#define SDCARD_ERROR                (1 << 19)
#define SDCARD_CSD_OVERWRITE        (1 << 16)
#define SDCARD_WP_ERASE_SKIP        (1 << 15)
#define SDCARD_CARD_ECC_DISABLED    (1 << 14)
#define SDCARD_ERASE_RESET          (1 << 13)
#define SDCARD_CURRENT_STATE        (0xF << 9)
#define SDCARD_READY_FOR_DATA       (1 << 8)
#define SDCARD_APP_CMD              (1 << 5)
#define SDCARD_AKE_SEQ_ERROR        (1 << 3)

// RCA RESPONSE STATUS BITS
#define RCASTATUS_COM_CRC_ERROR     (1 << 15)
#define RCASTATUS_ILLEGAL_COMMAND   (1 << 14)
#define RCASTATUS_ERROR             (1 << 13)
#define RCASTATUS_CURRENT_STATE     (SDCARD_CURRENT_STATE)
#define RCASTATUS_READY_FOR_DATA    (SDCARD_READY_FOR_DATA)
#define RCASTATUS_APP_CMD           (SDCARD_APP_CMD)
#define RCASTATUS_AKE_SEQ_ERROR     (SDCARD_AKE_SEQ_ERROR)

#define RCAERROR_MSK    (RCASTATUS_COM_CRC_ERROR | RCASTATUS_ILLEGAL_COMMAND | RCASTATUS_ERROR)

typedef enum
{
    eIdle = 0,
    eReady,
    eIdent,
    eStby,
    eTran,
    eData,
    eRcv,
    ePrg,
    eDis,
    eInvalid
}sdCardState_e;

#define NUM_INIT_ATTEMPTS 2

bool sdCardBusInit( sdHc_t* pHc );
bool sdReadSingleBlock( sdCard_t* pCard, uint8_t* pData, uint32_t addr );
bool sdReadMultipleBlock( sdCard_t* pCard );
bool sdStopTransmission( sdCard_t* pCard );

#endif // __SD_H

