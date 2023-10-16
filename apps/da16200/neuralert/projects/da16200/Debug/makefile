################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include library/subdir.mk
-include customer_app/src/user_main/subdir.mk
-include customer_app/src/apps/subdir.mk
-include core/system/src/provision/subdir.mk
-include core/system/src/ota/subdir.mk
-include core/system/src/mqtt_client/lib/subdir.mk
-include core/system/src/mqtt_client/client/subdir.mk
-include core/system/src/diag/subdir.mk
-include core/system/src/common/nvram/subdir.mk
-include core/system/src/common/network/subdir.mk
-include core/system/src/common/main/subdir.mk
-include core/system/src/common/library/subdir.mk
-include core/system/src/common/command/subdir.mk
-include core/system/src/common/subdir.mk
-include core/system/src/coap/subdir.mk
-include core/system/src/at_cmd/subdir.mk
-include core/segger_tools/SEGGER/subdir.mk
-include core/segger_tools/OS/subdir.mk
-include core/segger_tools/Config/subdir.mk
-include core/libraries/3rdparty/lwip_osal/src/subdir.mk
-include core/libraries/3rdparty/lwip/src/portable/dialog/da16200/netif/subdir.mk
-include core/libraries/3rdparty/lwip/src/netif/ppp/polarssl/subdir.mk
-include core/libraries/3rdparty/lwip/src/netif/ppp/subdir.mk
-include core/libraries/3rdparty/lwip/src/netif/subdir.mk
-include core/libraries/3rdparty/lwip/src/core/ipv6/subdir.mk
-include core/libraries/3rdparty/lwip/src/core/ipv4/subdir.mk
-include core/libraries/3rdparty/lwip/src/core/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/tftp/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/sntp/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/snmp/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/smtp/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/ping/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/netbiosns/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/mdns/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/lwiperf/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/http/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/dhcpserver/subdir.mk
-include core/libraries/3rdparty/lwip/src/apps/altcp_tls/subdir.mk
-include core/libraries/3rdparty/lwip/src/api/subdir.mk
-include core/libraries/3rdparty/json/subdir.mk
-include core/libraries/3rdparty/crypto/src/subdir.mk
-include core/freertos/portable/MemMang/subdir.mk
-include core/freertos/portable/GCC/ARM_CM4F/subdir.mk
-include core/freertos/portable/Common/subdir.mk
-include core/freertos/subdir.mk
-include core/bsp/startup/subdir.mk
-include core/bsp/driver/src/DA16200/sflash/subdir.mk
-include core/bsp/driver/src/DA16200/subdir.mk
-include core/bsp/cortexm/src/subdir.mk
-include core/bsp/cmsis/src/subdir.mk
-include app_common/src/util/subdir.mk
-include app_common/src/provisioning/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := tcp_client_sleep2_sample
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_SIZE += \
tcp_client_sleep2_sample.siz \


# All Target
all:
	+@$(MAKE) --no-print-directory pre-build && $(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: tcp_client_sleep2_sample.elf secondary-outputs

# Tool invocations
tcp_client_sleep2_sample.elf: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM Cross C++ Linker'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wextra  -g3 -T sections_xip.ld -nostartfiles -Xlinker --gc-sections -L../../../../../../../../core/bsp/ldscripts -L../../../../../../../../library -Wl,-Map,"tcp_client_sleep2_sample.map" --specs=nano.specs --specs=nosys.specs -o "tcp_client_sleep2_sample.elf" $(OBJS) $(USER_OBJS) -Wl,--start-group $(LIBS) -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

tcp_client_sleep2_sample.siz: tcp_client_sleep2_sample.elf makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: GNU ARM Cross Print Size'
	arm-none-eabi-size --format=berkeley "tcp_client_sleep2_sample.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(OBJS)$(C_UPPER_DEPS)$(CXX_DEPS)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS)$(CPP_DEPS) tcp_client_sleep2_sample.elf
	-@echo ' '

pre-build:
	-bash "C:\Stclients\Inteprod\Neuralert\Source\Stage10f\SRC-9079-Stage10f\apps\common\examples\Network\TCP_Client_Sleep2\projects\da16200\Debug/../../../../../../../../tools/util/prebuild_start.sh" "C:\Stclients\Inteprod\Neuralert\Source\Stage10f\SRC-9079-Stage10f\apps\common\examples\Network\TCP_Client_Sleep2\projects\da16200\Debug/../../../../../../../../tools" "dummy" "C:\Stclients\Inteprod\Neuralert\Source\Stage10f\SRC-9079-Stage10f\apps\common\examples\Network\TCP_Client_Sleep2\projects\da16200\Debug" "tcp_client_sleep2_sample"
	-@echo ' '

post-build:
	-bash "C:\Stclients\Inteprod\Neuralert\Source\Stage10f\SRC-9079-Stage10f\apps\common\examples\Network\TCP_Client_Sleep2\projects\da16200\Debug/../../../../../../../../tools/util/postbuild_start.sh" "C:\Stclients\Inteprod\Neuralert\Source\Stage10f\SRC-9079-Stage10f\apps\common\examples\Network\TCP_Client_Sleep2\projects\da16200\Debug/../../../../../../../../tools" "dummy" "C:\Stclients\Inteprod\Neuralert\Source\Stage10f\SRC-9079-Stage10f\apps\common\examples\Network\TCP_Client_Sleep2\projects\da16200\Debug" "tcp_client_sleep2_sample" DA16200
	-@echo ' '

secondary-outputs: $(SECONDARY_SIZE)

.PHONY: all clean dependents main-build pre-build post-build

-include ../makefile.targets
