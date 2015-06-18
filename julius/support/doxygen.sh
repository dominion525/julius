#!/bin/sh
#
# generate source document
#
# usage: doxygen.sh directory
#
set outdir=$1
set cwd=`pwd`/
set version=`cat julius/configure.in | grep "VERSION=" | sed -e s/VERSION=//g`
