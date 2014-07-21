/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Sage Electronic Engineering, LLC.
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

#include <console/console.h>
#include <device/path.h>
#include <device/pci_ids.h>
#include <cpu/cpu.h>
#include <arch/smp/mpspec.h>
#include <string.h>
#include <arch/cpu.h>
#include <cpu/x86/lapic.h>
#include <drivers/generic/ioapic/chip.h>

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
#include <lib.h> // hexdump
#endif

/* Initialize the specified "mc" struct with initial values. */
void mptable_init(struct mp_config_table *mc, u32 lapic_addr)
{
	int i;

	memset(mc, 0, sizeof(*mc));

	memcpy(mc->mpc_signature, MPC_SIGNATURE, 4);

	mc->mpc_length = sizeof(*mc);	/* Initially just the header size. */
	mc->mpc_spec = 0x04;		/* MultiProcessor specification 1.4 */
	mc->mpc_checksum = 0;		/* Not yet computed. */
	mc->mpc_oemptr = 0;
	mc->mpc_oemsize = 0;
	mc->mpc_entry_count = 0;	/* No entries yet... */
	mc->mpc_lapic = lapic_addr;
	mc->mpe_length = 0;
	mc->mpe_checksum = 0;
	mc->reserved = 0;

	strncpy(mc->mpc_oem, CONFIG_MAINBOARD_VENDOR, 8);
	strncpy(mc->mpc_productid, CONFIG_MAINBOARD_PART_NUMBER, 12);

	/*
	 * The oem/productid fields are exactly 8/12 bytes long. If the resp.
	 * entry is shorter, the remaining bytes are filled with spaces.
	 */
	for (i = MIN(strlen(CONFIG_MAINBOARD_VENDOR), 8); i < 8; i++)
		mc->mpc_oem[i] = ' ';
	for (i = MIN(strlen(CONFIG_MAINBOARD_PART_NUMBER), 12); i < 12; i++)
		mc->mpc_productid[i] = ' ';

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "MP_Tables: Init MP Configuration Table Header:\n"
			"\tSignature\t\t: %4.4s\n"
			"\tLength\t\t\t: 0x%x (Only Header, no other entries)\n"
			"\tMP Specification\t: 0x%x\n"
			"\tChecksum\t\t: 0x%x (Not calculated yet)\n"
			"\tOEM ID\t\t\t: %8.8s\n"
			"\tProduct ID\t\t: %12.12s\n"
			"\tOEM Table PTR\t\t: 0x%x\n"
			"\tOEM Table Size\t\t: 0x%x (Not filled in yet)\n"
			"\tEntry Count\t\t: 0x%x (No entries created yet)\n"
			"\tMMIO LAPIC Address\t: 0x%x\n"
			"\tExtended Table Length\t: 0x%x (Not filled in yet)\n"
			"\tExtended Table Checksum\t: 0x%x (Not calculated yet)\n",
			mc->mpc_signature, mc->mpc_length, mc->mpc_spec, mc->mpc_checksum,
			mc->mpc_oem, mc->mpc_productid, mc->mpc_oemptr, mc->mpc_oemsize,
			mc->mpc_entry_count, mc->mpc_lapic, mc->mpe_length, mc->mpe_checksum);
#endif
}

unsigned char smp_compute_checksum(void *v, int len)
{
	unsigned char *bytes;
	unsigned char checksum;
	int i;
	bytes = v;
	checksum = 0;
	for(i = 0; i < len; i++) {
		checksum -= bytes[i];
	}
	return checksum;
}

