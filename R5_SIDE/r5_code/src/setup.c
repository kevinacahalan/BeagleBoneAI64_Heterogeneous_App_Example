#include "../include/setup.h"
#include <stdio.h>

#include <r5/kernel/dpl/our_CacheP.h>

// #include <r5/kernel/dpl/CycleCounterP.h>
#include <ti/drv/ipc/ipc.h>
#include <ti/csl/arch/csl_arch.h> // brings in some warnings about masterIsr no defined
// #include <ti/csl/arch/r5/interrupt.h>

// global structures used by MPU and cache init code
our_CacheP_Config gCacheConfig = {1, 0}; // cache on, no forced writethrough

#define OLD_MEM_CONFIG
#if defined(OLD_MEM_CONFIG)
#include <r5/kernel/dpl/our_MpuP_armv7.h>
our_MpuP_RegionConfig gMpuRegionConfig[] = {
    // position in this array MATTERS, later stuff wins....INCLUDING UNDEFINED STUFF!

    // Complete 32-bit address space
    {
        .baseAddr = 0x0u,
        .size = our_MpuP_RegionSize_4G,
        .attrs = {
            .isEnable = 1,
            .isCacheable = 0, // IO will not work if this is 1
            .isBufferable = 1, // Maybe could be 1, this is the arm B bit https://developer.arm.com/documentation/ddi0460/d/System-Control/Register-descriptions/c6--MPU-memory-region-programming-registers?lang=en
            .isSharable = 0,
            .isExecuteNever = 0, 
            .tex = 1, // 0
            .accessPerm = our_MpuP_AP_ALL_RW,
            .subregionDisableMask = 0x0u,
        },
    },
    // MSRAM region
    {
        .baseAddr = 0x70000000u,
        .size = our_MpuP_RegionSize_8M,
        .attrs = {
            .isEnable = 1,
            .isCacheable = 1, // 0
            .isBufferable = 1,
            .isSharable = 0, //1
            .isExecuteNever = 0,
            .tex = 7, // 5
            .accessPerm = our_MpuP_AP_ALL_RW,
            .subregionDisableMask = 0x0u,
        },
    },
    // DDR region
    {
        .baseAddr = 0x80000000u,
        .size = our_MpuP_RegionSize_2G,
        .attrs = {
            .isEnable = 1,
            .isCacheable = 1, // 0
            .isBufferable = 1,
            .isSharable = 0, // 1... If this is set to `1` the R5 runs very slow, like 30x slower
            .isExecuteNever = 0,
            .tex = 7, // 5
            .accessPerm = our_MpuP_AP_ALL_RW,
            .subregionDisableMask = 0x0u
        },
    },
    // rpmsg region
    {
        .baseAddr = 0xA2000000u,
        .size = our_MpuP_RegionSize_1M,
        .attrs = {
            .isEnable = 1,
            .isCacheable = 0,
            .isBufferable = 0,
            .isSharable = 1,
            .isExecuteNever = 0, // PROBABLY SHOULD BE 1
            .tex = 1,
            .accessPerm = our_MpuP_AP_ALL_RW,
            .subregionDisableMask = 0x0u
        },
    },
    // MCU2_0_R5F_MEM_TEXT and MCU2_0_R5F_MEM_DATA
    {
        .baseAddr = 0xA2200000u,
        .size = our_MpuP_RegionSize_2M,
        .attrs = {
            .isEnable = 1,
            .isCacheable = 1,
            .isBufferable = 1, // Maybe could be 1, this is the arm B bit https://developer.arm.com/documentation/ddi0460/d/System-Control/Register-descriptions/c6--MPU-memory-region-programming-registers?lang=en
            .isSharable = 0,
            .isExecuteNever = 0, 
            .tex = 7,
            .accessPerm = our_MpuP_AP_ALL_RW,
            .subregionDisableMask = 0x0u,
        },
    },
    // r5f-linux shared memory region
    {
        .baseAddr = 0x90000000u,
        .size = our_MpuP_RegionSize_16M,
        .attrs = {
            .isEnable = 1,
            .isCacheable = 0,
            .isBufferable = 0,
            .isSharable = 1,
            .isExecuteNever = 1,
            .tex = 1,
            .accessPerm = our_MpuP_AP_ALL_RW,
            .subregionDisableMask = 0x0u
        },
    },
};

