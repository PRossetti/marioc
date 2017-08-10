################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/funciones_aux.c \
../src/fuse_main.c 

OBJS += \
./src/funciones_aux.o \
./src/fuse_main.o 

C_DEPS += \
./src/funciones_aux.d \
./src/fuse_main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64 -lpthread -lf -I"/home/utnso/workspace/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


