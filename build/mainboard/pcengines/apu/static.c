#include <device/device.h>
#include <device/pci.h>
#include "northbridge/amd/agesa/family14/chip.h"
#include "southbridge/amd/cimx/sb800/chip.h"
#include "superio/nuvoton/nct5104d/chip.h"

#ifndef __PRE_RAM__
__attribute__((weak)) struct chip_operations mainboard_ops = {};
__attribute__((weak)) struct chip_operations cpu_amd_agesa_family14_ops = {};
__attribute__((weak)) struct chip_operations northbridge_amd_agesa_family14_ops = {};
__attribute__((weak)) struct chip_operations northbridge_amd_agesa_family14_root_complex_ops = {};
__attribute__((weak)) struct chip_operations southbridge_amd_cimx_sb800_ops = {};
__attribute__((weak)) struct chip_operations superio_nuvoton_nct5104d_ops = {};
#endif

/* pass 0 */
ROMSTAGE_CONST struct bus dev_root_links[];
ROMSTAGE_CONST static struct device _dev2;
ROMSTAGE_CONST struct bus _dev2_links[];
ROMSTAGE_CONST static struct device _dev5;
ROMSTAGE_CONST struct bus _dev5_links[];
ROMSTAGE_CONST static struct device _dev4;
ROMSTAGE_CONST static struct device _dev47;
ROMSTAGE_CONST static struct device _dev48;
ROMSTAGE_CONST static struct device _dev49;
ROMSTAGE_CONST static struct device _dev50;
ROMSTAGE_CONST static struct device _dev51;
ROMSTAGE_CONST static struct device _dev52;
ROMSTAGE_CONST static struct device _dev53;
ROMSTAGE_CONST static struct device _dev54;
ROMSTAGE_CONST static struct device _dev8;
ROMSTAGE_CONST static struct device _dev9;
ROMSTAGE_CONST static struct device _dev10;
ROMSTAGE_CONST static struct device _dev11;
ROMSTAGE_CONST static struct device _dev12;
ROMSTAGE_CONST static struct device _dev13;
ROMSTAGE_CONST static struct device _dev14;
ROMSTAGE_CONST static struct device _dev16;
ROMSTAGE_CONST static struct device _dev17;
ROMSTAGE_CONST static struct device _dev18;
ROMSTAGE_CONST static struct device _dev19;
ROMSTAGE_CONST static struct device _dev20;
ROMSTAGE_CONST static struct device _dev21;
ROMSTAGE_CONST static struct device _dev22;
ROMSTAGE_CONST static struct device _dev23;
ROMSTAGE_CONST static struct device _dev24;
ROMSTAGE_CONST static struct device _dev25;
ROMSTAGE_CONST static struct device _dev26;
ROMSTAGE_CONST struct bus _dev26_links[];
ROMSTAGE_CONST static struct device _dev39;
ROMSTAGE_CONST static struct device _dev40;
ROMSTAGE_CONST static struct device _dev41;
ROMSTAGE_CONST static struct device _dev42;
ROMSTAGE_CONST static struct device _dev43;
ROMSTAGE_CONST static struct device _dev44;
ROMSTAGE_CONST static struct device _dev45;
ROMSTAGE_CONST static struct device _dev46;
ROMSTAGE_CONST static struct device _dev28;
ROMSTAGE_CONST static struct device _dev29;
ROMSTAGE_CONST struct resource _dev29_res[];
ROMSTAGE_CONST static struct device _dev30;
ROMSTAGE_CONST struct resource _dev30_res[];
ROMSTAGE_CONST static struct device _dev31;
ROMSTAGE_CONST struct resource _dev31_res[];
ROMSTAGE_CONST static struct device _dev32;
ROMSTAGE_CONST struct resource _dev32_res[];
ROMSTAGE_CONST static struct device _dev33;
ROMSTAGE_CONST static struct device _dev34;
ROMSTAGE_CONST static struct device _dev35;
ROMSTAGE_CONST static struct device _dev36;
ROMSTAGE_CONST static struct device _dev37;
ROMSTAGE_CONST static struct device _dev38;

/* pass 1 */
ROMSTAGE_CONST struct device * ROMSTAGE_CONST last_dev = &_dev54;
ROMSTAGE_CONST struct device dev_root = {
#ifndef __PRE_RAM__
	.ops = &default_dev_ops_root,
#endif
	.bus = &dev_root_links[0],
	.path = { .type = DEVICE_PATH_ROOT },
	.enabled = 1,
	.on_mainboard = 1,
	.link_list = &dev_root_links[0],
#ifndef __PRE_RAM__
	.chip_ops = &mainboard_ops,
	.name = mainboard_name,
#endif
	.next=&_dev2
};
ROMSTAGE_CONST struct bus dev_root_links[] = {
		[0] = {
			.link_num = 0,
			.dev = &dev_root,
			.children = &_dev2,
			.next = NULL,
		},
	};