static void *smp_write_floating_table_physaddr(u32 addr, u32 mpf_physptr, unsigned int virtualwire)
{
	struct intel_mp_floating *mf;
	void *v;

	v = (void *)addr;
	mf = v;
	mf->mpf_signature[0] = '_';
	mf->mpf_signature[1] = 'M';
	mf->mpf_signature[2] = 'P';
	mf->mpf_signature[3] = '_';
	mf->mpf_physptr = mpf_physptr;
	mf->mpf_length = 1;
	mf->mpf_specification = 4;
	mf->mpf_checksum = 0;
	mf->mpf_feature1 = 0;
	mf->mpf_feature2 = virtualwire?MP_FEATURE_PIC:MP_FEATURE_VIRTUALWIRE;
	mf->mpf_feature3 = 0;
	mf->mpf_feature4 = 0;
	mf->mpf_feature5 = 0;
	mf->mpf_checksum = smp_compute_checksum(mf, mf->mpf_length*16);

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "MP_Tables: Add MP Floating Pointer Structure:\n"
			"\tSignature\t\t: %4.4s\n"
			"\tPhysical Pointer\t: 0x%x\n"
			"\tLength\t\t\t: %d Paragraph(s) (16-bytes each)\n"
			"\tMP Specification\t: 0x%x\n"
			"\tChecksum\t\t: 0x%x\n"
			"\tFeature1\t\t: 0x%x (Should be 0)\n"
			"\tFeature2\t\t: '%s'\n"
			"\tFeature3\t\t: 0x%x\n"
			"\tFeature4\t\t: 0x%x\n"
			"\tFeature5\t\t: 0x%x\n",
			mf->mpf_signature, mpf_physptr, mf->mpf_length, mf->mpf_specification,
			mf->mpf_checksum, mf->mpf_feature1, virtualwire?"PIC Mode":"Virtual Wire Mode",
			mf->mpf_feature3, mf->mpf_feature4,	mf->mpf_feature5);
#endif

        return v;
}

void *smp_write_floating_table(unsigned long addr, unsigned int virtualwire)
{
	/* 16 byte align the table address */
	addr = (addr + 0xf) & (~0xf);

	printk(BIOS_INFO, "MP_Tables: Start writing the MP_Tables at 0x%lx\n", addr);
	return smp_write_floating_table_physaddr(addr, addr + SMP_FLOATING_TABLE_LEN, virtualwire);
}

void *smp_next_mpc_entry(struct mp_config_table *mc)
{
	void *v;
	v = (void *)(((char *)mc) + mc->mpc_length);

	return v;
}
static void smp_add_mpc_entry(struct mp_config_table *mc, u16 length)
{
	mc->mpc_length += length;
	mc->mpc_entry_count++;
}

void *smp_next_mpe_entry(struct mp_config_table *mc)
{
	void *v;
	v = (void *)(((char *)mc) + mc->mpc_length + mc->mpe_length);

	return v;
}
static void smp_add_mpe_entry(struct mp_config_table *mc, mpe_t mpe)
{
	mc->mpe_length += mpe->mpe_length;
}

/*
 * Type 0: Processor Entries:
 * Entry Type, LAPIC ID, LAPIC Version, CPU Flags EN/BP,
 * CPU Signature (Stepping, Model, Family), Feature Flags
 */
void smp_write_processor(struct mp_config_table *mc,
	u8 apicid, u8 apicver, u8 cpuflag,
	u32 cpufeature, u32 featureflag)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd CPU/LAPIC Entry (Type 0)\n");
#endif

	struct mpc_config_processor *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_PROCESSOR;
	mpc->mpc_apicid = apicid;
	mpc->mpc_apicver = apicver;
	mpc->mpc_cpuflag = cpuflag;
	mpc->mpc_cpufeature = cpufeature;
	mpc->mpc_featureflag = featureflag;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tID\t\t: 0x%x\n"
			"\tVersion\t\t: 0x%x\n"
			"\tFlags\t\t: 0x%x (%s%s)\n"
			"\tCPU Signature 0x%x:\n"
			"\t  (Family %d, Model %d, Stepping %d)\n"
			"\tFeature Flag\t: 0x%x (See CPUID)\n",
			apicid, apicver, cpuflag,
			(cpuflag & 0x1) ? "Enabled" : "Disabled",
			(cpuflag & 0x2) ? ", Bootstrap" : "",
			cpufeature, (cpufeature >> 8) & 0x0F, (cpufeature  >> 4) & 0x0F,
			cpufeature & 0x0F, featureflag);
