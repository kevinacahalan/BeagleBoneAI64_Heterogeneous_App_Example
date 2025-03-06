/* Linker script to configure memory regions. 
 * Need modifying for a specific board. 
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 */

/* 
   This linker script was adapted from an example and further modified by a novice in linker 
   script development. Major room for improvement likely exists.
*/



/* Kernel 5.10,main_r5fss0_core0 */
/* This value IS NOW SET BY THE MAKEFILE! */
/* __DDR_START__ = 0xA2000000; */


/* Define sizes for stacks and heap */
__stack_size = 0x100000; /* 1 MB for main stack */
__heap_size = 0x400000; /* 4MB for heap */
__guard_size = 0x100; /* 256 bytes for guard regions */

/* Stack Sizes for various modes */
__IRQ_STACK_SIZE = 0x1000;
__FIQ_STACK_SIZE = 0x1000;
__ABORT_STACK_SIZE = 0x1000;
__UND_STACK_SIZE = 0x1000;
__SVC_STACK_SIZE = 0x1000;



/* R5 memory locations */
__DDR0_ALLOCATED_START = 0xA0000000;

__MCU2_0_ALLOCATED_START = __DDR0_ALLOCATED_START + 0x02000000; /* Same for kernel 5 and 6 */
__MCU2_0_EXT_DATA_BASE = __MCU2_0_ALLOCATED_START + 0x00100000;
__MCU2_0_R5F_MEM_TEXT_BASE = __MCU2_0_ALLOCATED_START + 0x00200000;
__MCU2_0_R5F_MEM_DATA_BASE = __MCU2_0_ALLOCATED_START + 0x00300000;
__MCU2_0_DDR_SPACE_BASE = __MCU2_0_ALLOCATED_START + 0x00400000;

__MCU2_1_ALLOCATED_START = __DDR0_ALLOCATED_START + 0x03000000; /* 0x04000000 for kernel 5 */
__MCU2_1_EXT_DATA_BASE = __MCU2_1_ALLOCATED_START + 0x00100000;
__MCU2_1_R5F_MEM_TEXT_BASE = __MCU2_1_ALLOCATED_START + 0x00200000;
__MCU2_1_R5F_MEM_DATA_BASE = __MCU2_1_ALLOCATED_START + 0x00300000;
__MCU2_1_DDR_SPACE_BASE = __MCU2_1_ALLOCATED_START + 0x00400000;

__MCU3_0_ALLOCATED_START = __DDR0_ALLOCATED_START + 0x04000000; /* 0x06000000 for kernel 5 */
__MCU3_0_EXT_DATA_BASE = __MCU3_0_ALLOCATED_START + 0x00100000;
__MCU3_0_R5F_MEM_TEXT_BASE = __MCU3_0_ALLOCATED_START + 0x00200000;
__MCU3_0_R5F_MEM_DATA_BASE = __MCU3_0_ALLOCATED_START + 0x00300000;
__MCU3_0_DDR_SPACE_BASE = __MCU3_0_ALLOCATED_START + 0x00400000;

__MCU3_1_ALLOCATED_START = __DDR0_ALLOCATED_START + 0x05000000; /* 0x07000000 for kernel 5 */
__MCU3_1_EXT_DATA_BASE = __MCU3_1_ALLOCATED_START + 0x00100000;
__MCU3_1_R5F_MEM_TEXT_BASE = __MCU3_1_ALLOCATED_START + 0x00200000;
__MCU3_1_R5F_MEM_DATA_BASE = __MCU3_1_ALLOCATED_START + 0x00300000;
__MCU3_1_DDR_SPACE_BASE = __MCU3_1_ALLOCATED_START + 0x00400000;

__ATCM_START = 0x00000000;
__OCMRAM_MCU1_0_START = 0x41cfe000;
__OCMRAM_MCU1_1_START = 0x41cff000;





