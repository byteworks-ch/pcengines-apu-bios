#ifndef CPU_CPU_H
#define CPU_CPU_H

#include <arch/cpu.h>

#if !defined(__PRE_RAM__) && !defined(__SMM__)
void cpu_initialize(unsigned int cpu_index);
struct bus;
void initialize_cpus(struct bus *cpu_bus);
void asmlinkage secondary_cpu_init(unsigned int cpu_index);

#if CONFIG_HAVE_SMI_HANDLER
void smm_init(void);
void smm_lock(void);
void smm_setup_structures(void *gnvs, void *tcg, void *smi1);
#endif

#define __cpu_driver __attribute__ ((used,__section__(".rodata.cpu_driver")))
#ifndef __SIMPLE_DEVICE__
/** start of compile time generated pci driver array */
extern struct cpu_driver cpu_drivers[];
/** end of compile time generated pci driver array */
extern struct cpu_driver ecpu_drivers[];
#endif
#endif /* !__PRE_RAM__ && !__SMM__ */

/* CPUID values */
#define CPUID_BASIC_INFORMATION		0x00
#define CPUID_VERSION_INFORMATION	0x01
# define ECX_CPU_SUPPORTS_SSE3				(1 << 0)
# define ECX_CPU_SUPPORTS_PCLMULQDQ			(1 << 1)
# define ECX_CPU_SUPPORTS_DTES64			(1 << 2)
# define ECX_CPU_SUPPORTS_MONITOR			(1 << 3)
# define ECX_CPU_SUPPORTS_DS_CPL			(1 << 4)
# define ECX_CPU_SUPPORTS_VMX				(1 << 5)
# define ECX_CPU_SUPPORTS_SMX				(1 << 6)
# define ECX_CPU_SUPPORTS_EIST				(1 << 7)
# define ECX_CPU_SUPPORTS_TM2				(1 << 8)
# define ECX_CPU_SUPPORTS_SSSE3				(1 << 9)
# define ECX_CPU_SUPPORTS_CNXT_ID			(1 << 10)
# define ECX_CPU_SUPPORTS_RESERVED1			(1 << 11)
# define ECX_CPU_SUPPORTS_FMA				(1 << 12)
# define ECX_CPU_SUPPORTS_CMPXCHG16B		(1 << 13)
# define ECX_CPU_SUPPORTS_XTPR_UC			(1 << 14)
# define ECX_CPU_SUPPORTS_PDCM				(1 << 15)
# define ECX_CPU_SUPPORTS_RESERVED2			(1 << 16)
# define ECX_CPU_SUPPORTS_PCID				(1 << 17)
# define ECX_CPU_SUPPORTS_DCA				(1 << 18)
# define ECX_CPU_SUPPORTS_SSE4_1			(1 << 19)
# define ECX_CPU_SUPPORTS_SSE4_2			(1 << 20)
# define ECX_CPU_SUPPORTS_X2APIC			(1 << 21)
# define ECX_CPU_SUPPORTS_MOVBE				(1 << 22)
# define ECX_CPU_SUPPORTS_POPCNT			(1 << 23)
# define ECX_CPU_SUPPORTS_TSC_DEADLINE		(1 << 24)
# define ECX_CPU_SUPPORTS_AESNI				(1 << 25)
# define ECX_CPU_SUPPORTS_XSAVE				(1 << 26)
# define ECX_CPU_SUPPORTS_OSXSAVE			(1 << 27)
# define ECX_CPU_SUPPORTS_AVX				(1 << 28)
# define ECX_CPU_SUPPORTS_F16C				(1 << 29)
# define ECX_CPU_SUPPORTS_RDRAND			(1 << 30)
# define ECX_CPU_SUPPORTS_UNUSED			(1 << 31)
# define EDX_CPU_SUPPORTS_FPU				(1 << 0)
# define EDX_CPU_SUPPORTS_VME				(1 << 1)
# define EDX_CPU_SUPPORTS_DE				(1 << 2)
# define EDX_CPU_SUPPORTS_PSE				(1 << 3)
# define EDX_CPU_SUPPORTS_TSC				(1 << 4)
# define EDX_CPU_SUPPORTS_MSR				(1 << 5)
# define EDX_CPU_SUPPORTS_PAE				(1 << 6)
# define EDX_CPU_SUPPORTS_MCE				(1 << 7)
# define EDX_CPU_SUPPORTS_CX8				(1 << 8)
# define EDX_CPU_SUPPORTS_APIC				(1 << 9)
# define EDX_CPU_SUPPORTS_RESERVED1			(1 << 10)
# define EDX_CPU_SUPPORTS_SEP				(1 << 11)
# define EDX_CPU_SUPPORTS_MTRR				(1 << 12)
# define EDX_CPU_SUPPORTS_PGE				(1 << 13)
# define EDX_CPU_SUPPORTS_MCA				(1 << 14)
# define EDX_CPU_SUPPORTS_CMOV				(1 << 15)
# define EDX_CPU_SUPPORTS_PAT				(1 << 16)
# define EDX_CPU_SUPPORTS_PSE_36			(1 << 17)
# define EDX_CPU_SUPPORTS_PSN				(1 << 18)
# define EDX_CPU_SUPPORTS_CLFSH				(1 << 19)
# define EDX_CPU_SUPPORTS_RESERVED2			(1 << 20)
# define EDX_CPU_SUPPORTS_DS				(1 << 21)
# define EDX_CPU_SUPPORTS_ACPI				(1 << 22)
# define EDX_CPU_SUPPORTS_MMX				(1 << 23)
# define EDX_CPU_SUPPORTS_FXSR				(1 << 24)
# define EDX_CPU_SUPPORTS_SSE				(1 << 25)
# define EDX_CPU_SUPPORTS_SSE2				(1 << 26)
# define EDX_CPU_SUPPORTS_SS				(1 << 27)
# define EDX_CPU_SUPPORTS_HTT				(1 << 28)
# define EDX_CPU_SUPPORTS_TM				(1 << 29)
# define EDX_CPU_SUPPORTS_RESERVED3			(1 << 30)
# define EDX_CPU_SUPPORTS_PBE				(1 << 31)
#define CPUID_CACHE_INFORMATION			0x02
#define CPUID_PSN_BITS_0_63				0x03
#define CPUID_CACHE_PARAM_LEAF			0x04
#define CPUID_MWAIT_LEAF				0x05
#define CPUID_THERMAL_PM_LEAF			0x06
#define CPUID_EXT_FEATURE_FLAGS_LEAF	0x07
#define CPUID_DIRECT_CACHE_INFO_LEAF	0x09
#define CPUID_PERF_MON_LEAF				0x0A
#define CPUID_EXT_TOPOLOGY_LEAF			0x0B
#define CPUID_EXT_STATE_ENUM_LEAF		0x0D
#define CPUID_QOS_RESOURCE_LEAF			0x0F
#define EXT_CPUID_MAX_INPUT_VAL			0x80000000
#define EXT_CPUID_EXT_PROC_FEATURES1	0x80000001
#define EXT_CPUID_PROC_BRAND_STR1		0x80000002
#define EXT_CPUID_PROC_BRAND_STR2		0x80000003
#define EXT_CPUID_PROC_BRAND_STR3		0x80000004
#define EXT_CPUID_EXT_PROC_FEATURES2	0x80000006
#define EXT_CPUID_EXT_PROC_FEATURES3	0x80000007
#define EXT_CPUID_EXT_PROC_FEATURES4	0x80000008

#endif /* CPU_CPU_H */
