/*
 * This file is part of the libpayload project.
 *
 * It has originally been taken from the HelenOS project
 * (http://www.helenos.eu), and slightly modified for our purposes.
 *
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2014 Sage Electronic Engineering, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <libpayload.h>
#include <x86/arch/types.h>

static void *default_memset(void *s, int c, size_t n)
{
	char *os = s;

	while (n--)
		*(os++) = c;

	return s;
}

void *memset(void *s, int c, size_t n)
	__attribute__((weak, alias("default_memset")));

static void *default_memcpy(void *dst, const void *src, size_t n)
{
	int i;
	void *ret = dst;

	for(i = 0; i < n % sizeof(unsigned long); i++)
		((unsigned char *) dst)[i] = ((unsigned char *) src)[i];

	n -= i;
	src += i;
	dst += i;

	for(i = 0; i < n / sizeof(unsigned long); i++)
		((unsigned long *) dst)[i] = ((unsigned long *) src)[i];

	return ret;
}

void *memcpy(void *dst, const void *src, size_t n)
	__attribute__((weak, alias("default_memcpy")));

static void *default_memmove(void *dst, const void *src, size_t n)
{
	int i;
	unsigned long offs;

	if (src > dst)
		return memcpy(dst, src, n);

	offs = n - (n % sizeof(unsigned long));

	for (i = (n % sizeof(unsigned long)) - 1; i >= 0; i--)
		((unsigned char *)dst)[i + offs] =
			((unsigned char *)src)[i + offs];

	for (i = n / sizeof(unsigned long) - 1; i >= 0; i--)
		((unsigned long *)dst)[i] = ((unsigned long *)src)[i];

	return dst;
}

void *memmove(void *dst, const void *src, size_t n)
	__attribute__((weak, alias("default_memmove")));

/**
 * Compare two memory areas.
 *
 * @param s1 Pointer to the first area to compare.
 * @param s2 Pointer to the second area to compare.
 * @param len Size of the first area in bytes (both must have the same length).
 * @return If len is 0, return zero. If the areas match, return zero.
 *         Otherwise return non-zero.
 */

static int default_memcmp(const void *s1, const void *s2, size_t len)
{
	for (; len && *(char *)s1++ == *(char *)s2++; len--) ;
	return len;
}

int memcmp(const void *s1, const void *s2, size_t len)
	__attribute__((weak, alias("default_memcmp")));

u8 memory_read_byte( u32 address )
{
	volatile u8 *ptr = (u8 *)(address);
	return(*ptr);
}

u16 memory_read_word( u32 address )
{
	volatile u16 *ptr = (u16 *)(address);
	return(*ptr);
}

u32 memory_read_dword( u32 address )
{
	volatile u32 *ptr = (u32 *)(address);
	return(*ptr);
}

void memory_write_byte( u32 address, u8 data )
{
	volatile u8 *ptr = (u8 *)(address);
	*ptr = data;
}

void memory_write_word( u32 address, u16 data )
{
	volatile u16 *ptr = (u16 *)(address);
	*ptr = data;
}

void memory_write_dword( u32 address, u32 data )
{
	volatile u32 *ptr = (u32 *)(address);
	*ptr = data;
}

