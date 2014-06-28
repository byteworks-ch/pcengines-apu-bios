#ifndef RESET_H
#define RESET_H

#if CONFIG_HAVE_HARD_RESET
void hard_reset(void);
void reset_system(void);
#else
#define hard_reset() do {} while(0)
#define reset_system() do {} while(0)
#endif
void soft_reset(void);

#endif

#define PCI_RESET_REGISTER	0xcf9
#define  SYS_RST_BIT		(1 << 1)
#define  RST_CPU_BIT		(1 << 2)
#define  PCI_COLD_RESET		( SYS_RST_BIT | RST_CPU_BIT )
#define  PCI_WARM_RESET		RST_CPU_BIT
