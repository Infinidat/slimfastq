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

// Manifest:
// update by jezra Sep, 2019

//  This project was written in a "simplified" c++ to support any old compiler out there. The structure is:
//  During save (encoding)
//    UsrSave  - handling fastq files, detecting records and providing them to:
//      RecSave - saving the header lines (break to word tockens and save diffs)
//      GenSave - saves the genomic data (optimized for 4 values)
//      QltSave - saves the quality measure (optimized for 64 values)
//  During load (decode)
//    UsrLoad - reads record fragments from RecLoad, GenLoad, QltLoad and prints in order

// under the hood, use WORM file system to implement multiple range coders streams. Note that the
// file's first block is metadata + info.


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "common.hpp"
#include "config.hpp"
#include "usrs.hpp"

Config conf;
int main(int argc, char** argv) {

    conf.init(argc, argv);
    int ret =
        conf.encode ?
        UsrSave () . encode() :
        UsrLoad () . decode() ;

    conf.finit();
    return ret;
}
