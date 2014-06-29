# pcengines-apu-bios

This documentation covers the whole process of building and updating the bios for the [PC Engines apu system board](http://www.pcengines.ch/apu.htm).

## Project

### Status

This software is currently stable, but use it at your own risk. If you have any issues or feature requests, we would love to know, please open an issue. Contributions and pull requests are also very welcome.


### License

This code and documentation is free; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.


### Author

[Byteworks GmbH](http://www.byteworks.ch)

Michael Bischof <mb@byteworks.ch>


### Credits

- [Sage Electronic Engineering](http://www.se-eng.com)  - Development of the bios on behalf of PC Engines
- [PC Engines](http://www.pcengines.ch) - Development of the apu system board, flashing image, ...
- Daniel Kern <<info@kromlech.ch>> - Testing and documentation fixes


## Instructions

### Setup

#### Install base system

At the moment two linux distributions are supported:

[Debian 7 - x86_64](https://github.com/byteworks-ch/pcengines-apu-bios/wiki/Setup-Debian-7-(Wheezy))

[Ubuntu 14.04 LTS - x86_64](https://github.com/byteworks-ch/pcengines-apu-bios/wiki/Setup-Ubuntu-14.04-LTS-(Trusty-Tahr))


Most likely it will work on any recent 64 bit linux distribution. Feel free to try. I'm happy to update the instructions.


### Build

#### Get bios source code

`git clone https://github.com/byteworks-ch/pcengines-apu-bios.git`

#### Build bios

```	
cd pcengines-apu-bios
./build.sh
```

If the build did complete without an error, you'll find the bios file here: build/coreboot.rom

#### Build bios update image

`./image.sh`

If the image build did complete without an error, you'll find an bios update image here: build/image.img


### Flash

To update the bios you need to write the built bios update image to a USB stick or a similar flash media (SD/CF/...). It seems that this works only with medias which have a capacity < 2 GB.

The following instructions have been based on the [installation guide of pfSense](https://doc.pfsense.org/index.php/Installing_pfSense).

#### Windows

When using a windows machine the easiest way to write your media is with [physdiskwrite](http://m0n0.ch/wall/physdiskwrite.php).

Place physdiskwrite and the update image in the same folder. Attach the media you want to write the image on  (i.e. CF-Reader and CF-Card). Then issue the following command:

`physdiskwrite image.img`

Physdiskwrite will show you a numbered list of suitable devices (USB-stick, CF-cards in USB-readers, harddisks,...). Enter the number of the desired media.

#### Linux

For Linux machines, use the built in dd command from a terminal shell. 

`dd if=image.img of=<device>`

You will need to amend if= (input file) when your downloaded file name varies. Destination of= (output file) is where the image is written to. It should be a block device. Check the output of the `dmesg` command to figure out the device name after you have connected the media:

```
[16572.076340] usb 1-1: new high-speed USB device number 2 using ehci_hcd
[16572.262361] usb 1-1: New USB device found, idVendor=0ea0, idProduct=2168
[16572.262364] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[16572.262366] usb 1-1: Product: Flash Disk      
[16572.262367] usb 1-1: Manufacturer: USB     
[16572.262368] usb 1-1: SerialNumber: 41-41-511-1001
[16572.334798] Initializing USB Mass Storage driver...
[16572.336276] scsi3 : usb-storage 1-1:1.0
[16572.338026] usbcore: registered new interface driver usb-storage
[16572.338028] USB Mass Storage support registered.
[16573.338497] scsi 3:0:0:0: Direct-Access     USB      Flash Disk       2.00 PQ: 0 ANSI: 2
[16573.343529] sd 3:0:0:0: Attached scsi generic sg2 type 0
[16573.346438] sd 3:0:0:0: [sdb] 256000 512-byte logical blocks: (131 MB/125 MiB)
[16573.348529] sd 3:0:0:0: [sdb] Write Protect is off
[16573.348532] sd 3:0:0:0: [sdb] Mode Sense: 03 00 00 00
[16573.350764] sd 3:0:0:0: [sdb] No Caching mode page found
[16573.352723] sd 3:0:0:0: [sdb] Assuming drive cache: write through
[16573.366294] sd 3:0:0:0: [sdb] No Caching mode page found
[16573.366701] sd 3:0:0:0: [sdb] Assuming drive cache: write through
[16573.371988]  sdb:
[16573.385391] sd 3:0:0:0: [sdb] No Caching mode page found
[16573.385708] sd 3:0:0:0: [sdb] Assuming drive cache: write through
[16573.386059] sd 3:0:0:0: [sdb] Attached SCSI removable disk
```

#### Mac OS X

For Mac OS X machines, use the built in dd command from a terminal shell. 

`dd if=image.img of=<device>`

You will need to amend if= (input file) when your downloaded file name varies. Destination of= (output file) is where the image is written to. It should be a block device. Check the output of the `diskutil list` command to figure out the device name after you have connected the media:

```
/dev/disk0
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:      GUID_partition_scheme                        *251.0 GB   disk0
   1:                        EFI EFI                     209.7 MB   disk0s1
   2:          Apple_CoreStorage                         250.1 GB   disk0s2
   3:                 Apple_Boot Recovery HD             650.0 MB   disk0s3
/dev/disk1
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:                  Apple_HFS Macintosh HD           *249.8 GB   disk1
/dev/disk2
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:                            PCENGINES-APU-ROCKS    *131.1 MB   disk2
```

In our case we did plug in an 128 MB USB Stick, which is listed as /dev/disk2.
For writing the image we're going to use the raw device, which is the device name with
an 'r' prefixed: /dev/rdisk2.

Make sure the plugged in media is not mounted using the `mount` command:

```
/dev/disk1 on / (hfs, local, journaled)
devfs on /dev (devfs, local, nobrowse)
map -hosts on /net (autofs, nosuid, automounted, nobrowse)
map auto_home on /home (autofs, automounted, nobrowse)
/dev/disk2 on /Volumes/PCENGINES-APU-ROCKS (msdos, local, nodev, nosuid, noowners)
```

If you see the device listed in the output unmount it:

`diskutil unmount /dev/disk2`


### Recovery

In the case that something went wrong with the bios update, the bios can be restored using the [flash recovery board lpc1aapu](http://www.pcengines.ch/lpc1aapu.htm) by PC Engines.
I recommend to get one of these before starting to play.

