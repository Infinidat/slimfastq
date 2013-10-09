//
// Written by Josef Ezra <jezra@infinidat.com> during R&D phase.
// Please do not use without author's explicit permission.
// 

#ifndef FQ_COMMON_H
#define FQ_COMMON_H

#define PACKED __attribute__((__packed__))

#define LIKELY(X) __builtin_expect((X),1)
#define RARELY(X) __builtin_expect((X),0)
// TEST: Would profiler optimization work better with no expectations
// #define LIKELY(X) (X)
// #define RARELY(X) (X)
#define likely_if(x) if (LIKELY(x))
#define rarely_if(x) if (RARELY(x))

typedef unsigned char      UCHAR ;
typedef UCHAR              UINT8 ;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;

#define BZERO(X) bzero(&X, sizeof(X))
#define DELETE(X) do {if (X) delete X; X = NULL; } while (0)

#ifdef DO_DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif 

#endif // FQ_COMMON_H
