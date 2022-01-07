SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT_DIR=$PATHNAME/..
BASE_BIN_DIR=$ROOT_DIR/build
THIRD_PATH=$ROOT_DIR/src/3rd

REBUILD=0

if [ "$*" ]; then
    for input in $@
    do
        if [ $input = "--rebuild" ]; then REBUILD=1; fi
    done
fi


cd $THIRD_PATH/googletest
echo -e "\033[41;36m 1.build gtest \033[0m"

if [ $REBUILD = 1 ]; then
	if [ -e ./build ]; then 
		rm -fr ./build; 
	fi
fi

if [ ! -d ./build ]; then 
	cmake . -Bbuild 
fi

cd ./build
make


cd $THIRD_PATH/libsdptransform
echo -e "\033[41;36m 2.build libsdptransform \033[0m"

if [ $REBUILD = 1 ]; then
	if [ -e ./build ]; then 
		rm -fr ./build; 
	fi
fi

if [ ! -d ./build ]; then 
	cmake . -Bbuild 
fi

cd ./build
make

cd $ROOT_DIR/build/debug
echo -e "\033[41;36m 3.build rtc stack \033[0m"
make

#echo -e "\033[41;36m 4.run unit test \033[0m"
#cd  ./test
#mkdir -p data
#cp $ROOT_DIR/test/data ./data

#./test_rtc

cd $ROOT_DIR

echo -e "\033[41;36m done \033[0m"

