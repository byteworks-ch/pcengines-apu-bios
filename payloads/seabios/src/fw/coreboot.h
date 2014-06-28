//*****************************************************************************
//
//
// Copyright (c) 2013-2014 Sage Electronic Engineering.  All rights reserved.
// Software License Agreement
//
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// Sage Electronic Engineering SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR
// SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************

#ifndef __COREBOOT_H
#define __COREBOOT_H

struct cbfs_romfile_s {
    struct romfile_s file;
    struct cbfs_file *fhdr;
    void *data;
    u32 rawsize, flags;
};

struct cbmem_entry {
	u32 magic;
	u32 id;
	u64 base;
	u64 size;
};

struct bootorder_container {
    u32  bootorder_signature;   /* "BOOT" */
    u32  bootorder_size;        /* size of bootorder_data[] */
    char bootorder_file_name[16];
    char bootorder_data[0];     /* Variable size */
};

#define CBMEM_ID_BOOTORDER  0x424f4f54
#define CBMEM_MAGIC         0x434f5245

char * get_cbmem_bootorder_file(void);
char *find_coreboot_build_date(void);
u64 find_coreboot_mem_size(void);

#endif