our_MpuP_Config gMpuConfig = {sizeof gMpuRegionConfig / sizeof gMpuRegionConfig[0], 1, 1}; // regions, background region on, MPU on

#else
#endif

// Replaces weak definition from ti/csl/arch/r5/src/interrupt_register.c
// void Intc_IntRegister(uint16_t intrNum, IntrFuncPtr fptr, void *fun_arg)
// {
//     printf("Attempted, Intc_IntRegister with intrNum = %d, IntrFuncPtr = %p, fun_arg = %p\n", intrNum, fptr, fun_arg);
// }

// // Replaces weak definition from ti/csl/arch/r5/src/interrupt_register.c
// void Intc_IntUnregister(uint16_t intrNum)
// {
//     printf("Attempted Intc_IntUnregister with intrNum = %d\n", intrNum);
// }


// External declaration of TI's handlers (discovered from symbol table)
// ti/csl/arch/r5/src/interrupt_handlers.c
extern void masterIsr_c(void);
extern void undefInstructionExptnHandler_c(void);
extern void swIntrExptnHandler_c(void);
extern void prefetchAbortExptnHandler_c(void);
extern void dataAbortExptnHandler_c(void);
extern void irqExptnHandler_c(void);
extern void fiqExptnHandler_c(void);


#include <stdint.h>

void MyMasterIsrWrapper(void)
{
    // printf("Entering my custom MasterIsr wrapper.\n");
    // Call TI’s provided handler
    masterIsr_c();
    // printf("Exiting my custom MasterIsr wrapper.\n");
}


void masterIsr(void) __attribute__((naked, section(".text.hwi"), target("arm")));
void masterIsr(void)
{
    asm volatile(
        // Adjust LR to point to the interrupted instruction
        "SUB     lr, lr, #4           \n"

        // Push return address and SPSR
        "PUSH    {lr}                 \n"
        "MRS     lr, SPSR             \n"
        "PUSH    {lr}                 \n"

        // Save used registers
        "PUSH    {r0-r4, r12}         \n"

        // Save floating point context
        "FMRX    r0, FPSCR            \n"
        "VPUSH   {d0-d15}             \n"
        "PUSH    {r0}                 \n"

        // Align stack pointer to 8-byte boundary
        "MOV     r2, sp               \n"
        "AND     r2, r2, #4           \n"
        "SUB     sp, sp, r2           \n"

        // Call the interrupt handler (masterIsr_c)
        "PUSH    {r0-r4, lr}          \n"
        "LDR     r1, =MyMasterIsrWrapper\n"
        "BLX     r1                   \n"
        "POP     {r0-r4, lr}          \n"
        "ADD     sp, sp, r2           \n"

        // Disable IRQ to prevent handling new interrupts during restore
        "CPSID   i                    \n"
        "DSB                          \n"
        "ISB                          \n"

        // Restore floating point context
        "POP     {r0}                 \n"
        "VPOP    {d0-d15}             \n"
        "VMSR    FPSCR, r0            \n"

        // Restore used registers and return state
        "POP     {r0-r4, r12}         \n"
        "POP     {lr}                 \n"
        "MSR     SPSR_cxsf, lr        \n"
        "POP     {lr}                 \n"
        "MOVS    pc, lr               \n"
    );
}



void MyUndefHandlerWrapper(void)
{
    printf("Entering my custom Undef handler wrapper.\n");
    // Call TI’s provided handler
    undefInstructionExptnHandler_c();
    printf("Exiting my custom Undef handler wrapper.\n");
}

