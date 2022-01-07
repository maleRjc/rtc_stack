#!/usr/bin/env bash

SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT_DIR=$PATHNAME/..
BIN_DIR=$ROOT_DIR/build

OBJ_DIR="CMakeFiles"

buildAll() {
  if [ -d $BIN_DIR ]; then
    cd $BIN_DIR
    for d in */ ; do
      if [ "$d" != "libdeps/" ];
      then
        echo "Building $d - $*"
        cd $d
        make -j4 $*
        #ninja
        cd ..
      fi
    done
    cd $ROOT_DIR
  else
    echo "Error, build directory does not exist, run generateProject.sh first"
  fi
}

lib_all_path="build/debug/libwebrtc_agent.a \
	build/debug/3rd/libsdptransform/libsdptransform.a \
	build/debug/3rd/owt/libowt.a \
	build/debug/3rd/erizo/src/erizo/liberizo.a \
	build/libdeps/build/lib/libsrtp2.a \
	build/debug/3rd/myrtc/libmyrtc.a \
	build/debug/3rd/abseil-cpp/libabsl.a \
	build/debug/3rd/libevent/libevent.a \
	build/libdeps/build/lib64/libnice.a \
	build/libdeps/build/lib/libssl.a \
	build/libdeps/build/lib/libcrypto.a"

gen_lib() {
	rm -f ../libwa.a
	mkdir ./build/o -p
	cp $lib_all_path ./build/o
	pushd $BIN_DIR/o >/dev/null
	all_a=`find . -name "*.a"`

	for i in $all_a
	do
	  echo "process $i"
	  ar x $i
	  all_o=`find . -name "*.o"`
	  if [[ -n $all_o ]];
	  then
	  	if [ -f "../libwa.a" ];
	  	then
	  	  echo "ar q ../libwa.a"
	      ar q ../libwa.a $all_o
	    else
	      echo "ar cq ../libwa.a"
	      ar cq ../libwa.a $all_o
	    fi
	    rm -f *.o
	  fi
	done
	
	pushd
}

OS=`./detectOS.sh | awk '{print tolower($0)}'`
echo $OS

if [[ "$OS" =~ .*centos.* ]];then
  source scl_source enable devtoolset-7
fi

#export PATH=$PATH:$(pwd)/../build/libdeps/build/ninja

DWARF_FLAG=
#-gdwarf-2

./build_3rd.sh $DWARF_FLAG

./generateProject.sh $DWARF_FLAG

buildAll $*

