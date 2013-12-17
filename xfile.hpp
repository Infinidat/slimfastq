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


#ifndef FQ_XFILE_H
#define FQ_XFILE_H


#include <stdio.h>
#include "common.hpp"
#include "config.hpp"

#include "filer.hpp"
#include "power_ranger.hpp"     // exception list

class XFileBase {

protected:

    XFileBase(const char* filename);

    const char* m_filename;
    bool m_valid;
    RCoder rcoder;
    PowerRangerU ranger;
    PowerRanger  ranger_str;
};


class XFileSave : private XFileBase {
    FilerSave* filer;
    void init();
    
public:
    XFileSave(const char* filename);
    ~XFileSave();
    bool put(UINT64 gap);
    void put_chr(UCHAR chr);
    void put_str(const UCHAR* p, size_t len);
    size_t tell() const ;
};

class XFileLoad : private XFileBase {
    FilerLoad* filer;
    void init();
public:
    XFileLoad(const char* filename);
    ~XFileLoad();
    UINT64 get();
    UCHAR  get_chr();
    UCHAR* get_str(UCHAR* p);
    bool is_valid() const { return m_valid; }
};


#endif
