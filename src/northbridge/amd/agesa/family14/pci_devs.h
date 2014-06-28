/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2014 Sage Electronic Engineering, LLC.
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

#ifndef _AMD_FAM14_PCI_DEVS_H_
#define _AMD_FAM14_PCI_DEVS_H_

#define BUS0 0

#include <device/pci_def.h>

/* Graphics and Display */
#define GFX_DEV 0x1
#define GFX_FUNC 0
# define GFX_DEVFN PCI_DEVFN(GFX_DEV,GFX_FUNC)

/* PCIe Ports */
#define NB_PCIE_PORT1_DEV 0x4
#define NB_PCIE_PORT2_DEV 0x5
#define NB_PCIE_PORT3_DEV 0x6
#define NB_PCIE_PORT4_DEV 0x7
#define NB_PCIE_PORT5_DEV 0x8
#define NB_PCIE_FUNC 0
# define NB_PCIE_PORT1_DEVID 0x1512
# define NB_PCIE_PORT2_DEVID 0x1513
# define NB_PCIE_PORT3_DEVID 0x1514
# define NB_PCIE_PORT4_DEVID 0x1515
# define NB_PCIE_PORT5_DEVID 0x1516
# define NB_PCIE_PORT1_DEVFN PCI_DEVFN(NB_PCIE_PORT1_DEV,NB_PCIE_FUNC)
# define NB_PCIE_PORT2_DEVFN PCI_DEVFN(NB_PCIE_PORT2_DEV,NB_PCIE_FUNC)
# define NB_PCIE_PORT3_DEVFN PCI_DEVFN(NB_PCIE_PORT3_DEV,NB_PCIE_FUNC)
# define NB_PCIE_PORT4_DEVFN PCI_DEVFN(NB_PCIE_PORT4_DEV,NB_PCIE_FUNC)
# define NB_PCIE_PORT5_DEVFN PCI_DEVFN(NB_PCIE_PORT5_DEV,NB_PCIE_FUNC)

#endif /* _AMD_FAM14_PCI_DEVS_H_ */