#endif

	smp_add_mpc_entry(mc, sizeof(*mpc));
}

/*
 * If we assume a symmetric processor configuration we can
 * get all of the information we need to write the processor
 * entry from the bootstrap processor.
 * Plus I don't think linux really even cares.
 * Having the proper apicid's in the table so the non-bootstrap
 *  processors can be woken up should be enough.
 */
void smp_write_processors(struct mp_config_table *mc)
{
	int boot_apic_id;
	int order_id;
	unsigned apic_version;
	unsigned cpu_features;
	unsigned cpu_feature_flags;
	struct cpuid_result result;
	device_t cpu;

	boot_apic_id = lapicid();
	apic_version = lapic_read(LAPIC_LVR) & 0xff;
	result = cpuid(1);
	cpu_features = result.eax;
	cpu_feature_flags = result.edx;
	/* order the output of the cpus to fix a bug in kernel 2.6.11 */
	for(order_id = 0;order_id <256; order_id++) {
		for(cpu = all_devices; cpu; cpu = cpu->next) {
			unsigned long cpu_flag;
			if ((cpu->path.type != DEVICE_PATH_APIC) ||
				(cpu->bus->dev->path.type != DEVICE_PATH_CPU_CLUSTER))
				continue;

		if (!cpu->enabled)
			continue;

			cpu_flag = MPC_CPU_ENABLED;

		if (boot_apic_id == cpu->path.apic.apic_id)
			cpu_flag = MPC_CPU_ENABLED | MPC_CPU_BOOTPROCESSOR;

			if(cpu->path.apic.apic_id == order_id) {
				smp_write_processor(mc, cpu->path.apic.apic_id, apic_version,
						cpu_flag, cpu_features, cpu_feature_flags
				);
				break;
			}
		}
	}
}

/*
 * Type 1: Bus Entries:
 * Entry Type, Bus ID, Bus Type
 */
void smp_write_bus(struct mp_config_table *mc,
	u8 id, const char *bustype)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd Bus Entry (Type 1)\n");
#endif

	struct mpc_config_bus *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_BUS;
	mpc->mpc_busid = id;
	memcpy(mpc->mpc_bustype, bustype, sizeof(mpc->mpc_bustype));

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tBus ID\t\t: 0x%x\n"
			"\tBus Type\t: '%s'\n",
			id, bustype);
#endif

	smp_add_mpc_entry(mc, sizeof(*mpc));
}

/*
 * Type 2: I/O APIC Entries:
 * Entry Type, APIC ID, Version,
 * APIC Flags:EN, Address
 */
void smp_write_ioapic(struct mp_config_table *mc,
	u8 id, u8 ver, u32 apicaddr)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd IOAPIC Entry (Type 2)\n");
#endif
	struct mpc_config_ioapic *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_IOAPIC;
	mpc->mpc_apicid = id;
	mpc->mpc_apicver = ver;
	mpc->mpc_flags = MPC_APIC_USABLE;
	mpc->mpc_apicaddr = apicaddr;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tID\t\t: 0x%x\n"
			"\tVersion\t\t: 0x%x\n"
			"\tFlags\t\t: 0x%x (%s)\n"
			"\tAddress\t\t: 0x%x\n",
			id, ver, mpc->mpc_flags, mpc->mpc_flags ? "Useable" : "Unusable",
			apicaddr);
#endif

	smp_add_mpc_entry(mc, sizeof(*mpc));
}

/*
 * Type 3 and 4 MP entries have identical IRQ Types
 * where an integer corresponds to a type which is
 * easier to understand as a string
 */
const char * smp_irqtype_to_str(u8 irqtype)
{
	const char * str[] = {
			"Vectored INT",
			"NMI",
			"SMI",
			"ExtINT",
	};

	if (irqtype < 4)
		return str[irqtype];
	else
		return "Unknown IRQ Type";
}

