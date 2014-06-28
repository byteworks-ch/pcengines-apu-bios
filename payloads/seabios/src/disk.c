// 16bit code to access hard drives.
//
// Copyright (C) 2008  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2002  MandrakeSoft S.A.
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "biosvar.h" // SET_BDA
#include "bregs.h" // struct bregs
#include "config.h" // CONFIG_*
#include "hw/ata.h" // ATA_CB_DC
#include "hw/pci.h" // pci_bdf_to_bus
#include "hw/pic.h" // pic_eoi2
#include "output.h" // debug_enter
#include "stacks.h" // call16_int
#include "std/disk.h" // DISK_RET_SUCCESS
#include "string.h" // memset
#include "util.h" // CDRom_locks


/****************************************************************
 * Helper functions
 ****************************************************************/

static void
__disk_stub(struct bregs *regs, int lineno, const char *fname)
{
    __warn_unimplemented(regs, lineno, fname);
    __disk_ret(regs, DISK_RET_SUCCESS | (lineno << 8), fname);
}

#define DISK_STUB(regs)                         \
    __disk_stub((regs), __LINE__, __func__)

// Get the cylinders/heads/sectors for the given drive.
static struct chs_s
getLCHS(struct drive_s *drive_gf)
{
    struct chs_s res = { };
    if (CONFIG_CDROM_EMU && drive_gf == GET_GLOBAL(cdemu_drive_gf)) {
        // Emulated drive - get info from CDEmu.  (It's not possible to
        // populate the geometry directly in the driveid because the
        // geometry is only known after the bios segment is made
        // read-only).
        res.cylinder = GET_LOW(CDEmu.lchs.cylinder);
        res.head = GET_LOW(CDEmu.lchs.head);
        res.sector = GET_LOW(CDEmu.lchs.sector);
        return res;
    }
    res.cylinder = GET_GLOBALFLAT(drive_gf->lchs.cylinder);
    res.head = GET_GLOBALFLAT(drive_gf->lchs.head);
    res.sector = GET_GLOBALFLAT(drive_gf->lchs.sector);
    return res;
}

// Perform read/write/verify using old-style chs accesses
static void noinline
basic_access(struct bregs *regs, struct drive_s *drive_gf, u16 command)
{
    struct disk_op_s dop;
    dop.drive_gf = drive_gf;
    dop.command = command;

    u8 count = regs->al;
    u16 cylinder = regs->ch | ((((u16)regs->cl) << 2) & 0x300);
    u16 sector = regs->cl & 0x3f;
    u16 head = regs->dh;

    if (count > 128 || count == 0 || sector == 0) {
        warn_invalid(regs);
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }
    dop.count = count;

    struct chs_s chs = getLCHS(drive_gf);
    u16 nlc=chs.cylinder, nlh=chs.head, nls=chs.sector;

    // sanity check on cyl heads, sec
    if (cylinder >= nlc || head >= nlh || sector > nls) {
        warn_invalid(regs);
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }

    // translate lchs to lba
    dop.lba = (((((u32)cylinder * (u32)nlh) + (u32)head) * (u32)nls)
               + (u32)sector - 1);

    dop.buf_fl = MAKE_FLATPTR(regs->es, regs->bx);

    int status = send_disk_op(&dop);

    regs->al = dop.count;

    disk_ret(regs, status);
}

// Perform read/write/verify using new-style "int13ext" accesses.
static void noinline
extended_access(struct bregs *regs, struct drive_s *drive_gf, u16 command)
{
    struct disk_op_s dop;
    struct int13ext_s *param_far = (struct int13ext_s*)(regs->si+0);
    // Get lba and check.
    dop.lba = GET_FARVAR(regs->ds, param_far->lba);
    dop.command = command;
    dop.drive_gf = drive_gf;
    if (dop.lba >= GET_GLOBALFLAT(drive_gf->sectors)) {
        warn_invalid(regs);
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }

    dop.buf_fl = SEGOFF_TO_FLATPTR(GET_FARVAR(regs->ds, param_far->data));
    dop.count = GET_FARVAR(regs->ds, param_far->count);
    if (! dop.count) {
        // Nothing to do.
        disk_ret(regs, DISK_RET_SUCCESS);
        return;
    }

    int status = send_disk_op(&dop);

    SET_FARVAR(regs->ds, param_far->count, dop.count);

    disk_ret(regs, status);
}


