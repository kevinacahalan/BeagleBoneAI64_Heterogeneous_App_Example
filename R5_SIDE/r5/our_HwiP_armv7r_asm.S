/*
 *  Copyright (C) 2018-2021 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


        .text

/* FUNCTION DEF: uint32_t our_HwiP_disable(void) */
        .global our_HwiP_disable
        .type our_HwiP_disable,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_disable:
        mrs     r0, cpsr
        orr     r12, r0, #0x80
        msr     cpsr_cf, r12
        bx      LR

/* FUNCTION DEF: uint32_t our_HwiP_disableFIQ(void) */
        .global our_HwiP_disableFIQ
        .type our_HwiP_disableFIQ,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_disableFIQ:
        mrs     r0, cpsr
        orr     r12, r0, #0x40
        msr     cpsr_cf, r12
        bx      LR

/* FUNCTION DEF: void our_HwiP_enable() */
        .global our_HwiP_enable
        .type our_HwiP_enable,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_enable:
        mrs     r0, cpsr
        bic     r12, r0, #0x80
        msr     cpsr_cf, r12
        bx      LR

/* FUNCTION DEF: void our_HwiP_enableFIQ() */
        .global our_HwiP_enableFIQ
        .type   our_HwiP_enableFIQ,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_enableFIQ:
        mrs     r0, cpsr
        bic     r12, r0, #0x40
        msr     cpsr_cf, r12
        bx      LR

/* FUNCTION DEF: void our_HwiP_restore(uint32_t oldIntState) */
        .global our_HwiP_restore
        .type   our_HwiP_restore,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_restore:
        msr     cpsr_cf, r0
        bx      LR

/* FUNCTION DEF: void our_HwiP_enableVIC() */
        .global our_HwiP_enableVIC
        .type   our_HwiP_enableVIC,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_enableVIC:
        MRC     p15, #0, r0, c1, c0, #0  /* Read SCTLR */
        ORR     r0, r0, #(1<<24)         /* Set VE mask (VIC controller provides handler address for IRQ) */
        MCR     p15, #0, r0, c1, c0, #0  /* Write modified SCTLR */
        bx      LR

/* FUNCTION DEF: void our_HwiP_disableVIC() */
        .global our_HwiP_disableVIC
        .type   our_HwiP_disableVIC,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_disableVIC:
        MRC     p15, #0, r0, c1, c0, #0  /* Read SCTLR */
        BIC     r0, r0, #(1<<24)         /* Clear VE mask (VIC controller provides handler address for IRQ) */
        MCR     p15, #0, r0, c1, c0, #0  /* Write modified SCTLR */
        bx      LR

/* FUNCTION DEF: void our_HwiP_getCPSR() */
        .global our_HwiP_getCPSR
        .type   our_HwiP_getCPSR,%function
        .section ".text.hwi","ax",%progbits
        .arm
        .align 2
our_HwiP_getCPSR:
        mrs     r0, cpsr
        bx      LR

	.end

