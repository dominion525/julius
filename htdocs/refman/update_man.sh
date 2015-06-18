#!/bin/sh
#
# update manuals from julius source
#
# make sure you have already updated 00readme-* from the manuals.
#
srcdir=/home/ri/julius/src/julius

cp $srcdir/adintool/00readme.txt adintool.txt.en
cp $srcdir/adintool/00readme-ja.txt adintool.txt.ja
cp $srcdir/adinrec/00readme.txt adinrec.txt.en
cp $srcdir/adinrec/00readme-ja.txt adinrec.txt.ja
cp $srcdir/jcontrol/00readme.txt jcontrol.txt.en
cp $srcdir/jcontrol/00readme-ja.txt jcontrol.txt.ja
cp $srcdir/julius/00readme-julius.txt julius.txt.en
cp $srcdir/julius/00readme-julius-ja.txt julius.txt.ja
cp $srcdir/julius/00readme-julian.txt julian.txt.en
cp $srcdir/julius/00readme-julian-ja.txt julian.txt.ja
cp $srcdir/mkbingram/00readme.txt mkbingram.txt.en
cp $srcdir/mkbingram/00readme-ja.txt mkbingram.txt.ja
cp $srcdir/mkbinhmm/00readme.txt mkbinhmm.txt.en
cp $srcdir/mkbinhmm/00readme-ja.txt mkbinhmm.txt.ja
cp $srcdir/mkgshmm/00readme.txt mkgshmm.txt.en
cp $srcdir/mkgshmm/00readme-ja.txt mkgshmm.txt.ja
cp $srcdir/mkss/00readme.txt mkss.txt.en
cp $srcdir/mkss/00readme-ja.txt mkss.txt.ja

qkc -e -u *.txt.ja
qkc -u *.txt.en

for f in *.txt.ja; do
    ./filt_head.pl `basename $f .txt.ja` 00head.html.ja > tmphead
    ./filt_man.pl $f > tmpbody
    cat tmphead tmpbody 00tail.html > `basename $f .txt.ja`.html.ja
done
for f in *.txt.en; do
    ./filt_head.pl `basename $f .txt.en` 00head.html.en > tmphead
    ./filt_man.pl $f > tmpbody
    cat tmphead tmpbody 00tail.html > `basename $f .txt.en`.html.en
done

rm -f tmphead tmpbody manlist

rm -f *.txt.ja *.txt.en

echo Completed, please modify index.html.ja and index.html.en by hand.
