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


#include "xfile.hpp"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

XFileBase::XFileBase(const char* filename
                     ) : m_filename(filename),
                         m_init(false) {}


XFileSave::XFileSave(const char* filename
                     ) : XFileBase(filename),
                         filer(NULL) {}

XFileSave::~XFileSave() {
    if (filer) {
        rcoder.done();
        delete filer;
        filer = NULL;
    }
}
XFileLoad::XFileLoad(const char* filename
                     ) : XFileBase(filename),
                         filer(NULL) {}

XFileLoad::~XFileLoad() {
    if (filer) {
        rcoder.done();
        delete filer;
        filer = NULL;
    }
}

void XFileSave::init () {
    m_init = true;
    filer = new FilerSave(m_filename);
    assert(filer);
    rcoder.init(filer);
}

bool XFileSave::put(UINT64 gap) {
    rarely_if(not m_init) init();
    return ranger.put_u(&rcoder, gap);
}


void XFileLoad::init() {
    m_init = true;
    // FILE* fh = conf.open_r(m_filename, false);
    filer = new FilerLoad(m_filename, &m_valid);
    assert(filer);
    if (m_valid)
        rcoder.init(filer);
    else
        DELETE(filer);
}

UINT64 XFileLoad::get() {
    rarely_if(not m_init) init();
    return m_valid ? ranger.get_u(&rcoder) : 0;
}


void XFileSave::put_str(const UCHAR* p, size_t len) {
    put(len);
    for (UINT32 j = 0; j < len; j++)
        ranger_str.put(&rcoder, p[j]);
}

UCHAR* XFileLoad::get_str(UCHAR* p) {
    size_t len = get();
    for (UINT32 j = 0; j < len; j++)
        p[j] = ranger_str.get(&rcoder);
    return p + len;
}