/*
 * Type 3 and 4 MP entries have an irqflag field
 * that has (Polarity << 2) | (Trigger) data.  This
 * is much easier to read as a string for each MP
 * entry that uses them
 */
char * smp_irqflag_to_str(u16 irqflag)
{
	int polarity = (irqflag & 0x3);		/* Polarity is bottom 2 bits */
	int trigger = (irqflag >> 2) & 0x3;	/* Trigger is next 2 bits */
	char str[1000];
	char * str_ptr;
	int len = 0;

	str_ptr = str;

	/* IRQ Trigger polarity */
	const char * pol[] = {
			"Bus Default",
			"Active High",
			"Reserved",
			"Active Low",
	};

	/* IRQ Trigger mode */
	const char * trig[] = {
			"Bus Default",
			"Edge Triggered",
			"Reserved",
			"Level Triggered",
		};

	/* Create the Polarity|Trigger string */
	if ((strlen(pol[polarity]) + strlen("|") + strlen(trig[trigger]) + 1) < sizeof(str)) {
		strcpy(str, pol[polarity]);
		len = strlen(pol[polarity]);
		strcpy(str + len, "|");
		len += strlen("|");
		strcpy(str + len, trig[trigger]);
	}

	return str_ptr;
}

/*
 * Type 3: I/O Interrupt Table Entries:
 * Entry Type, Int Type, Int Polarity, Int Level,
 * Source Bus ID, Source Bus IRQ, Dest APIC ID, Dest PIN#
 */
void smp_write_intsrc(struct mp_config_table *mc,
	u8 irqtype, u16 irqflag,
	u8 srcbus, u8 srcbusirq,
	u8 dstapic, u8 dstirq)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd IO Interrupt Entry (Type 3)\n");
#endif

	struct mpc_config_intsrc *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_INTSRC;
	mpc->mpc_irqtype = irqtype;
	mpc->mpc_irqflag = irqflag;
	mpc->mpc_srcbus = srcbus;
	mpc->mpc_srcbusirq = srcbusirq;
	mpc->mpc_dstapic = dstapic;
	mpc->mpc_dstirq = dstirq;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tIRQ Type\t: 0x%x (%s)\n"
			"\tIRQ Flags\t: 0x%x (%s)\n"
			"\tSrc Bus ID\t: 0x%x\n"
			"\tSrc Bus IRQ\t: 0x%x\n"
			"\tDest IOAPIC\t: 0x%x\n"
			"\tDest IOAPIC IRQ\t: 0x%x\n",
			irqtype, smp_irqtype_to_str(irqtype),
			irqflag, smp_irqflag_to_str(irqflag),
			srcbus, srcbusirq, dstapic, dstirq);
#endif

	smp_add_mpc_entry(mc, sizeof(*mpc));
}

/*
 * Type 3: I/O Interrupt Table Entries for PCI Devices:
 * This has the same fields as 'Type 3: I/O Interrupt Table Entries'
 * but the Source Bus IRQ field has a slightly different
 * definition:
 * Bits 1-0: PIRQ pin: INT_A# = 0, INT_B# = 1, INT_C# = 2, INT_D# = 3
 * Bits 2-6: Originating PCI Device Number (Not its parent bridge device number)
 * Bit 7: Reserved
 */
void smp_write_pci_intsrc(struct mp_config_table *mc,
	u8 irqtype, u8 srcbus, u8 dev, u8 pirq,
	u8 dstapic, u8 dstirq)
{
	u8 srcbusirq = (dev << 2) | pirq;
	printk(BIOS_SPEW, "\tPCI srcbusirq = 0x%x from dev = 0x%x and pirq = %x\n", srcbusirq, dev, pirq);
	smp_write_intsrc(mc, irqtype, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, srcbus,
			srcbusirq, dstapic, dstirq);
}

