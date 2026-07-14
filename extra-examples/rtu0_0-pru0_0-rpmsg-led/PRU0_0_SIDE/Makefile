# PRU0_0 led_worker — blink P8_11 from shared DMEM mailbox (pair with RTU)
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
GEN_DIR := $(OUT_DIR)/obj/pru
SHARED_INC := $(DEMO_ROOT)/ICSSG0_SHARED/include

CC := $(PRU_CGT)/bin/clpru
LINKER_CMD := linker/J721E_PRU0_0_worker.cmd

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

WORKER_SRC := src/led_worker/main.c
WORKER_OBJ := $(GEN_DIR)/led_worker_main.object
WORKER_OUT := $(OUT_DIR)/pru0_0-led-worker.out
WORKER_MAP := $(OUT_DIR)/pru0_0-led-worker.map

.PHONY: all led_worker clean

all: led_worker

led_worker: $(WORKER_OUT)

$(GEN_DIR):
	@mkdir -p $(GEN_DIR) $(OUT_DIR)

$(WORKER_OBJ): $(WORKER_SRC) include/resource_table_led_worker.h \
		$(SHARED_INC)/led_mailbox.h | $(GEN_DIR)
	$(CC) $(CFLAGS) -fe $@ $<

$(WORKER_OUT): $(WORKER_OBJ) $(LINKER_CMD)
	$(CC) $(CFLAGS) -z -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) \
		-o $@ $< -m$(WORKER_MAP) $(LINKER_CMD) $(LIBS_COMMON)

clean:
	rm -rf $(GEN_DIR) $(WORKER_OUT) $(WORKER_MAP)
