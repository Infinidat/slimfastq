/***********************************************************************************************************************/
/* This program was written by Josef Ezra  <jezra@infinidat.com>                                                       */
/* Copyright (c) 2013, Infinidat                                                                                       */
/* All rights reserved.                                                                                                */
/*                                                                                                                     */
/* Redistribution and use in source and binary forms, with or without modification, are permitted provided that        */
/* the following conditions are met:                                                                                   */
/*                                                                                                                     */
/* Redistributions of source code must retain the above copyright notice, this list of conditions and the following    */
/* disclaimer.                                                                                                         */
/* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following */
/* disclaimer in the documentation and/or other materials provided with the distribution.                              */
/* Neither the name of the Infinidat nor the names of its contributors may be used to endorse or promote products      */
/* derived from this software without specific prior written permission.                                               */
/*                                                                                                                     */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE   */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,   */
/* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE    */
/* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                            */
/***********************************************************************************************************************/




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

#define IS_CLR(exmap, offset) (0 == (exmap&(1ULL<<offset)))
#define IS_SET(exmap, offset) !IS_CLR(exmap, offset)
#define DO_SET(exmap, offset) (exmap |=  (1ULL<<offset))
#define DO_CLR(exmap, offset) (exmap &= ~(1ULL<<offset))
#define BCOUNT(exmap)       __builtin_popcount(exmap)
#define BFIRST(exmap)       __builtin_ffsl(exmap)

#ifdef DO_DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif 

#ifdef __SSE__
#   include <xmmintrin.h>
#   define PREFETCH(X) _mm_prefetch((const char *)(X), _MM_HINT_T0)
// This prefetch saves 8 seconds (on 5.4G fastq), but seems to use slightly
// more cpu (or is it just doing the same work quota at lesser time?)
#else
#   define PREFETCH(X)
#endif

#endif // FQ_COMMON_H
