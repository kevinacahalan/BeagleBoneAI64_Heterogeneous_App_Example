BUILD_DIR := ../build/R5_0
APP ?= $(BUILD_DIR)/r5f_$(CORE).elf

# Extract the directory part of the application path
APP_DIR := $(dir $(APP))

# These cores also go by R5FSS0_CORE0, R5FSS0_CORE1, R5FSS1_CORE0, and R5FSS1_CORE1 elsewhere.
# And also by mcu2_0, mcu2_1, mcu3_0 and mcu3_1.
CORE_LIST_MAIN_R5 = r5f0_0 r5f0_1 r5f1_0 r5f1_1

CROSS_COMPILE ?= arm-none-eabi-

INCLUDE_DIR = r5_code/include

# Setup spots compiler checkers for header files
INCLUDES += -I$(PDK_PATH)/packages
INCLUDES += -I$(INCLUDE_DIR)
INCLUDES += -I. 
INCLUDES += -Ir5

INCL += $(wildcard $(INCLUDE_DIR)/*.h) 
INCL += $(wildcard r5/*.h)
INCL += $(wildcard $(INCLUDE_DIR)/ai64/*.h)
INCL += $(wildcard $(INCLUDE_DIR)/io_test_functions/*.h)

SRCDIR = r5_code/src
SRC += $(wildcard $(SRCDIR)/*.c) 
SRC += $(wildcard $(SRCDIR)/ai64/*.c)
SRC += $(wildcard $(SRCDIR)/io_test_functions/*.c)
SRC += $(wildcard ../SHARED_CODE/src/*.c) 
SRC += $(wildcard r5/*.S) 
SRC += $(wildcard r5/*.c)



CROSS_CC ?= $(CROSS_COMPILE)gcc
CROSS_SIZE ?= $(CROSS_COMPILE)size
CROSS_OBJDUMP ?= $(CROSS_COMPILE)objdump


# Define the SOC, the TI libraries act different depening on the SOC defined
DEFINES = -DSOC_J721E -DJ721E_TDA4VM
DEFINES += -DBUILD_MCU2_0
DEFINES += -DBAREMETAL
DEFINES += -DA72_LINUX_OS # Tell TI to not do stuff that Linux already deals with
DEFINES += -DR5

# MAKE SURE YOU HAVE YOUR PDK INSTALLED AND IN THE CORRECT LOCATION
PDK_PATH = $(HOME)/ti/ti-processor-sdk-rtos-j721e-evm-10_00_00_05/pdk_jacinto_10_00_00_27


# CSL
CSL_LIBPATH := $(PDK_PATH)/packages/ti/csl/lib/j721e/r5f/debug
#CSL_LIBS = $(CSL_LIBPATH)/ti.csl.aer5f $(CSL_LIBPATH)/ti.csl.init.aer5f
CSL_LIBS = $(CSL_LIBPATH)/ti.csl.aer5f

#OSAL
OSAL_LIBPATH := $(PDK_PATH)/packages/ti/osal/lib/nonos/j721e/r5f/debug
OSAL_LIBS = $(OSAL_LIBPATH)/ti.osal.aer5f

#BOARD (not really useful with the bbai64 from my understanding)
BOARD_LIBPATH := $(PDK_PATH)/packages/ti/board/lib/j721e_evm/r5f/debug
BOARD_LIBS = $(BOARD_LIBPATH)/ti.board.aer5f

#SCI_CLIENT
SCI_CLIENT_LIBPATH := $(PDK_PATH)/packages/ti/drv/sciclient/lib/j721e/mcu2_0/release
SCI_CLIENT_LIBS := $(SCI_CLIENT_LIBPATH)/sciclient.aer5f 
SCI_CLIENT_LIBS += $(SCI_CLIENT_LIBPATH)/sciclient_hs.aer5f

#IPC
IPC_LIBPATH := $(PDK_PATH)/packages/ti/drv/ipc/lib/j721e/mcu2_0/release
IPC_LIBS += $(IPC_LIBPATH)/ipc.aer5f
IPC_LIBS := $(IPC_LIBPATH)/ipc_baremetal.aer5f

# SPI
SPI_LIBPATH := $(PDK_PATH)/packages/ti/drv/spi/lib/j721e/r5f/debug
SPI_LIBS := $(SPI_LIBPATH)/ti.drv.spi.aer5f 
SPI_LIBS += $(SPI_LIBPATH)/ti.drv.spi.dma.aer5f

# UART
UART_LIBPATH := $(PDK_PATH)/packages/ti/drv/uart/lib/j721e/r5f/debug
UART_LIBS := $(UART_LIBPATH)/ti.drv.uart.aer5f 
UART_LIBS += $(UART_LIBPATH)/ti.drv.uart.dma.aer5f

# I2C
I2C_LIBPATH := $(PDK_PATH)/packages/ti/drv/i2c/lib/j721e/r5f/debug
I2C_LIBS = $(I2C_LIBPATH)/ti.drv.i2c.aer5f

# Setup LDFLAGS
LIBS := $(SPI_LIBS) 
LIBS += $(CSL_LIBS) 
LIBS += $(OSAL_LIBS) 
LIBS += $(SCI_CLIENT_LIBS)
LIBS += $(IPC_LIBS) 
LIBS += $(BOARD_LIBS)
LIBS += $(UART_LIBS)
LIBS += $(I2C_LIBS)

LDFLAGS = -Wl,--warn-common,--warn-section-align,--warn-unresolved-symbols
LDFLAGS += -Wl,--start-group $(LIBS) -Wl,--end-group 
LDFLAGS += -Wl,-z,noexecstack
LDFLAGS += -lm


# This base address picking code is dead as __DDR_START__ is nolonger used in the linker script. Edit the linker script
# directly to change which core to run the firmware on.
CORE ?= r5f0_0
# KERNEL ?= 5
# Kernel 5 address https://git.beagleboard.org/beagleboard/BeagleBoard-DeviceTrees/-/blob/v5.10.x-ti-unified/src/arm64/k3-j721e-rtos-memory-map.dtsi#L151
# LDFLAGS += -Wl,--defsym,__DDR_START__=0xA2000000



CFGENERAL = -fpic -ffreestanding -ffunction-sections -fdata-sections -fno-exceptions -mcpu=cortex-r5 -marm -mfloat-abi=hard -ggdb -u _printf_float -u _printf_int64 -u _printf_uint64
WARNINGS = -W -Wall -Wshadow -fno-common
STANDARD =
CFLAGS := -g3 -Og $(WARNINGS) $(CFGENERAL) $(DEFINES) $(INCLUDES)

.PHONY: clean all help

all: $(APP)
	$(info )
	$(info --- Make sure to run `make help` for help and options)
	$(info )
help:
	@echo ------------------------------------------------------
	@echo R5F firmware make help
	@echo ------------------------------------------------------
	@echo make [OPTIONAL TARGET] [OPTIONAL MAKE VARIABLES]
	@echo
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all            : Builds all libraries and examples for the provided CORE and KERNEL"
	@echo "clean          : Cleans"
	@echo "help           : Prints help message"
	@echo ""
	@echo ""
	@echo "Optional make variables:"
	@echo "------------------------"
	@echo "[there are no options currently]"


clean:
	rm -f $(APP)
	rm -f $(APP).lst

# Target to create the build directory
$(APP_DIR):
	mkdir -p $@

$(APP): $(APP_DIR) $(SRC) $(INCL) gcc.ld Makefile
	$(CROSS_CC) $(CFLAGS) --specs=nosys.specs --specs=nano.specs -T gcc.ld -o $(APP) $(SRC) $(LDFLAGS)
	$(CROSS_SIZE) $(APP)
	$(CROSS_OBJDUMP) -xd $(APP) > $(APP).lst