void smp_write_intsrc_pci_bridge(struct mp_config_table *mc,
	u8 irqtype, u16 irqflag, struct device *dev,
	unsigned char dstapic, unsigned char *dstirq)
{
	struct device *child;

	int i;
	int srcbus;
	int slot;

	struct bus *link;
	unsigned char dstirq_x[4];

	for (link = dev->link_list; link; link = link->next) {

		child = link->children;
		srcbus = link->secondary;

		while (child) {
			if (child->path.type != DEVICE_PATH_PCI)
				goto next;

			slot = (child->path.pci.devfn >> 3);
			/* round pins */
			for (i = 0; i < 4; i++)
				dstirq_x[i] = dstirq[(i + slot) % 4];

			if ((child->class >> 16) != PCI_BASE_CLASS_BRIDGE) {
				/* pci device */
				printk(BIOS_DEBUG, "route irq: %s\n", dev_path(child));
				for (i = 0; i < 4; i++)
					smp_write_intsrc(mc, irqtype, irqflag, srcbus, (slot<<2)|i, dstapic, dstirq_x[i]);
				goto next;
			}

			switch (child->class>>8) {
			case PCI_CLASS_BRIDGE_PCI:
			case PCI_CLASS_BRIDGE_PCMCIA:
			case PCI_CLASS_BRIDGE_CARDBUS:
				printk(BIOS_DEBUG, "route irq bridge: %s\n", dev_path(child));
				smp_write_intsrc_pci_bridge(mc, irqtype, irqflag, child, dstapic, dstirq_x);
			}

next:
			child = child->sibling;
		}

	}
}

/*
 * Type 4: Local Interrupt Assignment Entries:
 * Entry Type, Int Type, Int Polarity, Int Level,
 * Source Bus ID, Source Bus IRQ, Dest LAPIC ID,
 * Dest LAPIC LINTIN#
 */
void smp_write_lintsrc(struct mp_config_table *mc,
	u8 irqtype, u16 irqflag,
	u8 srcbusid, u8 srcbusirq,
	u8 destapic, u8 destapiclint)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd Local Interrupt Entry (Type 4)\n");
#endif

	struct mpc_config_lintsrc *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_LINTSRC;
	mpc->mpc_irqtype = irqtype;
	mpc->mpc_irqflag = irqflag;
	mpc->mpc_srcbusid = srcbusid;
	mpc->mpc_srcbusirq = srcbusirq;
	mpc->mpc_destapic = destapic;
	mpc->mpc_destapiclint = destapiclint;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tIRQ Type\t: 0x%x (%s)\n"
			"\tIRQ Flags\t: 0x%x (%s)\n"
			"\tSrc Bus ID\t: 0x%x\n"
			"\tSrc Bus IRQ\t: 0x%x\n"
			"\tDest LAPIC\t: 0x%x\n"
			"\tDest LAPIC IRQ\t: 0x%x\n",
			irqtype, smp_irqtype_to_str(irqtype),
			irqflag, smp_irqflag_to_str(irqflag),
			srcbusid, srcbusirq, destapic, destapiclint);
#endif

	smp_add_mpc_entry(mc, sizeof(*mpc));
}

/*
 * Type 128: System Address Space Mapping Entries
 * Entry Type, Entry Length, Bus ID, Address Type,
 * Address Base Lo/Hi, Address Length Lo/Hi
 */
void smp_write_address_space(struct mp_config_table *mc,
	u8 busid, u8 address_type,
	u32 address_base_low, u32 address_base_high,
	u32 address_length_low, u32 address_length_high)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd Address Space Mapping Entry (Type 128)\n");
#endif

	struct mp_exten_system_address_space *mpe;
	mpe = smp_next_mpe_entry(mc);
	memset(mpe, '\0', sizeof(*mpe));
	mpe->mpe_type = MPE_SYSTEM_ADDRESS_SPACE;
	mpe->mpe_length = sizeof(*mpe);
	mpe->mpe_busid = busid;
	mpe->mpe_address_type = address_type;
	mpe->mpe_address_base_low  = address_base_low;
	mpe->mpe_address_base_high = address_base_high;
	mpe->mpe_address_length_low  = address_length_low;
	mpe->mpe_address_length_high = address_length_high;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tEntry Length\t: 0x%x\n"
			"\tBus ID\t\t: 0x%x\n"
			"\tAddress Type\t: 0x%x\n"
			"\tAddress Base\t: 0x%x%x\n"
			"\tAddress Length\t: 0x%x%x\n",
			mpe->mpe_length, busid, address_type, address_base_high, address_base_low,
			address_length_high, address_length_low);