/****************************************************************
 * Hard Drive functions
 ****************************************************************/

// disk controller reset
static void
disk_1300(struct bregs *regs, struct drive_s *drive_gf)
{
    struct disk_op_s dop;
    dop.drive_gf = drive_gf;
    dop.command = CMD_RESET;
    int status = send_disk_op(&dop);
    disk_ret(regs, status);
}

// read disk status
static void
disk_1301(struct bregs *regs, struct drive_s *drive_gf)
{
    u8 v;
    if (regs->dl < EXTSTART_HD)
        // Floppy
        v = GET_BDA(floppy_last_status);
    else
        v = GET_BDA(disk_last_status);
    regs->ah = v;
    set_cf(regs, v);
    // XXX - clear disk_last_status?
}

// read disk sectors
static void
disk_1302(struct bregs *regs, struct drive_s *drive_gf)
{
    basic_access(regs, drive_gf, CMD_READ);
}

// write disk sectors
static void
disk_1303(struct bregs *regs, struct drive_s *drive_gf)
{
    basic_access(regs, drive_gf, CMD_WRITE);
}

// verify disk sectors
static void
disk_1304(struct bregs *regs, struct drive_s *drive_gf)
{
    basic_access(regs, drive_gf, CMD_VERIFY);
}

// format disk track
static void noinline
disk_1305(struct bregs *regs, struct drive_s *drive_gf)
{
    debug_stub(regs);

    struct chs_s chs = getLCHS(drive_gf);
    u16 nlc=chs.cylinder, nlh=chs.head, nls=chs.sector;

    u8 count = regs->al;
    u8 cylinder = regs->ch;
    u8 head = regs->dh;

    if (cylinder >= nlc || head >= nlh || count == 0 || count > nls) {
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }

    struct disk_op_s dop;
    dop.drive_gf = drive_gf;
    dop.command = CMD_FORMAT;
    dop.lba = (((u32)cylinder * (u32)nlh) + (u32)head) * (u32)nls;
    dop.count = count;
    dop.buf_fl = MAKE_FLATPTR(regs->es, regs->bx);
    int status = send_disk_op(&dop);
    disk_ret(regs, status);
}

// read disk drive parameters
static void noinline
disk_1308(struct bregs *regs, struct drive_s *drive_gf)
{
    // Get logical geometry from table
    struct chs_s chs = getLCHS(drive_gf);
    u16 nlc=chs.cylinder, nlh=chs.head, nls=chs.sector;
    nlc--;
    nlh--;
    u8 count;
    if (regs->dl < EXTSTART_HD) {
        // Floppy
        count = GET_GLOBAL(FloppyCount);

        if (CONFIG_CDROM_EMU && drive_gf == GET_GLOBAL(cdemu_drive_gf))
            regs->bx = GET_LOW(CDEmu.media) * 2;
        else
            regs->bx = GET_GLOBALFLAT(drive_gf->floppy_type);

        // set es & di to point to 11 byte diskette param table in ROM
        regs->es = SEG_BIOS;
        regs->di = (u32)&diskette_param_table2;
    } else if (regs->dl < EXTSTART_CD) {
        // Hard drive
        count = GET_BDA(hdcount);
        nlc--;  // last sector reserved
    } else {
        // Not supported on CDROM
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }

    if (CONFIG_CDROM_EMU && GET_LOW(CDEmu.active)) {
        u8 emudrive = GET_LOW(CDEmu.emulated_extdrive);
        if (((emudrive ^ regs->dl) & 0x80) == 0)
            // Note extra drive due to emulation.
            count++;
        if (regs->dl < EXTSTART_HD && count > 2)
            // Max of two floppy drives.
            count = 2;
    }

    regs->al = 0;
    regs->ch = nlc & 0xff;
    regs->cl = ((nlc >> 2) & 0xc0) | (nls & 0x3f);
    regs->dh = nlh;

    disk_ret(regs, DISK_RET_SUCCESS);
    regs->dl = count;
}

// initialize drive parameters
static void
disk_1309(struct bregs *regs, struct drive_s *drive_gf)
{
    DISK_STUB(regs);
}

