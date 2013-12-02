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

#ifndef FQ_CONFIG_H
#define FQ_CONFIG_H

#include <stdio.h>
#include "filer.hpp"

class Config;

// NOTE: soon there would be new machines, providing longer lines. 
#define MAX_REC_LEN  1000
#define MAX_ID_LLEN  250
#define MAX_GN_LLEN  250
#define MIN_GN_LLEN   50

void croak(const char* msg               ) __attribute__ ((noreturn, cold));
void croak(const char* fmt, long long num) __attribute__ ((noreturn, cold));

class Config {
    // Singleton - croaks if already exists
public:
    Config();
    ~Config();
    void init (int argc, char **argv, int ver);
    void finit();
    FILE * file_usr() const { return reinterpret_cast<FILE*>(f_usr);}
    int   version, level;
    bool  encode, profiling;

    void load_info() const;
    void set_info(const char* key, const char* val) const;
    void set_info(const char* key, long long num) const;
    const char* get_info(const char* key) const;
    bool        has_info(const char* key) const;
    bool        get_bool(const char* key) const;
    long long   get_long(const char* key, long long val=0) const;

private:
    void usage() const;
    void statistics_dump() const;

    FILE  *f_usr;
    FilerSave* m_info_filer;
};

extern Config conf;
extern unsigned long long g_record_count;
extern unsigned long long g_genofs_count;

#endif  // FQ_CONFIG_H