#endif

	smp_add_mpe_entry(mc, (mpe_t)mpe);
}

/*
 * Type 129: Bus Hierarchy Descriptor Entry
 * Entry Type, Entry Length, Bus ID, Bus Info,
 * Parent Bus ID
 */
void smp_write_bus_hierarchy(struct mp_config_table *mc,
	u8 busid, u8 bus_info, u8 parent_busid)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd Bus Hierarchy Entry (Type 129)\n");
#endif

	struct mp_exten_bus_hierarchy *mpe;
	mpe = smp_next_mpe_entry(mc);
	memset(mpe, '\0', sizeof(*mpe));
	mpe->mpe_type = MPE_BUS_HIERARCHY;
	mpe->mpe_length = sizeof(*mpe);
	mpe->mpe_busid = busid;
	mpe->mpe_bus_info = bus_info;
	mpe->mpe_parent_busid = parent_busid;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tEntry Length\t: 0x%x\n"
			"\tBus ID\t\t: 0x%x\n"
			"\tBus Info\t: 0x%x\n"
			"\tParent Bus ID\t: 0x%x\n",
			mpe->mpe_length, busid, bus_info, parent_busid);
#endif

	smp_add_mpe_entry(mc, (mpe_t)mpe);
}

/*
 * Type 130: Compatibility Bus Address Space Modifier Entry
 * Entry Type, Entry Length, Bus ID, Address Modifier
 * Predefined Range List
 */
void smp_write_compatibility_address_space(struct mp_config_table *mc,
	u8 busid, u8 address_modifier,
	u32 range_list)
{
#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "\n\tAdd Compatibility Address Space Entry (Type 130)\n");
#endif

	struct mp_exten_compatibility_address_space *mpe;
	mpe = smp_next_mpe_entry(mc);
	memset(mpe, '\0', sizeof(*mpe));
	mpe->mpe_type = MPE_COMPATIBILITY_ADDRESS_SPACE;
	mpe->mpe_length = sizeof(*mpe);
	mpe->mpe_busid = busid;
	mpe->mpe_address_modifier = address_modifier;
	mpe->mpe_range_list = range_list;

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_SPEW,
			"\tEntry Length\t: 0x%x\n"
			"\tBus ID\t\t: 0x%x\n"
			"\tAddress Modifier: 0x%x\n"
			"\tRange List\t: 0x%x\n",
			mpe->mpe_length, busid, address_modifier, range_list);
#endif

	smp_add_mpe_entry(mc, (mpe_t)mpe);
}

void mptable_lintsrc(struct mp_config_table *mc, unsigned long bus_isa)
{
	smp_write_lintsrc(mc, mp_ExtINT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, MP_APIC_ALL, 0x0);
	smp_write_lintsrc(mc, mp_NMI, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH, bus_isa, 0x0, MP_APIC_ALL, 0x1);
}

void mptable_add_isa_interrupts(struct mp_config_table *mc, unsigned long bus_isa, unsigned long apicid, int external_int2)
{
/*I/O Ints:                   Type         Trigger            Polarity         Bus ID   IRQ  APIC ID   PIN# */
	smp_write_intsrc(mc, external_int2?mp_INT:mp_ExtINT,
	                             MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x0, apicid, 0x0);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x1, apicid, 0x1);
	smp_write_intsrc(mc, external_int2?mp_ExtINT:mp_INT,
	                             MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x0, apicid, 0x2);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x3, apicid, 0x3);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x4, apicid, 0x4);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x6, apicid, 0x6);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x7, apicid, 0x7);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x8, apicid, 0x8);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0x9, apicid, 0x9);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0xa, apicid, 0xa);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0xb, apicid, 0xb);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0xc, apicid, 0xc);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0xd, apicid, 0xd);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0xe, apicid, 0xe);
	smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_EDGE|MP_IRQ_POLARITY_HIGH,  bus_isa, 0xf, apicid, 0xf);
}

