DIR=dist
MAN=julius.1 julius.1.ja jconf.5
PDF=Man_julius.pdf Man_jconf.pdf

FOP=/cygdrive/c/fop/fop.bat
FOPJA=/cygdrive/c/fop/fop-ja.bat

all: $(MAN) Juliusbook-ja/index.html

pdf: $(PDF)

julius.1: Man_julius.xml opt_*.xml
	xmlto man Man_julius.xml

julius.1.ja: Man_julius_ja.xml opt_*.xml
	- mkdir -p tmp
	xmlto -o tmp man Man_julius_ja.xml
	nkf -e tmp/julius.1 > julius.1.ja

jconf.5: Man_jconf.xml opt_*.xml
	xmlto man Man_jconf.xml

Juliusbook-ja/index.html: Juliusbook-ja.xml license.xml config.xsl docbook.css desc_*.xml
	- mkdir -p Juliusbook-ja
	xmlto -o Juliusbook-ja xhtml -m config.xsl Juliusbook-ja.xml
	cp -p docbook.css Juliusbook-ja

Man_julius.pdf: Man_julius.xml opt_*.xml
	xmlto fo Man_julius.xml
	$(FOP) Man_julius.fo Man_julius.pdf

Man_jconf.pdf: Man_jconf.xml opt_*.xml
	xmlto fo Man_jconf.xml
	$(FOP) Man_jconf.fo Man_jconf.pdf

Man_julius_ja.pdf: Man_julius_ja.xml opt_*.xml
	xmlto fo Man_julius_ja.xml
	$(FOPJA) Man_julius_ja.fo Man_julius_ja.pdf

dist:
	- mkdir -p $(DIR)
	cp -p $(MAN) $(DIR)
	cp -p $(PDF) $(DIR)
	cp -pr Juliusbook-ja $(DIR)

clean:
	rm -rf tmp
	rm -f *~ *.bak
	rm -f *.fo

distclean:
	rm -rf tmp
	rm -f *~ *.bak
	rm -f *.fo
	rm -f $(MAN)
	rm -rf Juliusbook-ja
	rm -f $(DIR)
	rm -f *.html
