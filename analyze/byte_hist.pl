# -*- cperl -*-

eval 'exec /home/jezra/Perl/bin/perl $0 $*'
    if 0 ;

use 5.6.0 ;
use warnings ;
use strict ;
use integer ;
use bytes ;

my $f = shift or die "usage: $0 <data-file>" ;
my $n = shift || 1000_000;
my %a;
my $count = 0;

open F, $f or die "can't open $f: $!\n" ;
binmode F;
while (read F, my $buf, 10000) {
    for (split //, $buf) {
        $a{ord $_} ++ ;
        exit if $n < ++ $count;
    }
}

END {
    printf "%02x: $a{$_}\n", $_ for sort {$a{$a} <=> $a{$b}} keys %a ;
    printf "count=$count numc=%d\n", scalar keys %a;
  }