// seek to specified cylinder
static void
disk_130c(struct bregs *regs, struct drive_s *drive_gf)
{
    DISK_STUB(regs);
}

// alternate disk reset
static void
disk_130d(struct bregs *regs, struct drive_s *drive_gf)
{
    DISK_STUB(regs);
}

// check drive ready
static void
disk_1310(struct bregs *regs, struct drive_s *drive_gf)
{
    // should look at 40:8E also???

    struct disk_op_s dop;
    dop.drive_gf = drive_gf;
    dop.command = CMD_ISREADY;
    int status = send_disk_op(&dop);
    disk_ret(regs, status);
}

// recalibrate
static void
disk_1311(struct bregs *regs, struct drive_s *drive_gf)
{
    DISK_STUB(regs);
}

// controller internal diagnostic
static void
disk_1314(struct bregs *regs, struct drive_s *drive_gf)
{
    DISK_STUB(regs);
}

// read disk drive size
static void noinline
disk_1315(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret(regs, DISK_RET_SUCCESS);
    if (regs->dl < EXTSTART_HD || regs->dl >= EXTSTART_CD) {
        // Floppy or cdrom
        regs->ah = 1;
        return;
    }
    // Hard drive

    // Get logical geometry from table
    struct chs_s chs = getLCHS(drive_gf);
    u16 nlc=chs.cylinder, nlh=chs.head, nls=chs.sector;

    // Compute sector count seen by int13
    u32 lba = (u32)(nlc - 1) * (u32)nlh * (u32)nls;
    regs->cx = lba >> 16;
    regs->dx = lba & 0xffff;
    regs->ah = 3; // hard disk accessible
}

static void
disk_1316(struct bregs *regs, struct drive_s *drive_gf)
{
    if (regs->dl >= EXTSTART_HD) {
        // Hard drive
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }
    disk_ret(regs, DISK_RET_ECHANGED);
}

// IBM/MS installation check
static void
disk_1341(struct bregs *regs, struct drive_s *drive_gf)
{
    regs->bx = 0xaa55;  // install check
    regs->cx = 0x0007;  // ext disk access and edd, removable supported
    disk_ret(regs, DISK_RET_SUCCESS);
    regs->ah = 0x30;    // EDD 3.0
}

// IBM/MS extended read
static void
disk_1342(struct bregs *regs, struct drive_s *drive_gf)
{
    extended_access(regs, drive_gf, CMD_READ);
}

// IBM/MS extended write
static void
disk_1343(struct bregs *regs, struct drive_s *drive_gf)
{
    extended_access(regs, drive_gf, CMD_WRITE);
}

// IBM/MS verify
static void
disk_1344(struct bregs *regs, struct drive_s *drive_gf)
{
    extended_access(regs, drive_gf, CMD_VERIFY);
}

// lock
static void
disk_134500(struct bregs *regs, struct drive_s *drive_gf)
{
    int cdid = regs->dl - EXTSTART_CD;
    u8 locks = GET_LOW(CDRom_locks[cdid]);
    if (locks == 0xff) {
        regs->al = 1;
        disk_ret(regs, DISK_RET_ETOOMANYLOCKS);
        return;
    }
    SET_LOW(CDRom_locks[cdid], locks + 1);
    regs->al = 1;
    disk_ret(regs, DISK_RET_SUCCESS);
}

// unlock
static void
disk_134501(struct bregs *regs, struct drive_s *drive_gf)
{
    int cdid = regs->dl - EXTSTART_CD;
    u8 locks = GET_LOW(CDRom_locks[cdid]);
    if (locks == 0x00) {
        regs->al = 0;
        disk_ret(regs, DISK_RET_ENOTLOCKED);
        return;
    }
    locks--;
    SET_LOW(CDRom_locks[cdid], locks);
    regs->al = (locks ? 1 : 0);
    disk_ret(regs, DISK_RET_SUCCESS);
}

// status
static void
disk_134502(struct bregs *regs, struct drive_s *drive_gf)
{
    int cdid = regs->dl - EXTSTART_CD;
    u8 locks = GET_LOW(CDRom_locks[cdid]);
    regs->al = (locks ? 1 : 0);
    disk_ret(regs, DISK_RET_SUCCESS);
}