void mptable_write_buses(struct mp_config_table *mc, int *max_pci_bus, int *isa_bus) {
	int dummy, i, highest;
	char buses[256];
	struct device *dev;

	if (!max_pci_bus) max_pci_bus = &dummy;
	if (!isa_bus) isa_bus = &dummy;

	*max_pci_bus = 0;
	highest = 0;
	memset(buses, 0, sizeof(buses));

	for (dev = all_devices; dev; dev = dev->next) {
		struct bus *bus;
		for (bus = dev->link_list; bus; bus = bus->next) {
			if (bus->secondary > 255) {
				printk(BIOS_ERR, "A bus claims to have a bus ID > 255?!? Aborting");
				return;
			}
			buses[bus->secondary] = 1;
			if (highest < bus->secondary) highest = bus->secondary;
		}
	}
	for (i=0; i <= highest; i++) {
		if (buses[i]) {
			smp_write_bus(mc, i, "PCI   ");
			*max_pci_bus = i;
		}
	}
	*isa_bus = *max_pci_bus + 1;
	smp_write_bus(mc, *isa_bus, "ISA   ");
}

void *mptable_finalize(struct mp_config_table *mc)
{
	void *last_mpc_entry = smp_next_mpc_entry(mc);
	void *last_mpe_entry = smp_next_mpe_entry(mc);

	mc->mpe_checksum = smp_compute_checksum(last_mpc_entry, mc->mpe_length);
	mc->mpc_checksum = smp_compute_checksum(mc, mc->mpc_length);
	printk(BIOS_INFO, "MP_Tables: Finished writing the MP_Table from 0x%p - 0x%p\n", mc, last_mpe_entry);

#if IS_ENABLED(CONFIG_DEBUG_MP_TABLE)
	printk(BIOS_DEBUG, "MP_Tables: Final MP Configuration Table Header:\n"
			"\tSignature\t\t: %4.4s\n"
			"\tLength\t\t\t: 0x%x\n"
			"\tMP Specification\t: 0x%x\n"
			"\tChecksum\t\t: 0x%x\n"
			"\tOEM ID\t\t\t: %8.8s\n"
			"\tProduct ID\t\t: %12.12s\n"
			"\tOEM Table PTR\t\t: 0x%x\n"
			"\tOEM Table Size\t\t: 0x%x\n"
			"\tEntry Count\t\t: 0x%x\n"
			"\tMMIO LAPIC Address\t: 0x%x\n"
			"\tExtended Table Length\t: 0x%x\n"
			"\tExtended Table Checksum\t: 0x%x\n",
			mc->mpc_signature, mc->mpc_length, mc->mpc_spec, mc->mpc_checksum,
			mc->mpc_oem, mc->mpc_productid, mc->mpc_oemptr, mc->mpc_oemsize,
			mc->mpc_entry_count, mc->mpc_lapic, mc->mpe_length, mc->mpe_checksum);

	printk(BIOS_SPEW, "\nMP_Tables: Hexdump of the MP Configuration Table (0x%x bytes):\n", mc->mpc_length);
	hexdump(BIOS_SPEW, mc, mc->mpc_length);

	if(mc->mpe_length){
		printk(BIOS_SPEW, "\nMP_Tables: Hexdump of the MP Extended Table (0x%x bytes):\n", mc->mpe_length);
		hexdump(BIOS_SPEW, last_mpc_entry, mc->mpe_length);
	}
#endif

	return last_mpe_entry;
}

