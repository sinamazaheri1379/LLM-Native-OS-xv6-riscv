#ifndef TYPES_H
#define TYPES_H

#ifndef __ASSEMBLER__

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef signed char schar;
typedef signed short sshort;
typedef signed int sint;

typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;
typedef signed long sint64;

typedef float float_t;
typedef double double_t;
typedef long double ldouble_t;

typedef uint64 pde_t;

#endif // __ASSEMBLER__

#endif // TYPES_H
