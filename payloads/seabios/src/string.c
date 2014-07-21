// String manipulation functions.
//
// Copyright (C) 2008-2013  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2014  Sage Electronic Engineering, LLC
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "stacks.h" // yield
#include "string.h" // memcpy
#include "farptr.h" // SET_SEG


/****************************************************************
 * String ops
 ****************************************************************/

// Sum the bytes in the specified area.
u8
checksum_far(u16 buf_seg, void *buf_far, u32 len)
{
    SET_SEG(ES, buf_seg);
    u32 i;
    u8 sum = 0;
    for (i=0; i<len; i++)
        sum += GET_VAR(ES, ((u8*)buf_far)[i]);
    return sum;
}

u8
checksum(void *buf, u32 len)
{
    return checksum_far(GET_SEG(SS), buf, len);
}

size_t
strlen(const char *s)
{
    if (__builtin_constant_p(s))
        return __builtin_strlen(s);
    const char *p = s;
    while (*p)
        p++;
    return p-s;
}

// Compare two areas of memory.
int
memcmp(const void *s1, const void *s2, size_t n)
{
    while (n) {
        if (*(u8*)s1 != *(u8*)s2)
            return *(u8*)s1 < *(u8*)s2 ? -1 : 1;
        s1++;
        s2++;
        n--;
    }
    return 0;
}

// Compare two strings.
int
strcmp(const char *s1, const char *s2)
{
    for (;;) {
        if (*s1 != *s2)
            return *s1 < *s2 ? -1 : 1;
        if (! *s1)
            return 0;
        s1++;
        s2++;
    }
}

inline void
memset_far(u16 d_seg, void *d_far, u8 c, size_t len)
{
    SET_SEG(ES, d_seg);
    asm volatile(
        "rep stosb %%es:(%%di)"
        : "+c"(len), "+D"(d_far)
        : "a"(c), "m" (__segment_ES)
        : "cc", "memory");
}

inline void
memset16_far(u16 d_seg, void *d_far, u16 c, size_t len)
{
    len /= 2;
    SET_SEG(ES, d_seg);
    asm volatile(
        "rep stosw %%es:(%%di)"
        : "+c"(len), "+D"(d_far)
        : "a"(c), "m" (__segment_ES)
        : "cc", "memory");
}

void *
memset(void *s, int c, size_t n)
{
    while (n)
        ((char *)s)[--n] = c;
    return s;
}

void memset_fl(void *ptr, u8 val, size_t size)
{
    if (MODESEGMENT)
        memset_far(FLATPTR_TO_SEG(ptr), (void*)(FLATPTR_TO_OFFSET(ptr)),
                   val, size);
    else
        memset(ptr, val, size);
}

inline void
memcpy_far(u16 d_seg, void *d_far, u16 s_seg, const void *s_far, size_t len)
{
    SET_SEG(ES, d_seg);
    u16 bkup_ds;
    asm volatile(
        "movw %%ds, %w0\n"
        "movw %w4, %%ds\n"
        "rep movsb (%%si),%%es:(%%di)\n"
        "movw %w0, %%ds"
        : "=&r"(bkup_ds), "+c"(len), "+S"(s_far), "+D"(d_far)
        : "r"(s_seg), "m" (__segment_ES)
        : "cc", "memory");
}

inline void
memcpy_fl(void *d_fl, const void *s_fl, size_t len)
{
    if (MODESEGMENT)
        memcpy_far(FLATPTR_TO_SEG(d_fl), (void*)FLATPTR_TO_OFFSET(d_fl)
                   , FLATPTR_TO_SEG(s_fl), (void*)FLATPTR_TO_OFFSET(s_fl)
                   , len);
    else
        memcpy(d_fl, s_fl, len);
}

void *
#undef memcpy
memcpy(void *d1, const void *s1, size_t len)
#if MODESEGMENT == 0
#define memcpy __builtin_memcpy
#endif
{
    SET_SEG(ES, GET_SEG(SS));
    void *d = d1;
    if (((u32)d1 | (u32)s1 | len) & 3) {
        // non-aligned memcpy
        asm volatile(
            "rep movsb (%%esi),%%es:(%%edi)"
            : "+c"(len), "+S"(s1), "+D"(d)
            : "m" (__segment_ES) : "cc", "memory");
        return d1;
    }
    // Common case - use 4-byte copy
    len /= 4;
    asm volatile(
        "rep movsl (%%esi),%%es:(%%edi)"
        : "+c"(len), "+S"(s1), "+D"(d)
        : "m" (__segment_ES) : "cc", "memory");
    return d1;
}