static ROMSTAGE_CONST struct device _dev2 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &dev_root_links[0],
	.path = {.type=DEVICE_PATH_CPU_CLUSTER,{.cpu_cluster={ .cluster = 0x0 }}},
	.enabled = 1,
	.on_mainboard = 1,
	.link_list = &_dev2_links[0],
	.sibling = &_dev5,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_root_complex_ops,
#endif
	.next=&_dev4
};
ROMSTAGE_CONST struct bus _dev2_links[] = {
		[0] = {
			.link_num = 0,
			.dev = &_dev2,
			.children = &_dev4,
			.next = NULL,
		},
	};
static ROMSTAGE_CONST struct device _dev5 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &dev_root_links[0],
	.path = {.type=DEVICE_PATH_DOMAIN,{.domain={ .domain = 0x0 }}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = &_dev5_links[0],
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_root_complex_ops,
#endif
	.next=&_dev8
};
ROMSTAGE_CONST struct bus _dev5_links[] = {
		[0] = {
			.link_num = 0,
			.dev = &_dev5,
			.children = &_dev8,
			.next = NULL,
		},
	};
static ROMSTAGE_CONST struct device _dev4 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev2_links[0],
	.path = {.type=DEVICE_PATH_APIC,{.apic={ .apic_id = 0x0 }}},
	.enabled = 1,
	.on_mainboard = 1,
	.link_list = NULL,
#ifndef __PRE_RAM__
	.chip_ops = &cpu_amd_agesa_family14_ops,
#endif
	.next=&_dev5
};
ROMSTAGE_CONST struct northbridge_amd_agesa_family14_config northbridge_amd_agesa_family14_info_6 = { };
ROMSTAGE_CONST struct northbridge_amd_agesa_family14_config northbridge_amd_agesa_family14_info_7 = { };
ROMSTAGE_CONST struct southbridge_amd_cimx_sb800_config southbridge_amd_cimx_sb800_info_15 = {
	.boot_switch_sata_ide = 0,
	.gpp_configuration = GPP_CFGMODE_X4000,
};

static ROMSTAGE_CONST struct device _dev47 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev48,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev48
};
static ROMSTAGE_CONST struct device _dev48 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,1)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev49,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev49
};
static ROMSTAGE_CONST struct device _dev49 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,2)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev50,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev50
};
static ROMSTAGE_CONST struct device _dev50 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,3)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev51,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev51
};
static ROMSTAGE_CONST struct device _dev51 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,4)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev52,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev52
};
static ROMSTAGE_CONST struct device _dev52 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,5)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev53,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev53
};
static ROMSTAGE_CONST struct device _dev53 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,6)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev54,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
	.next=&_dev54
};
static ROMSTAGE_CONST struct device _dev54 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x18,7)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_6,
};
static ROMSTAGE_CONST struct device _dev8 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x0,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev9,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev9
};
static ROMSTAGE_CONST struct device _dev9 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x1,0)}}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev10,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev10
};
static ROMSTAGE_CONST struct device _dev10 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x4,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev11,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev11
};
static ROMSTAGE_CONST struct device _dev11 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x5,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev12,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev12
};
static ROMSTAGE_CONST struct device _dev12 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x6,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev13,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev13
};
static ROMSTAGE_CONST struct device _dev13 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x7,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev14,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev14
};
static ROMSTAGE_CONST struct device _dev14 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x8,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev16,
#ifndef __PRE_RAM__
	.chip_ops = &northbridge_amd_agesa_family14_ops,
#endif
	.chip_info = &northbridge_amd_agesa_family14_info_7,
	.next=&_dev16
};
static ROMSTAGE_CONST struct device _dev16 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x11,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev17,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev17
};
static ROMSTAGE_CONST struct device _dev17 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x12,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev18,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev18
};
static ROMSTAGE_CONST struct device _dev18 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x12,1)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev19,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev19
};
static ROMSTAGE_CONST struct device _dev19 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x12,2)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev20,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev20
};
static ROMSTAGE_CONST struct device _dev20 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x13,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev21,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev21
};
static ROMSTAGE_CONST struct device _dev21 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x13,1)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev22,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev22
};
static ROMSTAGE_CONST struct device _dev22 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x13,2)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev23,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev23
};
static ROMSTAGE_CONST struct device _dev23 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x14,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev24,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev24
};
static ROMSTAGE_CONST struct device _dev24 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x14,1)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev25,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev25
};
static ROMSTAGE_CONST struct device _dev25 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x14,2)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev26,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev26
};
static ROMSTAGE_CONST struct device _dev26 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x14,3)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = &_dev26_links[0],
	.sibling = &_dev39,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev28
};
ROMSTAGE_CONST struct bus _dev26_links[] = {
		[0] = {
			.link_num = 0,
			.dev = &_dev26,
			.children = &_dev28,
			.next = NULL,
		},
	};
