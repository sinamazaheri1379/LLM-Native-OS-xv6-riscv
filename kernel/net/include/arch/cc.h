//
// Created by sina-mazaheri on 11/26/24.
//
#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#include "types.h"

// Basic types
typedef uint8  u8_t;
typedef uint16 u16_t;
typedef uint32 u32_t;
typedef int8   s8_t;
typedef int16  s16_t;
typedef int32  s32_t;

// Byte order is little endian
#define BYTE_ORDER LITTLE_ENDIAN

// Platform logging
#define LWIP_PLATFORM_DIAG(x) printf x
#define LWIP_PLATFORM_ASSERT(x) panic(x)

#endif
