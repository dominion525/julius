#!/usr/bin/perl
#
# post process for manual
# 
# $Id: filt_man.pl,v 1.1.1.1 2007/01/10 08:01:57 kudravka_ Exp $

while(<>) {
    chomp;
    s/\</\&lt\;/g;
    s/\>/\&gt\;/g;
    # headings -> H3
    if (/^[^\<\s]/) {
	$sec = $_;
	print "<h3>$sec</h3>\n";
    } else {
	print "$_\n";
    }
}
