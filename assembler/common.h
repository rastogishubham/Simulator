#ifndef COMMON_H
#define COMMON_H

typedef  unsigned char bool;
typedef unsigned char u_int8;
typedef unsigned short u_int16;
typedef unsigned int u_int32;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

#define SUCCESS     1
#define FAILURE     0
#define ERROR       -1

#define TRUE        1
#define FALSE       0

#define WORD_SIZE   4

#define isOdd(x)    ( (x) & 0x1 )
#define min(a,b)    ( (a) < (b) ? (a) : (b) )
#define max(a,b)    ( (a) > (b) ? (a) : (b) )

extern int debug;
#endif /* COMMON_H */