unsigned long __attribute__((weak)) write_smp_table(unsigned long addr)
{
	struct drivers_generic_ioapic_config *ioapic_config;
        struct mp_config_table *mc;
	int isa_bus, pin, parentpin;
	device_t dev, parent, oldparent;
	void *tmp, *v;
	int isaioapic = -1, have_fixed_entries;

	v = smp_write_floating_table(addr, 0);
        mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);

	mptable_init(mc, LOCAL_APIC_ADDR);

	smp_write_processors(mc);

	mptable_write_buses(mc, NULL, &isa_bus);

	for(dev = all_devices; dev; dev = dev->next) {
		if (dev->path.type != DEVICE_PATH_IOAPIC)
			continue;

		if (!(ioapic_config = dev->chip_info)) {
			printk(BIOS_ERR, "%s has no config, ignoring\n", dev_path(dev));
			continue;
		}
		smp_write_ioapic(mc, dev->path.ioapic.ioapic_id,
				     ioapic_config->version,
				     ioapic_config->base);

		if (ioapic_config->have_isa_interrupts) {
			if (isaioapic >= 0)
				printk(BIOS_ERR, "More than one IOAPIC with ISA interrupts?\n");
			else
				isaioapic = dev->path.ioapic.ioapic_id;
		}
	}

	if (isaioapic >= 0) {
		/* Legacy Interrupts */
		printk(BIOS_DEBUG, "Writing ISA IRQs\n");
		mptable_add_isa_interrupts(mc, isa_bus, isaioapic, 0);
	}

	for(dev = all_devices; dev; dev = dev->next) {

		if (dev->path.type != DEVICE_PATH_PCI || !dev->enabled)
			continue;

		have_fixed_entries = 0;
		for (pin = 0; pin < 4; pin++) {
			if (dev->pci_irq_info[pin].ioapic_dst_id) {
				printk(BIOS_DEBUG, "fixed IRQ entry for: %s: INT%c# -> IOAPIC %d PIN %d\n", dev_path(dev),
				       pin + 'A',
				       dev->pci_irq_info[pin].ioapic_dst_id,
				       dev->pci_irq_info[pin].ioapic_irq_pin);
				smp_write_intsrc(mc, mp_INT,
						 dev->pci_irq_info[pin].ioapic_flags,
						 dev->bus->secondary,
						 ((dev->path.pci.devfn & 0xf8) >> 1) | pin,
						 dev->pci_irq_info[pin].ioapic_dst_id,
						 dev->pci_irq_info[pin].ioapic_irq_pin);
				have_fixed_entries = 1;
			}
		}

		if (!have_fixed_entries) {
			pin = (dev->path.pci.devfn & 7) % 4;
			oldparent = parent = dev;
			while((parent = parent->bus->dev)) {
				parentpin = (oldparent->path.pci.devfn >> 3) + (oldparent->path.pci.devfn & 7);
				parentpin += dev->path.pci.devfn & 7;
				parentpin += dev->path.pci.devfn >> 3;
				parentpin %= 4;

				if (parent->pci_irq_info[parentpin].ioapic_dst_id) {
					printk(BIOS_DEBUG, "automatic IRQ entry for %s: INT%c# -> IOAPIC %d PIN %d\n",
					       dev_path(dev), pin + 'A',
					       parent->pci_irq_info[parentpin].ioapic_dst_id,
					       parent->pci_irq_info[parentpin].ioapic_irq_pin);
					smp_write_intsrc(mc, mp_INT,
							 parent->pci_irq_info[parentpin].ioapic_flags,
							 dev->bus->secondary,
							 ((dev->path.pci.devfn & 0xf8) >> 1) | pin,
							 parent->pci_irq_info[parentpin].ioapic_dst_id,
							 parent->pci_irq_info[parentpin].ioapic_irq_pin);

					break;
				}

				if (parent->path.type == DEVICE_PATH_DOMAIN) {
					printk(BIOS_WARNING, "no IRQ found for %s\n", dev_path(dev));
					break;
				}
				oldparent = parent;
			}
		}
	}

	mptable_lintsrc(mc, isa_bus);
	tmp = mptable_finalize(mc);
	printk(BIOS_INFO, "MPTABLE len: %d\n", (unsigned int)tmp - (unsigned int)v);
	return (unsigned long)tmp;
}
