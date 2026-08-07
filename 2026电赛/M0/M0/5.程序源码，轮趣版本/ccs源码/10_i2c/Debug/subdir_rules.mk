################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccstheia151/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/CCS/10_i2c" -I"D:/CCS/10_i2c/Debug" -I"D:/ti/mspm0_sdk_2_03_00_07/source/third_party/CMSIS/Core/Include" -I"D:/ti/mspm0_sdk_2_03_00_07/source" -I"D:/CCS/10_i2c/BSP" -I"D:/CCS/10_i2c/BSP/eMPL" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-403131441: ../empty.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"D:/ti/ccstheia151/ccs/utils/sysconfig_1.21.1/sysconfig_cli.bat" --script "D:/CCS/10_i2c/empty.syscfg" -o "." -s "D:/ti/mspm0_sdk_2_03_00_07/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-403131441 ../empty.syscfg
device.opt: build-403131441
device.cmd.genlibs: build-403131441
ti_msp_dl_config.c: build-403131441
ti_msp_dl_config.h: build-403131441
Event.dot: build-403131441

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccstheia151/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/CCS/10_i2c" -I"D:/CCS/10_i2c/Debug" -I"D:/ti/mspm0_sdk_2_03_00_07/source/third_party/CMSIS/Core/Include" -I"D:/ti/mspm0_sdk_2_03_00_07/source" -I"D:/CCS/10_i2c/BSP" -I"D:/CCS/10_i2c/BSP/eMPL" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: D:/ti/mspm0_sdk_2_03_00_07/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccstheia151/ccs/tools/compiler/ti-cgt-armllvm_4.0.0.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"D:/CCS/10_i2c" -I"D:/CCS/10_i2c/Debug" -I"D:/ti/mspm0_sdk_2_03_00_07/source/third_party/CMSIS/Core/Include" -I"D:/ti/mspm0_sdk_2_03_00_07/source" -I"D:/CCS/10_i2c/BSP" -I"D:/CCS/10_i2c/BSP/eMPL" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


