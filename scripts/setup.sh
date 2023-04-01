#!/bin/bash

export PATH=$PATH:$PWD/externals/raspberrypi/tools-master/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin

# Should always stay the same
ProjectRoot=$PWD/..
CrosscompilationRoot=$ProjectRoot/externals/raspberrypi

cd $ProjectRoot

# ---------------------------------------------------------------
# Crosscompilation toolchain
# ---------------------------------------------------------------

echo "Setup crosscompilation toolchain"

cd externals/raspberrypi # {projectRoot}/raspberrypi

if [ ! -d "tools-master" ]; then

	echo "Getting crosscompilation tools"
	wget https://github.com/raspberrypi/tools/archive/master.zip
	unzip -q master.zip
	rm master.zip

else

	echo "Crosscompilation tools already present"

fi

echo "Setting up crosscompilation rootfs"

if [ ! -d "rootfs" ]; then

	echo "Checking if rootfs zip is present"

	if [ -f "raspbian_rootfs.tar.gz" ]; then
	    echo "rootfs zip is present, decompressing"
            tar -xzvf raspbian_rootfs.tar.gz

	else

		echo "rootfs zip not present, please acquire it from your local dungeonmaster"
		echo "Exiting setup due to errors"
		exit 1

	fi

else

	echo "Crosscompilation rootfs already present"

fi

cd .. # {projectRoot}

echo "Crosscompilation toolchain setup done"

# ---------------------------------------------------------------
# Directories
# ---------------------------------------------------------------

echo "Setting up directory structure"

if [ ! -d "externals" ]; then
	mkdir externals
fi

cd externals # {projectRoot}/externals

if [ ! -d "sources" ]; then
	mkdir sources
fi

if [ ! -d "lib" ]; then
	mkdir lib
fi

if [ ! -d "lib64" ]; then
	mkdir lib64
fi

if [ ! -d "include" ]; then
	mkdir include
fi

cd .. # {projectRoot}

if [ ! -d "build" ]; then
	mkdir externals
fi

cd build # {projectRoot}/build

if [ ! -d "x64" ]; then
	mkdir x64
fi

if [ ! -d "arm" ]; then
	mkdir arm
fi

cd .. # {projectRoot}

echo "Directories setup done"

# ---------------------------------------------------------------
# OpenCV
# ---------------------------------------------------------------

echo "Setting up OpenCV"

cd externals  # {projectRoot}/externals

if [ ! -d "sources/opencv-3.4.3" ]; then
	
	echo "Getting OpenCV sources"	
	cd sources  # {projectRoot}/externals/sources
	wget https://github.com/opencv/opencv/archive/3.4.3.zip
	unzip -q 3.4.3.zip
	rm 3.4.3.zip
	cd ..  # {projectRoot}/externals

else

	echo "OpenCV sources already present"

fi

echo "Building opencv for raspberry pi"

cd sources/opencv-3.4.3 # {projectRoot}/externals/sources/opencv-3.4.3
if [ -d "build_raspi" ]; then
	rm -rf build_raspi
fi
mkdir build_raspi
cd build_raspi # {projectRoot}/externals/sources/opencv-3.4.3/build_raspi

cmake -DBUILD_LIST=core,calib3d,imgproc,features2d -DCMAKE_TOOLCHAIN_FILE=${CrosscompilationRoot}/pi.cmake -DWITH_OPENCL=off ..
make -j8
make install
cp -av install/lib/lib* $ProjectRoot/externals/lib
cp -av install/lib/lib* ${CrosscompilationRoot}/rootfs/usr/lib/arm-linux-gnueabihf
cp -av install/include/opencv* $ProjectRoot/externals/include
cp -av install/include/opencv* ${CrosscompilationRoot}/rootfs/usr/include

cd ../../.. # {projectRoot}/externals

echo "Building opencv for fedora host"

cd sources/opencv-3.4.3 # {projectRoot}/externals/sources/opencv-3.4.3
if [ -d "build" ]; then
	rm -rf build
fi
mkdir build
cd build # {projectRoot}/externals/sources/opencv-3.4.3/build

cmake -DBUILD_LIST=core,calib3d,imgproc -DWITH_OPENCL=off -DCMAKE_INSTALL_PREFIX=install ..
make -j8
make install
cp -av install/lib64/lib* $ProjectRoot/externals/lib64

cd ../../.. # {projectRoot}/externals