// The naked attribute prevents the compiler from generating any prologue/epilogue code.
void Undef_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));
void Undef_Handler(void)
{
    // Inline assembly to:
    // 1. Save registers
    // 2. Adjust LR to point back to the interrupted instruction
    // 3. Save SPSR and LR to stack
    // 4. Call our wrapper C handler
    // 5. Restore registers and return from exception
    asm volatile(
        // Save caller-saved registers + r12 on stack
        "PUSH    {r0-r4, r12}          \n"
        // Adjust LR so that we return to the exact instruction that was interrupted
        "SUB     lr, lr, #4            \n"
        // Save the return address (LR) to stack
        "PUSH    {lr}                  \n"
        // Move SPSR into LR and push it onto stack
        "MRS     lr, SPSR              \n"
        "PUSH    {lr}                  \n"
        // Load the address of our wrapper function into r1
        "LDR     r1, =MyUndefHandlerWrapper \n"
        // Branch and link to the wrapper (in ARM mode)
        "BLX     r1                    \n"
        // Pop SPSR and restore it
        "POP     {lr}                  \n"
        "MSR     SPSR_cxsf, lr         \n"
        // Pop LR
        "POP     {lr}                  \n"
        // Restore registers
        "POP     {r0-r4, r12}          \n"
        // Return from exception using MOVS pc, lr.
        // This ensures the processor mode and state are restored.
        "MOVS    pc, lr                \n"
    );
}


void MySVCHandlerWrapper(void)
{
    printf("Entering my custom SVC handler wrapper.\n");
    // Call TI’s provided handler
    swIntrExptnHandler_c();
    printf("Exiting my custom SVC handler wrapper.\n");
}

// The naked attribute prevents the compiler from generating any prologue/epilogue code.
void SVC_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));
void SVC_Handler(void)
{
    // Inline assembly to:
    // 1. Save registers
    // 2. Adjust LR to point back to the interrupted instruction
    // 3. Save SPSR and LR to stack
    // 4. Call our wrapper C handler
    // 5. Restore registers and return from exception
    asm volatile(
        // Save caller-saved registers + r12 on stack
        "PUSH    {r0-r4, r12}          \n"
        // Adjust LR so that we return to the exact instruction that was interrupted
        "SUB     lr, lr, #4            \n"
        // Save the return address (LR) to stack
        "PUSH    {lr}                  \n"
        // Move SPSR into LR and push it onto stack
        "MRS     lr, SPSR              \n"
        "PUSH    {lr}                  \n"
        // Load the address of our wrapper function into r1
        "LDR     r1, =MySVCHandlerWrapper \n"
        // Branch and link to the wrapper (in ARM mode)
        "BLX     r1                    \n"
        // Pop SPSR and restore it
        "POP     {lr}                  \n"
        "MSR     SPSR_cxsf, lr         \n"
        // Pop LR
        "POP     {lr}                  \n"
        // Restore registers
        "POP     {r0-r4, r12}          \n"
        // Return from exception using MOVS pc, lr.
        // This ensures the processor mode and state are restored.
        "MOVS    pc, lr                \n"
    );
}


void MyPAbtHandlerWrapper(void)
{
    printf("Entering my custom PAbt handler wrapper.\n");
    // Call TI’s provided handler
    prefetchAbortExptnHandler_c();
    printf("Exiting my custom PAbt handler wrapper.\n");
}