MEMORY
{
	RESET_VECTORS (x) : ORIGIN = 0x0, LENGTH = 0x100
    /* MCU0_R5F_0 local view */
	/* MCU0_R5F_TCMA (rwx) : ORIGIN = 0, LENGTH = 0x8000 */
    MCU0_R5F_TCMA (rwx)       : ORIGIN = 0x100,      LENGTH = 0x8000 - 0x100 /* ....For somereason this does not run*/
    MCU0_R5F_TCMB0 (rwx)   : ORIGIN = 0x41010000, LENGTH = 0x8000

    /* MCU0_R5F_1 SoC view */
    MCU0_R5F1_ATCM (rwx)   : ORIGIN = 0x41400000, LENGTH = 0x8000
    MCU0_R5F1_BTCM (rwx)   : ORIGIN = 0x41410000, LENGTH = 0x8000

    /* MCU0 share locations */
    OCMRAM  (rwx)          : ORIGIN = 0x41C00100, LENGTH = 0x80000 - 0x1100      /* ~510KB */

    /* j721e MCMS3 locations */
    /* j721e Reserved Memory for ARM Trusted Firmware */
    MSMC3_ARM_FW   (rwx)   : ORIGIN = 0x70000000, LENGTH = 0x20000         /* 128KB */
    MSMC3   (rwx)          : ORIGIN = 0x70020000, LENGTH = 0x7D0000        /* 8MB - 192KB */
    /* j721e Reserved Memory for DMSC Firmware */
    MSMC3_DMSC_FW  (rwx)   : ORIGIN = 0x707F0000, LENGTH = 0x10000         /* 64KB */

    DDR0_RESERVED    (rwx)  	: ORIGIN = 0x80000000, LENGTH = 0x20000000  	/* 512MB */

	/* Memory for R5 R5FSS0_CORE0, aka MCU2_0, aka r5f0_0 */
    MCU2_0_IPC_DATA (rwx)	: ORIGIN = __MCU2_0_ALLOCATED_START,   LENGTH = 0x00100000	/*   1MB */
    MCU2_0_EXT_DATA  (rwx)	: ORIGIN = __MCU2_0_EXT_DATA_BASE,     LENGTH = 0x00100000	/*   1MB */
    MCU2_0_R5F_MEM_TEXT (rwx)	: ORIGIN = __MCU2_0_R5F_MEM_TEXT_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU2_0_R5F_MEM_DATA (rwx)	: ORIGIN = __MCU2_0_R5F_MEM_DATA_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU2_0_DDR_SPACE (rwx)	: ORIGIN = __MCU2_0_DDR_SPACE_BASE,    LENGTH = 0x00C00000	/*  12MB */

	/* Memory for the other R5 cores */
    MCU2_1_IPC_DATA (rwx)	: ORIGIN = __MCU2_1_ALLOCATED_START,   LENGTH = 0x00100000	/*   1MB */
    MCU2_1_EXT_DATA  (rwx)	: ORIGIN = __MCU2_1_EXT_DATA_BASE,     LENGTH = 0x00100000	/*   1MB */
    MCU2_1_R5F_MEM_TEXT (rwx)	: ORIGIN = __MCU2_1_R5F_MEM_TEXT_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU2_1_R5F_MEM_DATA (rwx)	: ORIGIN = __MCU2_1_R5F_MEM_DATA_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU2_1_DDR_SPACE (rwx)	: ORIGIN = __MCU2_1_DDR_SPACE_BASE,    LENGTH = 0x00C00000	/*  12MB */

    MCU3_0_IPC_DATA (rwx)	: ORIGIN = __MCU3_0_ALLOCATED_START,   LENGTH = 0x00100000	/*   1MB */
    MCU3_0_EXT_DATA  (rwx)	: ORIGIN = __MCU3_0_EXT_DATA_BASE,     LENGTH = 0x00100000	/*   1MB */
    MCU3_0_R5F_MEM_TEXT (rwx)	: ORIGIN = __MCU3_0_R5F_MEM_TEXT_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU3_0_R5F_MEM_DATA (rwx)	: ORIGIN = __MCU3_0_R5F_MEM_DATA_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU3_0_DDR_SPACE (rwx)	: ORIGIN = __MCU3_0_DDR_SPACE_BASE,    LENGTH = 0x00C00000	/*  12MB */

    MCU3_1_IPC_DATA (rwx)	: ORIGIN = __MCU3_1_ALLOCATED_START,   LENGTH = 0x00100000	/*   1MB */
    MCU3_1_EXT_DATA  (rwx)	: ORIGIN = __MCU3_1_EXT_DATA_BASE,     LENGTH = 0x00100000	/*   1MB */
    MCU3_1_R5F_MEM_TEXT (rwx)	: ORIGIN = __MCU3_1_R5F_MEM_TEXT_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU3_1_R5F_MEM_DATA (rwx)	: ORIGIN = __MCU3_1_R5F_MEM_DATA_BASE, LENGTH = 0x00100000	/*   1MB */
    MCU3_1_DDR_SPACE (rwx)	: ORIGIN = __MCU3_1_DDR_SPACE_BASE,    LENGTH = 0x00C00000	/*  12MB */

	DDR0_SHARED_r5f0_0 (rwx) : ORIGIN = 0x90000000, LENGTH = 0x1000000 /* 16MB */
}