cd .. # {projectRoot}

echo "OpenCV setup done"

# ---------------------------------------------------------------
# ZMQ
# ---------------------------------------------------------------

echo "Setting up ZMQ"

cd externals  # {projectRoot}/externals

if [ ! -d "sources/libzmq-4.2.5" ]; then
	
	echo "Getting ZMQ sources"	
	cd sources  # {projectRoot}/externals/sources
	wget https://github.com/zeromq/libzmq/archive/v4.2.5.zip
	unzip -q v4.2.5.zip
	rm v4.2.5.zip
	cd ..  # {projectRoot}/externals

else

	echo "ZMQ sources already present"

fi

echo "Building ZMQ for raspberry pi"

cd sources/libzmq-4.2.5 # {projectRoot}/externals/sources/libzmq-4.2.5
if [ -d "build_raspi" ]; then
	rm -rf build_raspi
fi
mkdir build_raspi
cd build_raspi  # {projectRoot}/externals/sources/libzmq-4.2.5/build_raspi
cmake -DCMAKE_TOOLCHAIN_FILE=${CrosscompilationRoot}/pi.cmake ..
make -j8
cp -av lib/* $ProjectRoot/externals/lib
cp -av lib/* ${CrosscompilationRoot}/rootfs/usr/lib/arm-linux-gnueabihf
cp -av ../include/* $ProjectRoot/externals/include
cp -av ../include/* ${CrosscompilationRoot}/rootfs/usr/include

cd ../../.. # {projectRoot}/externals

echo "Building ZMQ for fedora host"

cd sources/libzmq-4.2.5 # {projectRoot}/externals/sources/libzmq-4.2.5
if [ -d "build" ]; then
	rm -rf build
fi
mkdir build
cd build  # {projectRoot}/externals/sources/libzmq-4.2.5/build

cmake ..
make -j8
cp -av lib/* $ProjectRoot/externals/lib64

cd ../../.. # {projectRoot}/externals

cd .. # {projectRoot}

echo "ZMQ setup done"

# ---------------------------------------------------------------
# GLFW
# ---------------------------------------------------------------

echo "Setting up GLFW"

cd externals  # {projectRoot}/externals

if [ ! -d "sources/glfw-3.2.1" ]; then
	
	echo "Getting GLFW sources"	
	cd sources  # {projectRoot}/externals/sources
	wget https://github.com/glfw/glfw/archive/3.2.1.zip
	unzip -q 3.2.1.zip
	rm 3.2.1.zip
	cd ..  # {projectRoot}/externals

else

	echo "GLFW sources already present"

fi

echo "Building GLFW for raspberry pi"

cd sources/glfw-3.2.1 # {projectRoot}/externals/sources/glfw-3.2.1
if [ -d "build_raspi" ]; then
	rm -rf build_raspi
fi
mkdir build_raspi
cd build_raspi  # {projectRoot}/externals/sources/glfw-3.2.1/build_raspi

cmake -DCMAKE_TOOLCHAIN_FILE=${CrosscompilationRoot}/pi.cmake -DBUILD_SHARED_LIBS=on -DGLFW_BUILD_EXAMPLES=off -DGLFW_BUILD_TESTS=off -DGLFW_BUILD_DOCS=off ..
make -j8
cp -av src/libglfw* $ProjectRoot/externals/lib
cp -av src/libglfw* ${CrosscompilationRoot}/rootfs/usr/lib/arm-linux-gnueabihf
cp -av ../include/* $ProjectRoot/externals/include
cp -av ../include/* ${CrosscompilationRoot}/rootfs/usr/include

cd ../../.. # {projectRoot}/externals

echo "Building GLFW for fedora host"

cd sources/glfw-3.2.1 # {projectRoot}/externals/sources/glfw-3.2.1
if [ -d "build" ]; then
	rm -rf build
fi
mkdir build
cd build # {projectRoot}/externals/sources/glfw-3.2.1/build

cmake -DBUILD_SHARED_LIBS=on -DGLFW_BUILD_EXAMPLES=off -DGLFW_BUILD_TESTS=off -DGLFW_BUILD_DOCS=off ..
make -j8
cp -av src/libglfw* $ProjectRoot/externals/lib64

cd ../../.. # {projectRoot}/externals

cd .. # {projectRoot}

echo "GLFW setup done"

echo "Setup completed! Ready to roll."