// The naked attribute prevents the compiler from generating any prologue/epilogue code.
void PAbt_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));
void PAbt_Handler(void)
{
    // Inline assembly to:
    // 1. Save registers
    // 2. Adjust LR to point back to the interrupted instruction
    // 3. Save SPSR and LR to stack
    // 4. Call our wrapper C handler
    // 5. Restore registers and return from exception
    asm volatile(
        // Save caller-saved registers + r12 on stack
        "PUSH    {r0-r4, r12}          \n"
        // Adjust LR so that we return to the exact instruction that was interrupted
        "SUB     lr, lr, #4            \n"
        // Save the return address (LR) to stack
        "PUSH    {lr}                  \n"
        // Move SPSR into LR and push it onto stack
        "MRS     lr, SPSR              \n"
        "PUSH    {lr}                  \n"
        // Load the address of our wrapper function into r1
        "LDR     r1, =MyPAbtHandlerWrapper \n"
        // Branch and link to the wrapper (in ARM mode)
        "BLX     r1                    \n"
        // Pop SPSR and restore it
        "POP     {lr}                  \n"
        "MSR     SPSR_cxsf, lr         \n"
        // Pop LR
        "POP     {lr}                  \n"
        // Restore registers
        "POP     {r0-r4, r12}          \n"
        // Return from exception using MOVS pc, lr.
        // This ensures the processor mode and state are restored.
        "MOVS    pc, lr                \n"
    );
}


void MyDAbtHandlerWrapper(void)
{
    printf("Entering my custom DAbt handler wrapper.\n");
    // Call TI’s provided handler
    dataAbortExptnHandler_c();
    printf("Exiting my custom DAbt handler wrapper.\n");
}

// The naked attribute prevents the compiler from generating any prologue/epilogue code.
void DAbt_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));
void DAbt_Handler(void)
{
    asm volatile(
        // Check Thumb state from SPSR
        "MRS    r0, SPSR              \n"
        "AND    r1, r0, #0x20         \n" // Check T bit (bit 5) in SPSR
        "CMP    r1, #0                \n"
        "BEQ    1f                    \n" // If T bit = 1 (Thumb mode), branch
        "SUB    lr, lr, #2            \n" // Thumb: subtract 2 from LR
        "1:                            \n"
        "SUB    lr, lr, #4            \n" // ARM: subtract 4 from LR

        // Push used registers
        "PUSH   {r0-r4, r12}          \n"

        // Push the return address and SPSR
        "PUSH   {lr}                  \n"
        "MRS    lr, SPSR              \n"
        "PUSH   {lr}                  \n"

        // Call the data abort handler (C function)
        "LDR    r1, =MyDAbtHandlerWrapper\n"
        "BLX    r1                    \n"

        // Restore LR and SPSR
        "POP    {lr}                  \n"
        "MSR    SPSR_cxsf, lr         \n"
        "POP    {lr}                  \n"

        // Restore registers
        "POP    {r0-r4, r12}          \n"

        // Return from exception
        "MOVS   pc, lr                \n"
    );
}

void MyIRQHandlerWrapper(void)
{
    printf("Entering my custom IRQ handler wrapper.\n");
    // Call TI’s provided handler
    irqExptnHandler_c();
    printf("Exiting my custom IRQ handler wrapper.\n");
}

// The naked attribute prevents the compiler from generating any prologue/epilogue code.
void IRQ_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));
void IRQ_Handler(void)
{
    // Inline assembly to:
    // 1. Save registers
    // 2. Adjust LR to point back to the interrupted instruction
    // 3. Save SPSR and LR to stack
    // 4. Call our wrapper C handler
    // 5. Restore registers and return from exception
    asm volatile(
        // Save caller-saved registers + r12 on stack
        "PUSH    {r0-r4, r12}          \n"
        // Adjust LR so that we return to the exact instruction that was interrupted
        "SUB     lr, lr, #4            \n"
        // Save the return address (LR) to stack
        "PUSH    {lr}                  \n"
        // Move SPSR into LR and push it onto stack
        "MRS     lr, SPSR              \n"
        "PUSH    {lr}                  \n"
        // Load the address of our wrapper function into r1
        "LDR     r1, =MyIRQHandlerWrapper \n"
        // Branch and link to the wrapper (in ARM mode)
        "BLX     r1                    \n"
        // Pop SPSR and restore it
        "POP     {lr}                  \n"
        "MSR     SPSR_cxsf, lr         \n"
        // Pop LR
        "POP     {lr}                  \n"
        // Restore registers
        "POP     {r0-r4, r12}          \n"
        // Return from exception using MOVS pc, lr.
        // This ensures the processor mode and state are restored.
        "MOVS    pc, lr                \n"
    );
}