static void
disk_1345XX(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret_unimplemented(regs, DISK_RET_EPARAM);
}

// IBM/MS lock/unlock drive
static void
disk_1345(struct bregs *regs, struct drive_s *drive_gf)
{
    if (regs->dl < EXTSTART_CD) {
        // Always success for HD
        disk_ret(regs, DISK_RET_SUCCESS);
        return;
    }

    switch (regs->al) {
    case 0x00: disk_134500(regs, drive_gf); break;
    case 0x01: disk_134501(regs, drive_gf); break;
    case 0x02: disk_134502(regs, drive_gf); break;
    default:   disk_1345XX(regs, drive_gf); break;
    }
}

// IBM/MS eject media
static void noinline
disk_1346(struct bregs *regs, struct drive_s *drive_gf)
{
    if (regs->dl < EXTSTART_CD) {
        // Volume Not Removable
        disk_ret(regs, DISK_RET_ENOTREMOVABLE);
        return;
    }

    int cdid = regs->dl - EXTSTART_CD;
    u8 locks = GET_LOW(CDRom_locks[cdid]);
    if (locks != 0) {
        disk_ret(regs, DISK_RET_ELOCKED);
        return;
    }

    // FIXME should handle 0x31 no media in device
    // FIXME should handle 0xb5 valid request failed

    // Call removable media eject
    struct bregs br;
    memset(&br, 0, sizeof(br));
    br.ah = 0x52;
    br.dl = regs->dl;
    call16_int(0x15, &br);

    if (br.ah || br.flags & F_CF) {
        disk_ret(regs, DISK_RET_ELOCKED);
        return;
    }
    disk_ret(regs, DISK_RET_SUCCESS);
}

// IBM/MS extended seek
static void
disk_1347(struct bregs *regs, struct drive_s *drive_gf)
{
    extended_access(regs, drive_gf, CMD_SEEK);
}

