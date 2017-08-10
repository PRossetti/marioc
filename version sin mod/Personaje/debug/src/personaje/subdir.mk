################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/personaje/Personaje.c 

OBJS += \
./src/personaje/Personaje.o 

C_DEPS += \
./src/personaje/Personaje.d 


# Each subdirectory must supply rules for building sources it contributes
src/personaje/%.o: ../src/personaje/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/so-commons-library" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


