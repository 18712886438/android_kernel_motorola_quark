#!/bin/bash
rm -rf modules
export CONFIG_FILE="apq8084-perf_defconfig"
export ARCH="arm"
export CROSS_COMPILE="arm-eabi-"
export TOOL_CHAIN_PATH="${HOME}/Mokee/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin"
export CONFIG_ABS_PATH="arch/${ARCH}/configs/${CONFIG_FILE}"
export PATH=$PATH:${TOOL_CHAIN_PATH}
make -j4 $CONFIG_FILE
make -j4
mkdir modules
find . -name '*.ko' -exec cp -av {} modules/ \;