// IBM/MS get drive parameters
static void noinline
disk_1348(struct bregs *regs, struct drive_s *drive_gf)
{
    u16 seg = regs->ds;
    struct int13dpt_s *param_far = (struct int13dpt_s*)(regs->si+0);
    u16 size = GET_FARVAR(seg, param_far->size);
    u16 t13 = size == 74;

    // Buffer is too small
    if (size < 26) {
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }

    // EDD 1.x

    u8  type    = GET_GLOBALFLAT(drive_gf->type);
    u16 npc     = GET_GLOBALFLAT(drive_gf->pchs.cylinder);
    u16 nph     = GET_GLOBALFLAT(drive_gf->pchs.head);
    u16 nps     = GET_GLOBALFLAT(drive_gf->pchs.sector);
    u64 lba     = GET_GLOBALFLAT(drive_gf->sectors);
    u16 blksize = GET_GLOBALFLAT(drive_gf->blksize);

    dprintf(DEBUG_HDL_13, "disk_1348 size=%d t=%d chs=%d,%d,%d lba=%d bs=%d\n"
            , size, type, npc, nph, nps, (u32)lba, blksize);

    SET_FARVAR(seg, param_far->size, 26);
    if (type == DTYPE_ATA_ATAPI) {
        // 0x74 = removable, media change, lockable, max values
        SET_FARVAR(seg, param_far->infos, 0x74);
        SET_FARVAR(seg, param_far->cylinders, 0xffffffff);
        SET_FARVAR(seg, param_far->heads, 0xffffffff);
        SET_FARVAR(seg, param_far->spt, 0xffffffff);
        SET_FARVAR(seg, param_far->sector_count, (u64)-1);
    } else {
        if (lba > (u64)nps*nph*0x3fff) {
            SET_FARVAR(seg, param_far->infos, 0x00); // geometry is invalid
            SET_FARVAR(seg, param_far->cylinders, 0x3fff);
        } else {
            SET_FARVAR(seg, param_far->infos, 0x02); // geometry is valid
            SET_FARVAR(seg, param_far->cylinders, (u32)npc);
        }
        SET_FARVAR(seg, param_far->heads, (u32)nph);
        SET_FARVAR(seg, param_far->spt, (u32)nps);
        SET_FARVAR(seg, param_far->sector_count, lba);
    }
    SET_FARVAR(seg, param_far->blksize, blksize);

    if (size < 30 ||
        (type != DTYPE_ATA && type != DTYPE_ATA_ATAPI &&
         type != DTYPE_VIRTIO_BLK && type != DTYPE_VIRTIO_SCSI)) {
        disk_ret(regs, DISK_RET_SUCCESS);
        return;
    }

    // EDD 2.x

    int bdf;
    u16 iobase1 = 0;
    u64 device_path = 0;
    u8 channel = 0;
    SET_FARVAR(seg, param_far->size, 30);
    if (type == DTYPE_ATA || type == DTYPE_ATA_ATAPI) {
        SET_FARVAR(seg, param_far->dpte, SEGOFF(SEG_LOW, (u32)&DefaultDPTE));

        // Fill in dpte
        struct atadrive_s *adrive_gf = container_of(
            drive_gf, struct atadrive_s, drive);
        struct ata_channel_s *chan_gf = GET_GLOBALFLAT(adrive_gf->chan_gf);
        u8 slave = GET_GLOBALFLAT(adrive_gf->slave);
        u16 iobase2 = GET_GLOBALFLAT(chan_gf->iobase2);
        u8 irq = GET_GLOBALFLAT(chan_gf->irq);
        iobase1 = GET_GLOBALFLAT(chan_gf->iobase1);
        bdf = GET_GLOBALFLAT(chan_gf->pci_bdf);
        device_path = slave;
        channel = GET_GLOBALFLAT(chan_gf->chanid);

        u16 options = 0;
        if (type == DTYPE_ATA) {
            u8 translation = GET_GLOBALFLAT(drive_gf->translation);
            if (translation != TRANSLATION_NONE) {
                options |= 1<<3; // CHS translation
                if (translation == TRANSLATION_LBA)
                    options |= 1<<9;
                if (translation == TRANSLATION_RECHS)
                    options |= 3<<9;
            }
        } else {
            // ATAPI
            options |= 1<<5; // removable device
            options |= 1<<6; // atapi device
        }
        options |= 1<<4; // lba translation
        if (CONFIG_ATA_PIO32)
            options |= 1<<7;

        SET_LOW(DefaultDPTE.iobase1, iobase1);
        SET_LOW(DefaultDPTE.iobase2, iobase2 + ATA_CB_DC);
        SET_LOW(DefaultDPTE.prefix, ((slave ? ATA_CB_DH_DEV1 : ATA_CB_DH_DEV0)
                                  | ATA_CB_DH_LBA));
        SET_LOW(DefaultDPTE.unused, 0xcb);
        SET_LOW(DefaultDPTE.irq, irq);
        SET_LOW(DefaultDPTE.blkcount, 1);
        SET_LOW(DefaultDPTE.dma, 0);
        SET_LOW(DefaultDPTE.pio, 0);
        SET_LOW(DefaultDPTE.options, options);
        SET_LOW(DefaultDPTE.reserved, 0);
        SET_LOW(DefaultDPTE.revision, 0x11);

        u8 sum = checksum_far(SEG_LOW, &DefaultDPTE, 15);
        SET_LOW(DefaultDPTE.checksum, -sum);
    } else {
        SET_FARVAR(seg, param_far->dpte.segoff, 0xffffffff);
        bdf = GET_GLOBALFLAT(drive_gf->cntl_id);
    }

    if (size < 66) {
        disk_ret(regs, DISK_RET_SUCCESS);
        return;
    }

    // EDD 3.x
    SET_FARVAR(seg, param_far->key, 0xbedd);
    SET_FARVAR(seg, param_far->dpi_length, t13 ? 44 : 36);
    SET_FARVAR(seg, param_far->reserved1, 0);
    SET_FARVAR(seg, param_far->reserved2, 0);

    if (bdf != -1) {
        SET_FARVAR(seg, param_far->host_bus[0], 'P');
        SET_FARVAR(seg, param_far->host_bus[1], 'C');
        SET_FARVAR(seg, param_far->host_bus[2], 'I');
        SET_FARVAR(seg, param_far->host_bus[3], ' ');

        u32 path = (pci_bdf_to_bus(bdf) | (pci_bdf_to_dev(bdf) << 8)
                    | (pci_bdf_to_fn(bdf) << 16));
        if (t13)
            path |= channel << 24;

        SET_FARVAR(seg, param_far->iface_path, path);
    } else {
        // ISA
        SET_FARVAR(seg, param_far->host_bus[0], 'I');
        SET_FARVAR(seg, param_far->host_bus[1], 'S');
        SET_FARVAR(seg, param_far->host_bus[2], 'A');
        SET_FARVAR(seg, param_far->host_bus[3], ' ');

        SET_FARVAR(seg, param_far->iface_path, iobase1);
    }

    if (type != DTYPE_VIRTIO_BLK) {
        SET_FARVAR(seg, param_far->iface_type[0], 'A');
        SET_FARVAR(seg, param_far->iface_type[1], 'T');
        SET_FARVAR(seg, param_far->iface_type[2], 'A');
        SET_FARVAR(seg, param_far->iface_type[3], ' ');
    } else {
        SET_FARVAR(seg, param_far->iface_type[0], 'S');
        SET_FARVAR(seg, param_far->iface_type[1], 'C');
        SET_FARVAR(seg, param_far->iface_type[2], 'S');
        SET_FARVAR(seg, param_far->iface_type[3], 'I');
    }
    SET_FARVAR(seg, param_far->iface_type[4], ' ');
    SET_FARVAR(seg, param_far->iface_type[5], ' ');
    SET_FARVAR(seg, param_far->iface_type[6], ' ');
    SET_FARVAR(seg, param_far->iface_type[7], ' ');

    if (t13) {
        SET_FARVAR(seg, param_far->t13.device_path[0], device_path);
        SET_FARVAR(seg, param_far->t13.device_path[1], 0);

        SET_FARVAR(seg, param_far->t13.checksum
                   , -checksum_far(seg, (void*)param_far+30, 43));
    } else {
        SET_FARVAR(seg, param_far->phoenix.device_path, device_path);

        SET_FARVAR(seg, param_far->phoenix.checksum
                   , -checksum_far(seg, (void*)param_far+30, 35));
    }

    disk_ret(regs, DISK_RET_SUCCESS);
}

