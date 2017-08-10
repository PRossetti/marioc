################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../commons/sogt/auxiliares.c \
../commons/sogt/mensajeria.c \
../commons/sogt/sockets.c 

OBJS += \
./commons/sogt/auxiliares.o \
./commons/sogt/mensajeria.o \
./commons/sogt/sockets.o 

C_DEPS += \
./commons/sogt/auxiliares.d \
./commons/sogt/mensajeria.d \
./commons/sogt/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
commons/sogt/%.o: ../commons/sogt/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


