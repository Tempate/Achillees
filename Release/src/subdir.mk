################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/board.c \
../src/draw.c \
../src/eval.c \
../src/hashtables.c \
../src/main.c \
../src/moves.c \
../src/play.c \
../src/search.c \
../src/sort.c \
../src/tests.c \
../src/uci.c 

OBJS += \
./src/board.o \
./src/draw.o \
./src/eval.o \
./src/hashtables.o \
./src/main.o \
./src/moves.o \
./src/play.o \
./src/search.o \
./src/sort.o \
./src/tests.o \
./src/uci.o 

C_DEPS += \
./src/board.d \
./src/draw.d \
./src/eval.d \
./src/hashtables.d \
./src/main.d \
./src/moves.d \
./src/play.d \
./src/search.d \
./src/sort.d \
./src/tests.d \
./src/uci.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -std=c11 -O3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