// Our custom wrapper function.
// This function is a place where you can do additional logging, instrumentation, etc.
// Right now, it just calls the TI handler.
void MyFIQHandlerWrapper(void)
{
    printf("Entering my custom FIQ handler wrapper.\n");
    // Call TI’s provided handler
    fiqExptnHandler_c();
    printf("Exiting my custom FIQ handler wrapper.\n");
}

// The naked attribute prevents the compiler from generating any prologue/epilogue code.
// The target("arm") ensures we generate ARM mode instructions.
void FIQ_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));
void FIQ_Handler(void)
{
    // Inline assembly to:
    // 1. Save registers
    // 2. Adjust LR to point back to the interrupted instruction
    // 3. Save SPSR and LR to stack
    // 4. Call our wrapper C handler
    // 5. Restore registers and return from exception
    asm volatile(
        // Save caller-saved registers + r12 on stack
        "PUSH    {r0-r4, r12}          \n"
        // Adjust LR so that we return to the exact instruction that was interrupted
        "SUB     lr, lr, #4            \n"
        // Save the return address (LR) to stack
        "PUSH    {lr}                  \n"
        // Move SPSR into LR and push it onto stack
        "MRS     lr, SPSR              \n"
        "PUSH    {lr}                  \n"
        // Load the address of our wrapper function into r1
        "LDR     r1, =MyFIQHandlerWrapper \n"
        // Branch and link to the wrapper (in ARM mode)
        "BLX     r1                    \n"
        // Pop SPSR and restore it
        "POP     {lr}                  \n"
        "MSR     SPSR_cxsf, lr         \n"
        // Pop LR
        "POP     {lr}                  \n"
        // Restore registers
        "POP     {r0-r4, r12}          \n"
        // Return from exception using MOVS pc, lr.
        // This ensures the processor mode and state are restored.
        "MOVS    pc, lr                \n"
    );
}


// NOTE: R5FSS defaults to ARM at reset so these must all be ARM instead of Thumb

void Reset_Handler(void) __attribute__((naked, section(".boot.reset"), target("arm")));
// void Default_Handler(void) __attribute__((naked, section(".boot.handler"), target("arm")));

// void Undef_Handler(void) __attribute__((naked));
// void SVC_Handler(void) __attribute__((naked));
// void PAbt_Handler(void) __attribute__((naked));
// void DAbt_Handler(void) __attribute__((naked));
// void IRQ_Handler(void) __attribute__((naked));
// void FIQ_Handler(void) __attribute__((naked));


__attribute__((naked, section(".rstvectors"), target("arm"))) void vectors()
{
    asm volatile(
        "LDR    PC, =Reset_Handler                        \n"
        "LDR    PC, =Undef_Handler                        \n"
        "LDR    PC, =SVC_Handler                          \n"
        "LDR    PC, =PAbt_Handler                         \n"
        "LDR    PC, =DAbt_Handler                         \n"
        "NOP                                              \n"
        "LDR    PC, =IRQ_Handler                          \n"
        "LDR    PC, =FIQ_Handler                          \n");
}

// newlib startup code
extern void _start();