// Copy to/from memory mapped IO.  IO mem is very slow, so yield
// periodically.
void
iomemcpy(void *d, const void *s, u32 len)
{
    ASSERT32FLAT();
    yield();
    while (len > 3) {
        u32 copylen = len;
        if (copylen > 2048)
            copylen = 2048;
        copylen /= 4;
        len -= copylen * 4;
        asm volatile(
            "rep movsl (%%esi),%%es:(%%edi)"
            : "+c"(copylen), "+S"(s), "+D"(d)
            : : "cc", "memory");
        yield();
    }
    if (len)
        // Copy any remaining bytes.
        memcpy(d, s, len);
}

void *
memmove(void *d, const void *s, size_t len)
{
    if (s >= d)
        return memcpy(d, s, len);

    d += len-1;
    s += len-1;
    while (len--) {
        *(char*)d = *(char*)s;
        d--;
        s--;
    }

    return d;
}

// Copy a string - truncating it if necessary.
char *
strtcpy(char *dest, const char *src, size_t len)
{
    char *d = dest;
    while (--len && *src != '\0')
        *d++ = *src++;
    *d = '\0';
    return dest;
}

// locate first occurance of character c in the string s
char *
strchr(const char *s, int c)
{
    for (; *s; s++)
        if (*s == c)
            return (char*)s;
    return NULL;
}

// Remove any trailing blank characters (spaces, new lines, carriage returns)
char *
nullTrailingSpace(char *buf)
{
    int len = strlen(buf);
    char *end = &buf[len-1];
    while (end >= buf && *end <= ' ')
        *(end--) = '\0';
    while (*buf && *buf <= ' ')
        buf++;
    return buf;
}

static inline int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

static inline int tolower(int c)
{
    if (isupper(c))
        c -= 'A'-'a';
    return c;
}

int strnicmp(const char *s1, const char *s2, int maxlen)
{
    int i;

    for (i = 0; i < maxlen; i++) {
        if (tolower(s1[i]) != tolower(s2[i]))
            return (tolower(s1[i]) - tolower(s2[i]));
    }

    return 0;
}

int isspace(int c)
{
    return (c == ' ' || (c >= '\t' && c <= '\r'));
}

// Check that a character is in the valid range for the given base

static int _valid(char ch, int base)
{
    char end = (base > 9) ? '9' : '0' + (base - 1);

    // all bases will be some subset of the 0-9 range

    if (ch >= '0' && ch <= end)
        return 1;

    // Bases > 11 will also have to match in the a-z range

    if (base > 11) {
        if (tolower(ch) >= 'a' &&
            tolower(ch) <= 'a' + (base - 11))
            return 1;
    }

    return 0;
}

// Return the "value" of the character in the given base

static int _offset(char ch, int base)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else
        return 10 + tolower(ch) - 'a';
}

// Convert the initial portion of a string into a signed int
// @param ptr A pointer to the string to convert
// @param endptr A pointer to the unconverted part of the string
// @param base The base of the number to convert, or 0 for auto
// @return A signed integer representation of the string

long int strtol(const char *ptr, char **endptr, int base)
{
    int ret = 0;
    int negative = 1;

    if (endptr != NULL)
        *endptr = (char *) ptr;

    // Purge whitespace

    for( ; *ptr && isspace(*ptr); ptr++);

    if (ptr[0] == '-') {
        negative = -1;
        ptr++;
    }

    if (!*ptr)
        return 0;

    // Determine the base

    if (base == 0) {
        if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
            base = 16;
        else if (ptr[0] == '0') {
            base = 8;
            ptr++;
        }
        else
            base = 10;
    }

    // Base 16 allows the 0x on front - so skip over it

    if (base == 16) {
        if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
            ptr += 2;
    }

    // If the first character isn't valid, then don't bother

    if (!*ptr || !_valid(*ptr, base))
        return 0;

    for( ; *ptr && _valid(*ptr, base); ptr++)
        ret = (ret * base) + _offset(*ptr, base);

    if (endptr != NULL)
        *endptr = (char *) ptr;

    return ret * negative;
}

/**
 * Find a substring within a string.
 *
 * @param h The haystack string.
 * @param n The needle string (substring).
 * @return A pointer to the first occurence of the substring in
 * the string, or NULL if the substring was not encountered within the string.
 */
char *strstr(const char *h, const char *n)
{
	int hn = strlen(h);
	int nn = strlen(n);
	int i;

	for (i = 0; i <= hn - nn; i++)
		if (!memcmp(&h[i], n, nn))
			return (char *)&h[i];

	return NULL;
}
