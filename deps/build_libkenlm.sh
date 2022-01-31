#!/bin/bash

set -e

build_shared=true

cd kenlm
rm -f {lm,util}/*.o 2>/dev/null

CXX=${CXX:-g++}

CXXFLAGS+=" -I. -O3 -DNDEBUG -DKENLM_MAX_ORDER=6 "
if [ "$build_shared" == true ]; then
  CXXFLAGS+=" -fPIC "
fi

echo 'Compiling with '$CXX $CXXFLAGS

#Grab all cc files in these directories except those ending in test.cc or main.cc
objects=""
for i in util/double-conversion/*.cc util/*.cc lm/*.cc $ADDED_PATHS; do
  if [ "${i%test.cc}" == "$i" ] && [ "${i%main.cc}" == "$i" ]; then
    $CXX $CXXFLAGS -c $i -o ${i%.cc}.o
    objects="$objects ${i%.cc}.o"
  fi
done

mkdir -p bin
if [ "$(uname)" != Darwin ]; then
  CXXFLAGS="$CXXFLAGS -lrt"
fi
$CXX lm/build_binary_main.cc $objects -o bin/build_binary $CXXFLAGS $LDFLAGS
$CXX lm/query_main.cc $objects -o bin/query $CXXFLAGS $LDFLAGS

if [ "$build_shared" == true ]; then
    $CXX $objects -shared -o ../libkenlm.so
else
    ar -crv ../libkenlm.a $objects
fi

cd -

