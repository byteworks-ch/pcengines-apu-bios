/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "agesawrapper.h"
#include "amdlib.h"
#include "BiosCallOuts.h"
#include "heapManager.h"
#include "SB800.h"
#include <northbridge/amd/agesa/family14/dimmSpd.h>
#include "../../../northbridge/amd/agesa/common/common.h"

int get_spd_offset(void);
AGESA_STATUS check_for_pcengines_spd_offset(UINT32 Func, UINT32 Data, VOID *ConfigPtr);

STATIC BIOS_CALLOUT_STRUCT BiosCallouts[] =
{
	{AGESA_ALLOCATE_BUFFER,			BiosAllocateBuffer },
	{AGESA_DEALLOCATE_BUFFER,		BiosDeallocateBuffer },
	{AGESA_DO_RESET,			BiosReset },
	{AGESA_LOCATE_BUFFER,			BiosLocateBuffer },
	{AGESA_READ_SPD,			check_for_pcengines_spd_offset },
	{AGESA_READ_SPD_RECOVERY,		BiosDefaultRet },
	{AGESA_RUNFUNC_ONAP,			BiosRunFuncOnAp },
	{AGESA_GNB_PCIE_SLOT_RESET,		BiosGnbPcieSlotReset },
	{AGESA_HOOKBEFORE_DRAM_INIT,		BiosHookBeforeDramInit },
	{AGESA_HOOKBEFORE_DRAM_INIT_RECOVERY,	BiosHookBeforeDramInitRecovery },
	{AGESA_HOOKBEFORE_DQS_TRAINING,		BiosHookBeforeDQSTraining },
	{AGESA_HOOKBEFORE_EXIT_SELF_REF,	BiosHookBeforeExitSelfRefresh },
};

AGESA_STATUS GetBiosCallout (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	UINTN i;
	AGESA_STATUS CalloutStatus;
	UINTN CallOutCount = sizeof (BiosCallouts) / sizeof (BiosCallouts [0]);

	/*
	 * printk(BIOS_SPEW,"%s function: %x\n", __func__, (u32) Func);
	 */

	CalloutStatus = AGESA_UNSUPPORTED;

	for (i = 0; i < CallOutCount; i++) {
		if (BiosCallouts[i].CalloutName == Func) {
			CalloutStatus = BiosCallouts[i].CalloutPtr (Func, Data, ConfigPtr);
			return CalloutStatus;
		}
	}

	return CalloutStatus;
}

/*	Call the host environment interface to provide a user hook opportunity. */
AGESA_STATUS BiosHookBeforeDQSTraining (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	return AGESA_SUCCESS;
}
/*	Call the host environment interface to provide a user hook opportunity. */
AGESA_STATUS BiosHookBeforeDramInit (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	// Unlike e.g. AMD Inagua, this board is unable to vary the RAM voltage.
	// Make sure the right speed settings are selected.
	((MEM_DATA_STRUCT*)ConfigPtr)->ParameterListPtr->DDR3Voltage = VOLT1_5;
	return AGESA_SUCCESS;
}

/*	Call the host environment interface to provide a user hook opportunity. */
AGESA_STATUS BiosHookBeforeDramInitRecovery (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	return AGESA_SUCCESS;
}

/*	Call the host environment interface to provide a user hook opportunity. */
AGESA_STATUS BiosHookBeforeExitSelfRefresh (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	return AGESA_SUCCESS;
}
/* PCIE slot reset control */
AGESA_STATUS BiosGnbPcieSlotReset (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	return AGESA_SUCCESS;
}

AGESA_STATUS check_for_pcengines_spd_offset (UINT32 Func, UINT32 Data, VOID *ConfigPtr)
{
	AGESA_STATUS Status;
#ifdef __PRE_RAM__
	AGESA_READ_SPD_PARAMS *info = ConfigPtr;
	struct multi_spd_info *spd_info = (struct multi_spd_info *)info->Buffer;
	spd_info->offset = get_spd_offset();
	spd_info->size = 128; // SPDs for PCEngines/APU are all 128 bytes
	Status = common_ReadCbfsSpd(Func, Data, ConfigPtr);
#else
	Status = AGESA_UNSUPPORTED;
#endif

	return Status;
}
