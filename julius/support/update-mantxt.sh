#!/bin/sh
#
# update all 00readme.txt and 00readme-ja.txt of the tools from each manuals
#
# should be invoked at parent directory.
#
# If conversion fails, see makeman.sh in this directory.
#

tools_man="\
./adinrec/adinrec.man \
./adintool/adintool.man \
./jcontrol/jcontrol.man \
./mkbingram/mkbingram.man \
./mkbinhmm/mkbinhmm.man \
./mkgshmm/mkgshmm.man \
./mkss/mkss.man \
"

##############################

for m in $tools_man; do
    echo $m
    ./support/makeman.sh $m
    mv $m.txt `dirname $m`/00readme.txt
    mv $m.txt.ja `dirname $m`/00readme-ja.txt
done

echo julius/julius.man
./support/makeman.sh julius/julius.man
mv julius/julius.man.txt julius/00readme-julius.txt
mv julius/julius.man.txt.ja julius/00readme-julius-ja.txt

echo julius/julian.man
./support/makeman.sh julius/julian.man
mv julius/julian.man.txt julius/00readme-julian.txt
mv julius/julian.man.txt.ja julius/00readme-julian-ja.txt

echo Finished.
