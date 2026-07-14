# PRU0_0 hello — ICSSG0 PRU0 (j7-pru0_0) firmware
#
# Build via repo root:
#   ./scripts/build.sh --setup
#   ./scripts/build.sh --extras
#   ./scripts/build.sh --extra pru0_0-hello

ifndef PRU_CGT
  PRU_CGT := /usr/share/ti/cgt-pru
endif

PSSP ?= /home/builder/ti/pru-software-support-package

DEMO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
REPO_ROOT := $(abspath $(DEMO_ROOT)/../..)
OUT_DIR := $(REPO_ROOT)/build/extra-examples/pru0_0-hello
GEN_DIR := $(OUT_DIR)/obj

CC := $(PRU_CGT)/bin/clpru
LINKER_CMD := linker/J721E_PRU0_0.cmd

INCLUDE := --include_path=$(PRU_CGT)/include \
	--include_path=$(PSSP)/include \
	--include_path=$(PSSP)/include/j721e \
	--include_path=$(abspath include)

CFLAGS := -v3 -O2 --display_error_number --endian=little --hardware_mac=on \
	--obj_directory=$(GEN_DIR) --pp_directory=$(GEN_DIR) -ppd -ppa \
	$(INCLUDE)

LFLAGS := --reread_libs --warn_sections --stack_size=0x100 --heap_size=0x100
LIBS_COMMON := --library=$(PRU_CGT)/lib/libc.a --library=rtspruv3_le.lib

HELLO_SRC := src/hello/main.c
HELLO_OBJ := $(GEN_DIR)/hello_main.object
HELLO_OUT := $(OUT_DIR)/pru0_0-hello.elf
HELLO_MAP := $(OUT_DIR)/pru0_0-hello.map

.PHONY: all hello clean

all: hello

hello: $(HELLO_OUT)

$(GEN_DIR):
	@mkdir -p $(GEN_DIR) $(OUT_DIR)

$(HELLO_OBJ): $(HELLO_SRC) include/resource_table_hello.h | $(GEN_DIR)
	$(CC) $(CFLAGS) -fe $@ $<

$(HELLO_OUT): $(HELLO_OBJ) $(LINKER_CMD)
	$(CC) $(CFLAGS) -z -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) \
		-o $@ $< -m$(HELLO_MAP) $(LINKER_CMD) $(LIBS_COMMON)

clean:
	rm -rf $(OUT_DIR)
