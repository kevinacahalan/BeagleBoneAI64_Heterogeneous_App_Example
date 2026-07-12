# PRU0_0_SIDE — ICSSG0 PRU0 (j7-pru0_0) firmware
#
# Build inside the TI container via:
#   ./scripts/build.sh --setup   # once: SDK/PDK + PSSP under ~/ti
#   ./scripts/build.sh --pru     # PRU0_0 firmware only
#
# Requires PRU_CGT (Dockerfile.ti → /usr/share/ti/cgt-pru) and PSSP under
# $HOME/ti/pru-software-support-package (from --setup; mounted at
# /home/builder/ti in the container).

ifndef PRU_CGT
  PRU_CGT := /usr/share/ti/cgt-pru
endif

# PSSP lives next to the Processor SDK (not in the git tree).
PSSP ?= /home/builder/ti/pru-software-support-package

REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
OUT_DIR := $(REPO_ROOT)/build/PRU0_0
GEN_DIR := $(OUT_DIR)/obj

CC := $(PRU_CGT)/bin/clpru
LD := $(PRU_CGT)/bin/lnkpru
LINKER_CMD_HELLO := linker/J721E_PRU0_0.cmd
LINKER_CMD_RPMSG := linker/J721E_PRU0_0_rpmsg.cmd

INCLUDE := --include_path=$(PRU_CGT)/include \
	--include_path=$(PSSP)/include \
	--include_path=$(PSSP)/include/j721e \
	--include_path=$(abspath include)

CFLAGS := -v3 -O2 --display_error_number --endian=little --hardware_mac=on \
	--obj_directory=$(GEN_DIR) --pp_directory=$(GEN_DIR) -ppd -ppa \
	$(INCLUDE)

LFLAGS := --reread_libs --warn_sections --stack_size=0x100 --heap_size=0x100
LIBS_COMMON := --library=$(PRU_CGT)/lib/libc.a --library=rtspruv3_le.lib
LIBS_RPMSG := --library=$(PSSP)/lib/rpmsg_lib.lib

HELLO_SRC := src/hello/main.c
HELLO_OBJ := $(GEN_DIR)/hello_main.object
HELLO_OUT := $(OUT_DIR)/pru0_0-hello.out
HELLO_MAP := $(OUT_DIR)/pru0_0-hello.map

RPMSG_SRC := src/rpmsg_led/main.c
RPMSG_OBJ := $(GEN_DIR)/rpmsg_led_main.object
RPMSG_OUT := $(OUT_DIR)/pru0_0-rpmsg-led.out
RPMSG_MAP := $(OUT_DIR)/pru0_0-rpmsg-led.map

.PHONY: all hello rpmsg_led clean

all: hello rpmsg_led

hello: $(HELLO_OUT)

rpmsg_led: $(RPMSG_OUT)

$(GEN_DIR):
	@mkdir -p $(GEN_DIR) $(OUT_DIR)

$(HELLO_OBJ): $(HELLO_SRC) include/resource_table_hello.h | $(GEN_DIR)
	$(CC) $(CFLAGS) -fe $@ $<

$(HELLO_OUT): $(HELLO_OBJ) $(LINKER_CMD_HELLO)
	$(CC) $(CFLAGS) -z -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) \
		-o $@ $< -m$(HELLO_MAP) $(LINKER_CMD_HELLO) $(LIBS_COMMON)

$(RPMSG_OBJ): $(RPMSG_SRC) include/resource_table_rpmsg.h include/intc_map_0.h | $(GEN_DIR)
	$(CC) $(CFLAGS) -fe $@ $<

$(RPMSG_OUT): $(RPMSG_OBJ) $(LINKER_CMD_RPMSG)
	$(CC) $(CFLAGS) -z -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) \
		-o $@ $< -m$(RPMSG_MAP) $(LINKER_CMD_RPMSG) $(LIBS_COMMON) $(LIBS_RPMSG)

clean:
	rm -rf $(OUT_DIR)
