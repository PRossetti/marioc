################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/nivel/Gui/nivel_gui.c \
../src/nivel/Gui/tad_items.c 

OBJS += \
./src/nivel/Gui/nivel_gui.o \
./src/nivel/Gui/tad_items.o 

C_DEPS += \
./src/nivel/Gui/nivel_gui.d \
./src/nivel/Gui/tad_items.d 


# Each subdirectory must supply rules for building sources it contributes
src/nivel/Gui/%.o: ../src/nivel/Gui/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/so-commons-library" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