/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 * 
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY("vectors")
SECTIONS
{	
	/* __shared_mem_base = ORIGIN(DDR0_SHARED_r5f0_0); */

	/* The TI libraries will not link without *(.ARM.exidx*) discarded */
	/DISCARD/ : { 
		*(.ARM.exidx*)
	}

	.rstvectors : {} > RESET_VECTORS

	.boot :
	{
		KEEP(*(.bootCode))
 		KEEP(*(.startupCode))
		*(.boot*)
		*(.text.hwi*)
		*(.text.cache*)
		*(.text.mpu*)
		*(.text.boot*)
	} > MCU0_R5F_TCMA

	
	
	/* .benchmark_buffer (NOLOAD) {} ALIGN (8) > MCU2_0_DDR_SPACE */
    ipc_data_buffer : ALIGN(128)
	{
		
	} > MCU2_0_DDR_SPACE


	.resource_table :
	{
		__RESOURCE_TABLE = .;
		*(.resource_table*)
	} > MCU2_0_EXT_DATA

	.tracebuf :
	{
		*(.tracebuf*)
	} > MCU2_0_EXT_DATA

	/* How to lay out the .text section */
	.text : ALIGN (32)
	{
		/* Place code related to hardware interrupts (may be redundant) */
		/**(.text.hwi*)*/

		/*Include all sections from all files that start with .text */
		*(.text*)
		/* *(EXCLUDE_FILE(*.text.hwi*) .text*) */

		/* Keep the code the runs before main() */
		KEEP(*(.init))

		/* Keep the code that runs after main() */
		KEEP(*(.fini))
		
		/* Place the read-only data, const variables, string literals, so on */
		*(.rodata*)

		/* Keep all sections that start with .eh_frame. The sections are used
		 * for C++ exception handling, and by debuggers for C
		*/
		KEEP(*(.eh_frame*))

		/* Align the end of the section to a 32-byte boundary */
		. = ALIGN(32);
	} > MCU2_0_R5F_MEM_TEXT

	/* Location counter can end up 2byte aligned with narrow Thumb code but
	   __etext is assumed by startup code to be the LMA of a section in RAM
	   which must be 4byte aligned */
	__etext = ALIGN (4);

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);


		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > MCU2_0_R5F_MEM_TEXT /* REALLY SHOULD BE PLACED IN MCU2_0_R5F_MEM_DATA, crashes to PABt handler if placed in  MCU2_0_R5F_MEM_DATA */

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > MCU2_0_R5F_MEM_DATA /* Might need to be placed in MCU2_0_R5F_MEM_TEXT or things might break in the future */

	/* This section matters for arm exception handling */
	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > MCU2_0_DDR_SPACE
	__exidx_end = .;

	/* Probably helps with debugging */
	.ARM.extab : 
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > MCU2_0_DDR_SPACE

	/*
	 *  Some random stacks used for error handlers
	*/
	.irqStack (NOLOAD) : 
	{
		__IRQ_STACK_START = .;
		. = ALIGN(4);
		. = . + __IRQ_STACK_SIZE;
		__IRQ_STACK_END = .;
	} > MCU2_0_DDR_SPACE


	.fiqStack (NOLOAD) : 
	{
		__FIQ_STACK_START = .;
		. = ALIGN(4);
		. = . + __FIQ_STACK_SIZE;
		__FIQ_STACK_END = .;
	} > MCU2_0_DDR_SPACE

	.abortStack (NOLOAD) : 
	{
		__ABORT_STACK_START = .;
		. = ALIGN(4);
		. = . + __ABORT_STACK_SIZE;
		__ABORT_STACK_END = .;
	} > MCU2_0_DDR_SPACE

	.undStack (NOLOAD) : 
	{
		__UND_STACK_START = .;
		. = ALIGN(4);
		. = . + __UND_STACK_SIZE;
		__UND_STACK_END = .;
	} > MCU2_0_DDR_SPACE

	.svcStack (NOLOAD) : 
	{
		__SVC_STACK_START = .;
		. = ALIGN(4);
		. = . + __SVC_STACK_SIZE;
		__SVC_STACK_END = .;
	} > MCU2_0_DDR_SPACE	
	

	/*** OLD STACK AND HEAP SETUP (AUTOMATIC SIZING STYLE)***/
	/* .heap (COPY):
	{
		__end__ = .;
		PROVIDE(end = .);
		*(.heap*)
		__HeapLimit = .;
	} > MCU2_0_DDR_SPACE */

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	/* .stack_dummy (COPY):
	{
		*(.stack*)
	} > MCU2_0_DDR_SPACE */


	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	/* __StackTop = ORIGIN(MCU2_0_DDR_SPACE) + LENGTH(MCU2_0_DDR_SPACE);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop); */
	
	/* Check if data + heap + stack exceeds RAM limit */
	/* ASSERT(__StackLimit >= __HeapLimit, "region TCMA overflowed with stack") */


	/*** NEW STACK AND HEAP SETUP (MANUAL SIZING STYLE) ***/
    /* Adjusted sections for heap and stack */
    . = ALIGN(4);
    .guard_region_heap (NOLOAD):
    {
        . += __guard_size; /* Skip over guard region */
    } > MCU2_0_DDR_SPACE

    .heap :
    {
        . = ALIGN(4);
		__end__ = .;
		PROVIDE(end = .);
        __heap_base = .;
        . += __heap_size; /* Allocate space for heap */
        __heap_limit = .;
    } > MCU2_0_DDR_SPACE

    .guard_region_stack (NOLOAD):
    {
        . += __guard_size; /* Skip over guard region */
    } > MCU2_0_DDR_SPACE

    .stack (NOLOAD):
    {
        . = ALIGN(8);
        __stack_limit = .; /* This might need to align with __stack if required by your startup code */
        . += __stack_size; /* Allocate space for stack */
        __stack_top = .; /* Ensure this or its equivalent is recognized by your startup code */
    } > MCU2_0_DDR_SPACE

    /* Correct __stack definition to align with your startup code expectations */
    PROVIDE(__stack = __stack_top);
    ASSERT(__heap_limit < __stack_limit, "Heap and stack collision")

	__shared_mem_base = ORIGIN(DDR0_SHARED_r5f0_0);
}

