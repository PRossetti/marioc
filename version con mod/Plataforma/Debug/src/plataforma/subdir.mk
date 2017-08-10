################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/plataforma/auxiliar.c \
../src/plataforma/orquestador.c \
../src/plataforma/planificador.c \
../src/plataforma/plataforma.c 

OBJS += \
./src/plataforma/auxiliar.o \
./src/plataforma/orquestador.o \
./src/plataforma/planificador.o \
./src/plataforma/plataforma.o 

C_DEPS += \
./src/plataforma/auxiliar.d \
./src/plataforma/orquestador.d \
./src/plataforma/planificador.d \
./src/plataforma/plataforma.d 


# Each subdirectory must supply rules for building sources it contributes
src/plataforma/%.o: ../src/plataforma/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/so-commons-library" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


