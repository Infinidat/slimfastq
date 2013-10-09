# -*- cperl -*-

eval 'exec /usr/bin/perl $0 $*'
    if 0 ;

use 5.6.0 ;
use warnings ;
use strict ;
use integer ;
use bytes ;

my $a = shift ;
my $b = shift ;

die <<HOWTO unless $a and $b ;
 usage:
   $0 <file-a> <file-b>
HOWTO

open A, $a or die "can't read '$a': $!\n" ;
open B, $b or die "can't read '$b': $!\n" ;

sub carp($) {
    use Term::ANSIColor qw(:constants);
    $Term::ANSIColor::AUTORESET = 1;
    my $errstr = RED " *********************************";
    die "$errstr\n@_\n$errstr\n";
}
my $l = 0;
while (1) {
    my $A = <A> ;
    my $B = <B> ;
    $l++;
    last if not $A and not $B ;
    carp "$a: early eof line $l" if not $A ;
    carp "$b: early eof line $l" if not $B ;
    carp "mismatch at line $l:\n$a\n$b\n$A$B" if $A ne $B ;
}

print STDERR "$a, $b:\n = Full Match =  \n";

exit(0);
