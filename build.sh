#!/bin/bash
# Timer Count
START=$(date +%s.%N);
START2="$(date)";
echo -e "\n build start $(date)\n";
# Clean 
make clean
rm -rf modules && rm arch/arm/boot/dt.img && rm mkboot/boot.img
# Export
export CONFIG_FILE="apq8084-perf_defconfig"
export ARCH="arm"
export CROSS_COMPILE="arm-eabi-"
export TOOL_CHAIN_PATH="${HOME}/kernel/ubertc/bin"
export CONFIG_ABS_PATH="arch/${ARCH}/configs/${CONFIG_FILE}"
export PATH=$PATH:${TOOL_CHAIN_PATH}
# Make 
make -j4 $CONFIG_FILE
make -j4
# CP Modules
mkdir modules
find . -name '*.ko' -exec cp -av {} modules/ \;
# Strip Modules 
${TOOL_CHAIN_PATH}/${CROSS_COMPILE}strip --strip-unneeded modules/*
mkdir modules/qca_cld
mv modules/wlan.ko modules/qca_cld/qca_cld_wlan.ko
# Build DTB
tools/dtbToolCM -s 4096 -o arch/arm/boot/dt.img -p scripts/dtc/ arch/arm/boot/dts/
# Cp Kernel Image and DT Image
cp -rf arch/arm/boot/zImage mkboot/stock/zImage
cp -rf arch/arm/boot/dt.img mkboot/stock/dt.img
# Build boot.img
mkboot/mkboot stock boot.img 
# Final Time Count
END2="$(date)";
END=$(date +%s.%N);
echo -e "Build end   $END2 \n";