static ROMSTAGE_CONST struct device _dev39 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x14,4)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev40,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev40
};
static ROMSTAGE_CONST struct device _dev40 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x14,5)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev41,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev41
};
static ROMSTAGE_CONST struct device _dev41 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x15,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev42,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev42
};
static ROMSTAGE_CONST struct device _dev42 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x15,1)}}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev43,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev43
};
static ROMSTAGE_CONST struct device _dev43 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x15,2)}}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev44,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev44
};
static ROMSTAGE_CONST struct device _dev44 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x15,3)}}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev45,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev45
};
static ROMSTAGE_CONST struct device _dev45 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x16,0)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev46,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev46
};
static ROMSTAGE_CONST struct device _dev46 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev5_links[0],
	.path = {.type=DEVICE_PATH_PCI,{.pci={ .devfn = PCI_DEVFN(0x16,2)}}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev47,
#ifndef __PRE_RAM__
	.chip_ops = &southbridge_amd_cimx_sb800_ops,
#endif
	.chip_info = &southbridge_amd_cimx_sb800_info_15,
	.next=&_dev47
};
ROMSTAGE_CONST struct superio_nuvoton_nct5104d_config superio_nuvoton_nct5104d_info_27 = {
	.irq_trigger_type = 0,
};

static ROMSTAGE_CONST struct device _dev28 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x0 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev29,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev29
};
static ROMSTAGE_CONST struct device _dev29 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x2 }}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.resource_list = &_dev29_res[0],
	.link_list = NULL,
	.sibling = &_dev30,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev30
};
ROMSTAGE_CONST struct resource _dev29_res[] = {
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IO, .index=0x60, .base=0x3f8,.next=&_dev29_res[1]},
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IRQ, .index=0x70, .base=0x4,.next=NULL },
	 };
static ROMSTAGE_CONST struct device _dev30 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x3 }}},
	.enabled = 1,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.resource_list = &_dev30_res[0],
	.link_list = NULL,
	.sibling = &_dev31,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev31
};
ROMSTAGE_CONST struct resource _dev30_res[] = {
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IO, .index=0x60, .base=0x2f8,.next=&_dev30_res[1]},
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IRQ, .index=0x70, .base=0x3,.next=NULL },
	 };
static ROMSTAGE_CONST struct device _dev31 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x10 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.resource_list = &_dev31_res[0],
	.link_list = NULL,
	.sibling = &_dev32,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev32
};
ROMSTAGE_CONST struct resource _dev31_res[] = {
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IO, .index=0x60, .base=0x3e8,.next=&_dev31_res[1]},
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IRQ, .index=0x70, .base=0x4,.next=NULL },
	 };
static ROMSTAGE_CONST struct device _dev32 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x11 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.resource_list = &_dev32_res[0],
	.link_list = NULL,
	.sibling = &_dev33,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev33
};
ROMSTAGE_CONST struct resource _dev32_res[] = {
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IO, .index=0x60, .base=0x2e8,.next=&_dev32_res[1]},
		{ .flags=IORESOURCE_FIXED | IORESOURCE_ASSIGNED | IORESOURCE_IRQ, .index=0x70, .base=0x3,.next=NULL },
	 };
static ROMSTAGE_CONST struct device _dev33 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x8 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev34,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev34
};
static ROMSTAGE_CONST struct device _dev34 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0xf }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev35,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev35
};
static ROMSTAGE_CONST struct device _dev35 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x7 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev36,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev36
};
static ROMSTAGE_CONST struct device _dev36 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x107 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev37,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev37
};
static ROMSTAGE_CONST struct device _dev37 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0x607 }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
	.sibling = &_dev38,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev38
};
static ROMSTAGE_CONST struct device _dev38 = {
#ifndef __PRE_RAM__
	.ops = 0,
#endif
	.bus = &_dev26_links[0],
	.path = {.type=DEVICE_PATH_PNP,{.pnp={ .port = 0x2e, .device = 0xe }}},
	.enabled = 0,
	.on_mainboard = 1,
	.subsystem_vendor = 0x1022,
	.subsystem_device = 0x1510,
	.link_list = NULL,
#ifndef __PRE_RAM__
	.chip_ops = &superio_nuvoton_nct5104d_ops,
#endif
	.chip_info = &superio_nuvoton_nct5104d_info_27,
	.next=&_dev39
};
