################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Sampler.cpp \
../src/fhog.cpp \
../src/kcftracker.cpp 

OBJS += \
./src/Sampler.o \
./src/fhog.o \
./src/kcftracker.o 

CPP_DEPS += \
./src/Sampler.d \
./src/fhog.d \
./src/kcftracker.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include/opencv -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