void Reset_Handler()
{
    asm volatile(
        // initialize stack
        "ldr sp, =__stack \n"

        // disable interrupts
        "mrs r0, cpsr \n"
        "orr r0, r0, #0xc0 \n"
        "msr cpsr_cf, r0 \n"

        // Limit the outstanding transactions to 2... whatever that means... Issue with J721e bug PRSDK-8161?
        // "PUSH    {r8}\n"                // Save r8 register contents
        // "MRC p15, #0, r8, c1, c0, #1\n" // Read ACTRL register into r8
        // "ORR r8, r8, #(1<<13)\n"        // Set DLFO bit in ACTRL register
        // "MCR p15, #0, r8, c1, c0, #1\n" // Write back ACTRL register
        // "POP     {r8}\n"                // Restore r8 register contents
        // "BX      lr\n"


        // enable FPU
        "mrc p15,#0x0,r0,c1,c0,#2 \n"
        "mov r3,#0xf00000 \n"
        "orr r0,r0,r3 \n"
        "mcr p15,#0x0,r0,c1,c0,#2 \n"
        "mov r0,#0x40000000 \n"
        "fmxr fpexc,r0 \n");

    // must initialize MPU if code is on external memory
    our_MpuP_init(); // replace with __mpu_init from ti/csl/arch/r5/src/startup/startup.c
    //CSL_armR5MpuEnable(1);
    our_CacheP_init();

    CSL_armR5IntrEnableVic(1);  /* Enable VIC */
    /* Disable/Clear pending Interrupts in VIM before enabling CPU Interrupts */
    /* This is done to prevent serving any bogus interrupt */
    for (unsigned loopCnt = 0U ; loopCnt < R5_VIM_INTR_NUM; loopCnt++)
    {
        /* Disable interrupt in vim */
        CSL_vimSetIntrEnable((CSL_vimRegs *)(uintptr_t)CSL_MAIN_DOMAIN_VIM_BASE_ADDR, loopCnt, false);
        /* Clear interrupt status */
        CSL_vimClrIntrPending((CSL_vimRegs *)(uintptr_t)CSL_MAIN_DOMAIN_VIM_BASE_ADDR, loopCnt);
    }
    //Intc_SystemEnable(); // alternate for the 2 lines below
    CSL_armR5IntrEnableFiq(1);  /* Enable FIQ */
    CSL_armR5IntrEnableIrq(1);  /* Enable IRQ */

    Intc_Init();

    _start();
}

// void Undef_Handler(void)
// {
//     printf("Inside the Undef handler\n");
//     // Handle Undefined Instruction exception
//     while (1) {
//         // Optional: Toggle an LED or send a debug message
//     }
// }

// void SVC_Handler(void)
// {
//     //printf("Inside the SVC handler\n");
//     // Handle Supervisor Call exception
//     while (1) {
//         // Optional: Toggle an LED or send a debug message
//     }
// }

// void PAbt_Handler(void)
// {
//     //printf("Inside the PABt handler\n");
//     // Handle Prefetch Abort exception
//     while (1) {
//         // Optional: Toggle an LED or send a debug message
//     }
// }

// void DAbt_Handler(void)
// {
//     //printf("Inside the DAbt handler\n");
//     // Handle Data Abort exception
//     while (1) {
//         // Optional: Toggle an LED or send a debug message
//         // This is probably a memory issue
//     }
// }

// void IRQ_Handler(void)
// {
//     //printf("Inside IRQ handler\n");
//     // Handle IRQ exception
//     while (1) {
//         // Optional: Toggle an LED or send a debug message
//     }
// }

// void FIQ_Handler(void)
// {
//     //printf("Inside the FIQ handler\n");
//     // Handle FIQ exception
//     while (1) {
//         // Optional: Toggle an LED or send a debug message
//     }
// }

// void Default_Handler()
// {
//     //printf("Inside the default handler");
//     while (1)
//         ;
// }



// ALL THIS IPC/rpmsg stuff should probably be broken into a separate file

/*
 * Sizes of the virtqueues (expressed in number of buffers supported,
 * and must be power of 2)
 */
#define R5F_RPMSG_VQ0_SIZE      256U
#define R5F_RPMSG_VQ1_SIZE      256U

/* flip up bits whose indices represent features we support */
#define RPMSG_R5F_C0_FEATURES   1U

// Let linux pick VRING address
#define RPMSG_VRING_ADDR_ANY FW_RSC_ADDR_ANY


// Taken from ti-processor-sdk-rtos-j721e-evm-09_01_00_06/pdk_jacinto_09_01_00_22/packages/ti/drv/ipc/examples/common/src/ipc_trace.h
#define IPC_TRACE_BUFFER_MAX_SIZE     (0x80000)

