# RTU0_0 rpmsg_led — ICSSG0 RTU0 (j7-rtu0_0) firmware
#
# Build via repo root:
#   ./scripts/build.sh --setup
#   ./scripts/build.sh --extras
#   ./scripts/build.sh --extra rtu0_0-pru0_0-rpmsg-led

ifndef PRU_CGT
  PRU_CGT := /usr/share/ti/cgt-pru
endif

PSSP ?= /home/builder/ti/pru-software-support-package

DEMO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
REPO_ROOT := $(abspath $(DEMO_ROOT)/../..)
OUT_DIR := $(REPO_ROOT)/build/extra-examples/rtu0_0-pru0_0-rpmsg-led
GEN_DIR := $(OUT_DIR)/obj/rtu
SHARED_INC := $(DEMO_ROOT)/ICSSG0_SHARED/include

CC := $(PRU_CGT)/bin/clpru
LINKER_CMD := linker/J721E_RTU0_0_rpmsg.cmd

INCLUDE := --include_path=$(PRU_CGT)/include \
	--include_path=$(PSSP)/include \
	--include_path=$(PSSP)/include/j721e \
	--include_path=$(abspath include) \
	--include_path=$(SHARED_INC)

CFLAGS := -v3 -O2 --display_error_number --endian=little --hardware_mac=on \
	--obj_directory=$(GEN_DIR) --pp_directory=$(GEN_DIR) -ppd -ppa \
	$(INCLUDE)

LFLAGS := --reread_libs --warn_sections --stack_size=0x100 --heap_size=0x100
LIBS_COMMON := --library=$(PRU_CGT)/lib/libc.a --library=rtspruv3_le.lib
LIBS_RPMSG := --library=$(PSSP)/lib/rpmsg_lib.lib

RPMSG_SRC := src/rpmsg_led/main.c
RPMSG_OBJ := $(GEN_DIR)/rpmsg_led_main.object
RPMSG_OUT := $(OUT_DIR)/rtu0_0-rpmsg-led.elf
RPMSG_MAP := $(OUT_DIR)/rtu0_0-rpmsg-led.map

.PHONY: all rpmsg_led clean

all: rpmsg_led

rpmsg_led: $(RPMSG_OUT)

$(GEN_DIR):
	@mkdir -p $(GEN_DIR) $(OUT_DIR)

$(RPMSG_OBJ): $(RPMSG_SRC) include/resource_table_rpmsg.h include/intc_map_0.h \
		$(SHARED_INC)/led_mailbox.h | $(GEN_DIR)
	$(CC) $(CFLAGS) -fe $@ $<

$(RPMSG_OUT): $(RPMSG_OBJ) $(LINKER_CMD)
	$(CC) $(CFLAGS) -z -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) \
		-o $@ $< -m$(RPMSG_MAP) $(LINKER_CMD) $(LIBS_COMMON) $(LIBS_RPMSG)

clean:
	rm -rf $(GEN_DIR) $(RPMSG_OUT) $(RPMSG_MAP)
