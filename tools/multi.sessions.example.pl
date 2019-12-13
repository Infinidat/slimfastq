# -*- cperl -*-

#  This program was written by Josef Ezra  <jezra@infinidat.com>, <jezra@cpan.org>                                     #
#  Copyright (c) 2019, Infinidat                                                                                       #
#  All rights reserved.                                                                                                #
#                                                                                                                      #
#  Redistribution and use in source and binary forms, with or without modification, are permitted provided that        #
#  the following conditions are met:                                                                                   #
#                                                                                                                      #
#  Redistributions of source code must retain the above copyright notice, this list of conditions and the following    #
#  disclaimer.                                                                                                         #
#  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following #
#  disclaimer in the documentation and/or other materials provided with the distribution.                              #
#  Neither the name of the Infinidat nor the names of its contributors may be used to endorse or promote products      #
#  derived from this software without specific prior written permission.                                               #

#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  #
#  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE   #
#  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  #
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR     #
#  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,   #
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE    #
#  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                            #

eval 'exec /usr/bin/perl $0 $*'
    if 0 ;

use 5.6.0 ;
use warnings ;
use strict ;
use integer ;
use bytes ;

use threads ;
use threads::shared ;
use File::Basename;

my $num_t = shift or die <<USAGE;
Usage:
  $0 <max-N> [src-dir] [tgt-dir] [slimfastq-cmd]
While:
 max-N  : (required) how many files to compress concurrently
 src-dir: (optional) directory to find '*.fastq' files (default is './')
 tgt-dir: (optional) directory to save '*.sfq' compressed files (default is src-dir/SFQ)
 slimfastq-cmd: valid path/name to the slimfastq (default is './slimfastq')

This will call ./slimfastq ( this script) to compress all *.fastq *.fq files in src-dir
into *.sfq in tgt-dir. tgt-dir may be created (if only one directory level is missing).

Please feel free to change the script to your needs.


USAGE

my $sdir = shift || '.';
my $tdir = shift || './SFQ';
my $sfq  = shift || "./slimfastq"; # change to your needs

die "$sdir: does not exists\n" if not -d $sdir;
die "$tdir: can't create\n"    if not -d $tdir and not mkdir $tdir;

my @files : shared ;
@files = glob "$sdir/*.fastq $sdir/*.fq";

sub getfile() {
    lock  @files;
    shift @files;
}

sub just_doit {
    while (my $u = getfile()) {
        my $b = basename $u, '.fastq', '.fq' ;
        my $f = "$tdir/$b.sfq";
        system "$sfq -q -u $u -f $f";
    }
}

my @threads;
sub thread_push(){
    push @threads, threads->create( \&just_doit ) ;
}
sub thread_pop(){
    my $t = shift @threads;
    $t->join;
}

while (@files) {
    thread_push;
    thread_pop if @threads > $num_t;
}   thread_pop while @threads;


