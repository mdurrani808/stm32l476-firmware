################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/blink_system/blink_system.c 

OBJS += \
./App/blink_system/blink_system.o 

C_DEPS += \
./App/blink_system/blink_system.d 


# Each subdirectory must supply rules for building sources it contributes
App/blink_system/%.o App/blink_system/%.su App/blink_system/%.cyclo: ../App/blink_system/%.c App/blink_system/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../Core/Inc -I../App/Inc -I../Platform/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-blink_system

clean-App-2f-blink_system:
	-$(RM) ./App/blink_system/blink_system.cyclo ./App/blink_system/blink_system.d ./App/blink_system/blink_system.o ./App/blink_system/blink_system.su

.PHONY: clean-App-2f-blink_system

