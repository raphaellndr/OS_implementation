################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/lists/generic_list.c 

OBJS += \
./component/lists/generic_list.o 

C_DEPS += \
./component/lists/generic_list.d 


# Each subdirectory must supply rules for building sources it contributes
component/lists/%.o: ../component/lists/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\board" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\source" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\drivers" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\device" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\CMSIS" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\utilities" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\component\serial_manager" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\component\lists" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\component\uart" -I"F:\Enib\Softwares\NXP\MCUXpressoIDE_11.1.1_3241\workspace\LPC55S69_OS.src\startup" -I"/LPC55S69_OS.src/kernel" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