// IBM/MS extended media change
static void
disk_1349(struct bregs *regs, struct drive_s *drive_gf)
{
    if (regs->dl < EXTSTART_CD) {
        // Always success for HD
        disk_ret(regs, DISK_RET_SUCCESS);
        return;
    }
    set_invalid(regs);
    // always send changed ??
    regs->ah = DISK_RET_ECHANGED;
}

static void
disk_134e01(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret(regs, DISK_RET_SUCCESS);
}

static void
disk_134e03(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret(regs, DISK_RET_SUCCESS);
}

static void
disk_134e04(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret(regs, DISK_RET_SUCCESS);
}

static void
disk_134e06(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret(regs, DISK_RET_SUCCESS);
}

static void
disk_134eXX(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret(regs, DISK_RET_EPARAM);
}

// IBM/MS set hardware configuration
static void
disk_134e(struct bregs *regs, struct drive_s *drive_gf)
{
    switch (regs->al) {
    case 0x01: disk_134e01(regs, drive_gf); break;
    case 0x03: disk_134e03(regs, drive_gf); break;
    case 0x04: disk_134e04(regs, drive_gf); break;
    case 0x06: disk_134e06(regs, drive_gf); break;
    default:   disk_134eXX(regs, drive_gf); break;
    }
}

static void
disk_13XX(struct bregs *regs, struct drive_s *drive_gf)
{
    disk_ret_unimplemented(regs, DISK_RET_EPARAM);
}

