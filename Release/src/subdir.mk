################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/board.c \
../src/eval.c \
../src/main.c \
../src/moves.c \
../src/play.c \
../src/search.c \
../src/tests.c \
../src/uci.c 

OBJS += \
./src/board.o \
./src/eval.o \
./src/main.o \
./src/moves.o \
./src/play.o \
./src/search.o \
./src/tests.o \
./src/uci.o 

C_DEPS += \
./src/board.d \
./src/eval.d \
./src/main.d \
./src/moves.d \
./src/play.d \
./src/search.d \
./src/tests.d \
./src/uci.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