// Taken from ti-processor-sdk-rtos-j721e-evm-09_01_00_06/pdk_jacinto_09_01_00_22/packages/ti/drv/ipc/examples/common/src/ipc_trace.h
char Ipc_traceBuffer[IPC_TRACE_BUFFER_MAX_SIZE] __attribute__((section(".tracebuf")));
// #define TRACEBUFADDR ((uintptr_t)&Ipc_traceBuffer)
static __attribute__((section(".tracebuf"))) uint32_t gTraceBufIndex = 0U;


const Ipc_ResourceTable ti_ipc_remoteproc_ResourceTable __attribute__((section(".resource_table"), aligned(4096))) =
{
    /* Ipc_Hdr */
    .base =
    {
        .ver = 1U, /* we're the first version that implements this */
        .num = NUM_ENTRIES, /* number of entries, MUST be 2 */
        .reserved = {
            0U,
            0U,
        } /* reserved, must be zero */
    },
    /* offsets to the entries */
    .offset =
    {
        offsetof(Ipc_ResourceTable, rpmsg_vdev),
        offsetof(Ipc_ResourceTable, trace),
    },
    /* vdev entry, Ipc_VDev*/
    .rpmsg_vdev =
    {
        .type = TYPE_VDEV,
        .id = VIRTIO_ID_RPMSG, // FIXME, maybe SHOULD BE 2 for R5F_MAIN0_0 to work with https://git.ti.com/cgit/rpmsg/ti-rpmsg-char/tree/include/rproc_id.h
        .notifyid = 0U,
        .dfeatures = RPMSG_R5F_C0_FEATURES,
        .gfeatures = 0U,
        .config_len = 0U, // config_len
        .status = 0U, // Status of VDev. It is updated by remote proc during loading
        .num_of_vrings = 2U,
        .reserved = {0U, 0U}, 
    },
    /* the two vrings, Ipc_VDevVRing*/
    .rpmsg_vring0 =
    { 
        .da = RPMSG_VRING_ADDR_ANY, 
        .align = 4096U, 
        .num = R5F_RPMSG_VQ0_SIZE, 
        .notifyid = 1U, 
        .reserved = 0U 
    },
    .rpmsg_vring1 =
    { 
        .da = RPMSG_VRING_ADDR_ANY, 
        .align = 4096U, 
        .num = R5F_RPMSG_VQ1_SIZE, 
        .notifyid = 2U, 
        .reserved = 0U
    },
    /* Ipc_Trace */
    .trace =
    {
        .type = (TRACE_INTS_VER0 | TYPE_TRACE),
        .da = (uint32_t)(uintptr_t)&Ipc_traceBuffer,
        .len = IPC_TRACE_BUFFER_MAX_SIZE,
        .reserved = 0, // reserved
        .name = "trace:r5f0",
    },
};

// It turns out this simple implementation of writing to the rpmsg trace buffer is correct and normal. There
// is no need for some fancy logic keeping track of circular buffer ends, or at least that is not normally
// done, or is not needed...?
//
// reference example functions
// ti-processor-sdk-rtos-j721e-evm-09_01_00_06/pdk_jacinto_09_01_00_22/packages/ti/drv/ipc/examples/common/src/ipc_trace.c
// https://github.com/TexasInstruments/mcupsdk-core/blob/next/source/kernel/nortos/dpl/common/DebugP_memTraceLogWriter.c
int _write(int handle, char *data, int size)
{
    (void)handle;
    int count;
    char *dst = Ipc_traceBuffer;

    for (count = 0; count < size; count++)
    {
        dst[gTraceBufIndex++] = data[count];
        if (gTraceBufIndex >= IPC_TRACE_BUFFER_MAX_SIZE)
        {
            // Wrap around
            gTraceBufIndex = 0;
        }
    }

    // Flush the cache so the host can see the data
    our_CacheP_wbInv(dst, IPC_TRACE_BUFFER_MAX_SIZE, our_CacheP_TYPE_ALL);

    return count;
}