################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables
SYSCFG_SRCS += \
../empty.syscfg

C_SRCS += \
../clock.c \
../control.c \
../display.c \
../encoder.c \
../icm42688.c \
../main.c \
../motor.c \
../odometry.c \
../oled.c \
../pid.c \
../uart_comm.c \
./ti_msp_dl_config.c \
C:/TI/mspm0_sdk_2_05_01_00/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c

GEN_CMDS += \
./device_linker.cmd

GEN_FILES += \
./device_linker.cmd \
./device.opt \
./ti_msp_dl_config.c

C_DEPS += \
./clock.d \
./control.d \
./display.d \
./encoder.d \
./icm42688.d \
./main.d \
./motor.d \
./odometry.d \
./oled.d \
./pid.d \
./uart_comm.d \
./ti_msp_dl_config.d \
./startup_mspm0g350x_ticlang.d

GEN_OPTS += \
./device.opt

OBJS += \
./clock.o \
./control.o \
./display.o \
./encoder.o \
./icm42688.o \
./main.o \
./motor.o \
./odometry.o \
./oled.o \
./pid.o \
./uart_comm.o \
./ti_msp_dl_config.o \
./startup_mspm0g350x_ticlang.o

GEN_MISC_FILES += \
./device.cmd.genlibs \
./ti_msp_dl_config.h \
./Event.dot

OBJS__QUOTED += \
"clock.o" \
"control.o" \
"display.o" \
"encoder.o" \
"icm42688.o" \
"main.o" \
"motor.o" \
"odometry.o" \
"oled.o" \
"pid.o" \
"uart_comm.o" \
"ti_msp_dl_config.o" \
"startup_mspm0g350x_ticlang.o"

GEN_MISC_FILES__QUOTED += \
"device.cmd.genlibs" \
"ti_msp_dl_config.h" \
"Event.dot"

C_DEPS__QUOTED += \
"clock.d" \
"control.d" \
"display.d" \
"encoder.d" \
"icm42688.d" \
"main.d" \
"motor.d" \
"odometry.d" \
"oled.d" \
"pid.d" \
"uart_comm.d" \
"ti_msp_dl_config.d" \
"startup_mspm0g350x_ticlang.d"

GEN_FILES__QUOTED += \
"device_linker.cmd" \
"device.opt" \
"ti_msp_dl_config.c"

C_SRCS__QUOTED += \
"../clock.c" \
"../control.c" \
"../display.c" \
"../encoder.c" \
"../icm42688.c" \
"../main.c" \
"../motor.c" \
"../odometry.c" \
"../oled.c" \
"../pid.c" \
"../uart_comm.c" \
"./ti_msp_dl_config.c" \
"C:/TI/mspm0_sdk_2_05_01_00/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c"

SYSCFG_SRCS__QUOTED += \
"../empty.syscfg"
