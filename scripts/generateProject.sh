#!/usr/bin/env bash

SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT_DIR=$PATHNAME/..
BASE_BIN_DIR=$ROOT_DIR/build


generateVersion() {
  echo "generating $1"
  BIN_DIR=${BASE_BIN_DIR}/$1
  if [ -d $BIN_DIR ]; then
    cd $BIN_DIR
  else
    mkdir -p $BIN_DIR
    cd $BIN_DIR
  fi
  if [[ $2 -ne 0 ]]; then
    cmake . ../../src -G "Ninja" "-DWA_BUILD_TYPE=$1" -Wno-dev
  else
    cmake ../../src "-DWA_BUILD_TYPE=$1" -Wno-dev
  fi
  cd $PATHNAME
}


#generateVersion release $1

generateVersion debug $1
