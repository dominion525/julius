#!/bin/sh
#
# change version
# (should be executed in the parent directory)
#
if [[ $# == 0 ]]; then
	echo usage: $0 newversionname
	exit
fi
newver=$1

for i in ./julius/configure.in ./libsent/configure.in ./support/build-all.sh; do
	echo $i
	sed -e "s/^VERSION=[^ ]*/VERSION=$newver/" < $i > tmp
	diff $i tmp
	mv tmp ${i}
done

echo 
echo updating configure scripts...
cd ./julius; autoconf
cd ../libsent; autoconf
