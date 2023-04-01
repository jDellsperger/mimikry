#!/bin/bash

projectRoot=$PWD

cd $projectRoot

export PATH=$PATH:$PWD/externals/raspberrypi/tools-master/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin

commonWarningFlags="-Wno-unused-function -Wno-unused-variable -Wno-sign-compare -Wno-switch -Wno-unused-but-set-variable -Wall -Werror"
commonCompilerFlags="$commonWarningFlags -O0 -g -DBUILD_DEBUG -std=c++11"
commonLinkerFlags="-g"
commonX64CompilerFlags="-Wl,-rpath,$PWD/externals/lib64 -L$PWD/externals/lib64 -I$PWD/externals/include"

# ---------------------------------------------------------------
# Spotter
# ---------------------------------------------------------------

echo "Building Spotter"

spotterCompilerFlags="$commonCompilerFlags"
spotterLinkerFlags="$commonLinkerFlags -lGLESv2 -ldl -lopencv_core -lopencv_calib3d -lopencv_imgproc -lopencv_features2d -lzmq"
x64SpotterCompilerFlags="$spotterCompilerFlags $commonX64CompilerFlags -DINTERFACE=\"enp0s25\" -DCAPTURE_FRAME_WIDTH=640 -DCAPTURE_FRAME_HEIGHT=480 -DWINDOW_WIDTH=640 -DWINDOW_HEIGHT=480"
armSpotterCompilerFlags="$spotterCompilerFlags -DINTERFACE=\"eth0\" -DCAPTURE_FRAME_WIDTH=1640 -DCAPTURE_FRAME_HEIGHT=1232 -DWINDOW_WIDTH=1640 -DWINDOW_HEIGHT=1232"

g++ $x64SpotterCompilerFlags -o build/x64/spotter sources/spotter/linux_spotter.cpp $spotterLinkerFlags
arm-linux-gnueabihf-g++ --sysroot=$PWD/externals/raspberrypi/rootfs $armSpotterCompilerFlags -o build/arm/spotter sources/spotter/linux_spotter.cpp $spotterLinkerFlags

# ---------------------------------------------------------------
# Spotter Calibration
# ---------------------------------------------------------------

echo "Building Spotter Calibration"

spotterCalibCompilerFlags="$commonCompilerFlags"
spotterCalibLinkerFlags="$commonLinkerFlags -lGLESv2 -ldl -lglfw -lopencv_core -lopencv_calib3d -lopencv_imgproc -lopencv_features2d"
x64SpotterCalibCompilerFlags="$spotterCalibCompilerFlags  $commonX64CompilerFlags -DINTERFACE=\"enp0s25\" -DCAPTURE_FRAME_WIDTH=640 -DCAPTURE_FRAME_HEIGHT=480 -DWINDOW_WIDTH=320 -DWINDOW_HEIGHT=240"
armSpotterCalibCompilerFlags="$spotterCalibCompilerFlags -DINTERFACE=\"eth0\" -DCAPTURE_FRAME_WIDTH=1640 -DCAPTURE_FRAME_HEIGHT=1232 -DWINDOW_WIDTH=410 -DWINDOW_HEIGHT=308"

g++ $x64SpotterCalibCompilerFlags -o build/x64/spotterCalibration sources/spotter/glfw_spotterCalibration.cpp $spotterCalibLinkerFlags
arm-linux-gnueabihf-g++ --sysroot=$PWD/externals/raspberrypi/rootfs $armSpotterCalibCompilerFlags -o build/arm/spotterCalibration sources/spotter/glfw_spotterCalibration.cpp $spotterCalibLinkerFlags

# ---------------------------------------------------------------
# Beholder
# ---------------------------------------------------------------

echo "Building Beholder"

beholderCompilerFlags="$commonCompilerFlags $commonX64CompilerFlags"
beholderLinkerFlags="$commongLinkerFlags -lpthread -lzmq"

g++ $beholderCompilerFlags -o build/x64/beholder sources/beholder/linux_beholder.cpp $beholderLinkerFlags

