// Bootmapper struct definitions
//
// Copyright (C) 2014  Sage Electronic Engineering, LLC.
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#ifndef __BOOTMAPPER_H
#define __BOOTMAPPER_H

#define BOOTMAPPER_STRING_LENGTH 40

struct map_struct {
    u8 map_char;         // ASCII char that replaces the numerical value
    char map_key[2];     // Two ASCII values representing the scancode
    u8 map_num;          // This gets set runtime with the replaced number
    char map_string[BOOTMAPPER_STRING_LENGTH]; // The boot device string that matches what seabios produces
};

#endif // bootmapper.h
