#!/usr/bin/perl

$name=$ARGV[0];

open(HEAD, $ARGV[1]);
while(<HEAD>) {
    s/\<name\>/$name/g;
    print;
}
close(HEAD);