static void
disk_13(struct bregs *regs, struct drive_s *drive_gf)
{
    //debug_stub(regs);

    // clear completion flag
    SET_BDA(disk_interrupt_flag, 0);

    switch (regs->ah) {
    case 0x00: disk_1300(regs, drive_gf); break;
    case 0x01: disk_1301(regs, drive_gf); break;
    case 0x02: disk_1302(regs, drive_gf); break;
    case 0x03: disk_1303(regs, drive_gf); break;
    case 0x04: disk_1304(regs, drive_gf); break;
    case 0x05: disk_1305(regs, drive_gf); break;
    case 0x08: disk_1308(regs, drive_gf); break;
    case 0x09: disk_1309(regs, drive_gf); break;
    case 0x0c: disk_130c(regs, drive_gf); break;
    case 0x0d: disk_130d(regs, drive_gf); break;
    case 0x10: disk_1310(regs, drive_gf); break;
    case 0x11: disk_1311(regs, drive_gf); break;
    case 0x14: disk_1314(regs, drive_gf); break;
    case 0x15: disk_1315(regs, drive_gf); break;
    case 0x16: disk_1316(regs, drive_gf); break;
    case 0x41: disk_1341(regs, drive_gf); break;
    case 0x42: disk_1342(regs, drive_gf); break;
    case 0x43: disk_1343(regs, drive_gf); break;
    case 0x44: disk_1344(regs, drive_gf); break;
    case 0x45: disk_1345(regs, drive_gf); break;
    case 0x46: disk_1346(regs, drive_gf); break;
    case 0x47: disk_1347(regs, drive_gf); break;
    case 0x48: disk_1348(regs, drive_gf); break;
    case 0x49: disk_1349(regs, drive_gf); break;
    case 0x4e: disk_134e(regs, drive_gf); break;
    default:   disk_13XX(regs, drive_gf); break;
    }
}

static void
floppy_13(struct bregs *regs, struct drive_s *drive_gf)
{
    // Only limited commands are supported on floppies.
    switch (regs->ah) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x08:
    case 0x15:
    case 0x16:
        disk_13(regs, drive_gf);
        break;
    default:   disk_13XX(regs, drive_gf); break;
    }
}


/****************************************************************
 * Entry points
 ****************************************************************/

static void
handle_legacy_disk(struct bregs *regs, u8 extdrive)
{
    if (! CONFIG_DRIVES) {
        // XXX - support handle_1301 anyway?
        disk_ret(regs, DISK_RET_EPARAM);
        return;
    }

    if (extdrive < EXTSTART_HD) {
        struct drive_s *drive_gf = getDrive(EXTTYPE_FLOPPY, extdrive);
        if (!drive_gf)
            goto fail;
        floppy_13(regs, drive_gf);
        return;
    }

    struct drive_s *drive_gf;
    if (extdrive >= EXTSTART_CD)
        drive_gf = getDrive(EXTTYPE_CD, extdrive - EXTSTART_CD);
    else
        drive_gf = getDrive(EXTTYPE_HD, extdrive - EXTSTART_HD);
    if (!drive_gf)
        goto fail;
    disk_13(regs, drive_gf);
    return;

fail:
    // XXX - support 1301/1308/1315 anyway?
    disk_ret(regs, DISK_RET_EPARAM);
}

void VISIBLE16
handle_40(struct bregs *regs)
{
    debug_enter(regs, DEBUG_HDL_40);
    handle_legacy_disk(regs, regs->dl);
}

// INT 13h Fixed Disk Services Entry Point
void VISIBLE16
handle_13(struct bregs *regs)
{
    debug_enter(regs, DEBUG_HDL_13);
    u8 extdrive = regs->dl;

    if (CONFIG_CDROM_EMU) {
        if (regs->ah == 0x4b) {
            cdemu_134b(regs);
            return;
        }
        if (GET_LOW(CDEmu.active)) {
            u8 emudrive = GET_LOW(CDEmu.emulated_extdrive);
            if (extdrive == emudrive) {
                // Access to an emulated drive.
                struct drive_s *cdemu_gf = GET_GLOBAL(cdemu_drive_gf);
                if (regs->ah > 0x16) {
                    // Only old-style commands supported.
                    disk_13XX(regs, cdemu_gf);
                    return;
                }
                disk_13(regs, cdemu_gf);
                return;
            }
            if (extdrive < EXTSTART_CD && ((emudrive ^ extdrive) & 0x80) == 0)
                // Adjust id to make room for emulated drive.
                extdrive--;
        }
    }
    handle_legacy_disk(regs, extdrive);
}

// record completion in BIOS task complete flag
void VISIBLE16
handle_76(void)
{
    debug_isr(DEBUG_ISR_76);
    SET_BDA(disk_interrupt_flag, 0xff);
    pic_eoi2();
}

// Old Fixed Disk Parameter Table (newer tables are in the ebda).
struct fdpt_s OldFDPT VAR16FIXED(0xe401);
