# -*- cperl -*-

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
  $0 <max-N> [src-dir] [tgt-dir]
While:
 max-N  : (required) how many files to compress concurrently
 src-dir: (optional) directory to find '*.fastq' files (default is './')
 tgt-dir: (optional) directory to save '*.sfq' compressed files (default is src-dir/SFQ)

This will call slimfastq (must be in path, or update this scripot) to compress 

USAGE

my $sdir = shift || '.';
my $tdir = shift || './SFQ';
my $sfq  = "./slimfastq";         # change to your needs

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


