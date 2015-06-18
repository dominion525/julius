#!/bin/sh
#
# usage: makeman.sh julius.man
# 
# generate julius.man.txt, julius.man.txt.ja
#
nroff -man $1 | sed -e 's/.//g' | nkf -c > $1.txt
LANG=ja_JP.eucJP nroff -Tnippon -man $1.ja | sed -e 's/.//g' | nkf -s -c > $1.txt.ja
#man -Tps -l $1 > $1.txt.ps
