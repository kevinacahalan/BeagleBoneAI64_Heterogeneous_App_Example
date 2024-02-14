APP ?= test.elf
APP_SOURCES ?= $(wildcard r5/*.S) \
          $(wildcard r5/*.c) \
	  test.c

CROSS_COMPILE ?= arm-none-eabi-

.PHONY: $(APP)

CROSS_CC ?= $(CROSS_COMPILE)gcc
CROSS_SIZE ?= $(CROSS_COMPILE)size
CROSS_OBJDUMP ?= $(CROSS_COMPILE)objdump

ARCH ?= r5

TI_LIB_PATH = $(HOME)/ti/ti-processor-sdk-rtos-j721e-evm-09_01_00_06/pdk_jacinto_09_01_00_22
INCLUDES := -I$(TI_LIB_PATH)/packages
LDFLAGS := -Wl,--start-group $(TI_LIB_PATH)/packages/ti/csl/lib/j721e/r5f/release/ti.csl.aer5f -Wl,--end-group
INCLUDES += -Ir5
DEFINES = -DSOC_J721E -DJ721E_TDA4VM

ifeq ($(ARCH),r5)
	CFLAGS += -fno-exceptions -mcpu=cortex-r5 -marm -mfloat-abi=hard -O3 $(INCLUDES) $(DEFINES)
endif

all: $(APP)

clean:
	rm -f $(APP)
	rm -f $(APP).lst

$(APP): $(APP_SOURCES) gcc.ld
	$(CROSS_CC) $(CFLAGS) --specs=nosys.specs --specs=nano.specs -T gcc.ld -o $(APP) $(APP_SOURCES) $(LDFLAGS) -u _printf_float
	$(CROSS_SIZE) $(APP)
	$(CROSS_OBJDUMP) -xd $(APP) > $(APP).lst
	# sudo cp $(APP) /lib/firmware/
	# sudo echo stop > /sys/class/remoteproc/remoteproc18/state
	# sudo echo $(APP) > /sys/class/remoteproc/remoteproc18/firmware
	# sudo echo start > /sys/class/remoteproc/remoteproc18/state
	# sudo cat /sys/kernel/debug/remoteproc/remoteproc18/trace0

