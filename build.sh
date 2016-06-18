#!/bin/bash
rm -rf modules
export CONFIG_FILE="apq8084-perf_defconfig"
export ARCH="arm"
export CROSS_COMPILE="arm-eabi-"
export TOOL_CHAIN_PATH="${HOME}/kernel/ubertc/bin"
export CONFIG_ABS_PATH="arch/${ARCH}/configs/${CONFIG_FILE}"
export PATH=$PATH:${TOOL_CHAIN_PATH}
make -j4 $CONFIG_FILE
make -j4
mkdir modules
find . -name '*.ko' -exec cp -av {} modules/ \;
# strip modules 
${TOOL_CHAIN_PATH}/${CROSS_COMPILE}strip --strip-unneeded modules/*
mkdir modules/qca_cld
mv modules/wlan.ko modules/qca_cld/qca_cld_wlan.ko
