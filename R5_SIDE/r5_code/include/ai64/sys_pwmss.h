/*
 * Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the
 *	  distribution.
 *
 *	* Neither the name of Texas Instruments Incorporated nor the names of
 *	  its contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_PWMSS_H_
#define _SYS_PWMSS_H_
#include <stdint.h>
// TBCTL (Time-Base Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// TBCNT MODE bits
#define TB_COUNT_UP      0x0
#define TB_COUNT_DOWN    0x1
#define TB_COUNT_UPDOWN  0x2
#define TB_FREEZE        0x3
// PHSEN bit
#define TB_DISABLE       0x0
#define TB_ENABLE        0x1
// PRDLD bit
#define TB_SHADOW        0x0
#define TB_IMMEDIATE     0x1
// SYNCOSEL bits
#define TB_SYNC_IN       0x0
#define TB_CTR_ZERO      0x1
#define TB_CTR_CMPB      0x2
#define TB_SYNC_DISABLE  0x3
// HSPCLKDIV and CLKDIV bits
#define TB_DIV1          0x0
#define TB_DIV2          0x1
#define TB_DIV4          0x2
// PHSDIR bit
#define TB_DOWN          0x0
#define TB_UP            0x1
// FREE_SOFT bit
#define TB_STOP_NEXT     0x0
#define TB_STOP_COMPLETE 0x1
#define TB_FREE_RUN      0x2
#define TB_FREE          0x3
// CMPCTL (Compare Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// LOADAMODE and LOADBMODE bits
#define CC_CTR_ZERO     0x0
#define CC_CTR_PRD      0x1
#define CC_CTR_ZERO_PRD 0x2
#define CC_LD_DISABLE   0x3
// SHDWAMODE and SHDWBMODE bits
#define CC_SHADOW       0x0
#define CC_IMMEDIATE    0x1
// AQCTLA and AQCTLB (Action-qualifier Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// ZRO, PRD, CAU, CAD, CBU, CBD bits
#define AQ_NO_ACTION    0x0
#define AQ_CLEAR        0x1
#define AQ_SET          0x2
#define AQ_TOGGLE       0x3
// DBCTL (Dead-Band Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// MODE bits
#define DB_DISABLE     0x0
#define DBA_ENABLE     0x1
#define DBB_ENABLE     0x2
#define DB_FULL_ENABLE 0x3
// POLSEL bits
#define DB_ACTV_HI     0x0
#define DB_ACTV_LOC    0x1
#define DB_ACTV_HIC    0x2
#define DB_ACTV_LO     0x3
// PCCTL (chopper control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// CHPEN bit
#define CHP_ENABLE     0x0
#define CHP_DISABLE    0x1
// CHPFREQ bits
#define CHP_DIV1       0x0
#define CHP_DIV2       0x1
#define CHP_DIV3       0x2
#define CHP_DIV4       0x3
#define CHP_DIV5       0x4
#define CHP_DIV6       0x5
#define CHP_DIV7       0x6
#define CHP_DIV8       0x7
// CHPDUTY bits
#define CHP1_8TH       0x0
#define CHP2_8TH       0x1
#define CHP3_8TH       0x2
#define CHP4_8TH       0x3
#define CHP5_8TH       0x4
#define CHP6_8TH       0x5
#define CHP7_8TH       0x6
// TZSEL (Trip-zone Select)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// CBCn and OSHTn bits
#define TZ_SEL_ENABLE   0x1
#define TZ_SEL_DISABLE  0x0
// TZCTL (Trip-zone Control)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// TZA and TZB bits
#define TZ_HIZ        0x0
#define TZ_FORCE_HI   0x1
#define TZ_FORCE_LO   0x2
#define TZ_DISABLE    0x3
// ETSEL (Event-trigger Select)
// = = = = = = = = = = = = = = = = = = = = = = = = = =
// INTSEL, SOCASEL, SOCBSEL bits
#define ET_CTR_ZERO   0x1
#define ET_CTR_PRD    0x2
#define ET_CTRU_CMPA  0x4
#define ET_CTRD_CMPA  0x5
#define ET_CTRU_CMPB  0x6
#define ET_CTRD_CMPB  0x7
// ETPS (Event-trigger Prescale)= = = = = = = = = = = = = = = = = = = = = = =
// INTPRD, SOCAPRD, SOCBPRD bits
#define ET_DISABLE    0x0
#define ET_ENABLE     0x1
#define ET_1ST        0x1
#define ET_2ND        0x2
#define ET_3RD        0x3

/* SYS PWMSS register set */
typedef struct {

	/* SYS_PWMSS_TBCTL register bit field */
    union {
        volatile uint16_t TBCTL; // 0x0
        volatile struct {
            uint16_t CTRMODE : 2; // 1:0
            uint16_t PHSEN : 1; // 2
            uint16_t PRDLD : 1; // 3
            uint16_t SYNCOSEL : 2; // 5:4
            uint16_t SWFSYNC : 1; // 6
            uint16_t HSPCLKDIV : 3; // 9:7
            uint16_t CLKDIV : 3; // 12:10
            uint16_t PHSDIR : 1; // 13
            uint16_t FREE_SOFT : 2; // 15:14
        } TBCTL_bit; // Reset 0x0083
    };
	/* SYS_PWMSS_TBSTS register bit field */
    union {
        volatile uint16_t TBSTS; // 0x2
        volatile struct {
            uint16_t CTRDIR : 1; // 0
            uint16_t SYNCI : 1; // 1
            uint16_t CTRMAX : 1; // 2
            uint16_t rsvd01: 13; // 15:3
        } TBSTS_bit; // Reset 0x0000
    };
	/* SYS_PWMSS_TBPHSHR register bit field */
	volatile uint16_t TBPHSHR; // 0x4 Reset 0x0000
	/* SYS_PWMSS_TBPHS register bit field */
	volatile uint16_t TBPHS; // 0x6 Reset 0x0000
	/* SYS_PWMSS_TBCNT register bit field */
	volatile uint16_t TBCNT; // 0x8 Reset 0x0000
	/* SYS_PWMSS_TBPRD register bit field */
	volatile uint16_t TBPRD; // 0xa Reset 0x0000
    uint8_t rsvda[2]; // 0xc - 0xd
    union {
	    volatile uint16_t CMPCTL; // 0xe
        volatile struct {
            uint16_t LOADAMODE : 2; // 1:0
            uint16_t LOADBMODE : 2; // 3:2
            uint16_t SHDWAMODE : 1; // 4
            uint16_t rsvd0 : 1; // 5
            uint16_t SHDWBMODE : 1; // 6
            uint16_t rsvd10 : 1; // 7
            uint16_t SHDWAFULL : 1; // 8
            uint16_t SHDWBFULL : 1; // 9
            uint16_t rsvd11 : 6; // 15:10
         }CMPCTL_bit; // Reset 0x0000
    };
	/* SYS_PWMSS_CMPAHR register bit field */
	volatile uint16_t CMPAHR; // 0x10 Reset 0x0000
	/* SYS_PWMSS_CMPA register bit field */
	volatile uint16_t CMPA; // 0x12 Reset 0x0000
	/* SYS_PWMSS_CMPB register bit field */
	volatile uint16_t CMPB; // 0x14 Reset 0x0000
	/* SYS_PWMSS_AQCTLA register bit field */
    union {
        volatile uint16_t AQCTLA; // 0x16
        volatile struct {
            uint16_t ZRO : 2; // 1:0
            uint16_t PRD : 2; // 3:2
            uint16_t CAU : 2; // 5:4
            uint16_t CAD : 2; // 7:6
            uint16_t CBU : 2; // 9:8
            uint16_t CBD : 2; // 11:10
            uint16_t rsvd20 : 4; // 15:12
         }AQCTLA_bit; // Reset 0x0000
    };
	/* SYS_PWMSS_AQCTLB register bit field */
    union {
        volatile uint16_t AQCTLB; // 0x18
        volatile struct {
            uint16_t ZRO : 2; // 1:0
            uint16_t PRD : 2; // 3:2
            uint16_t CAU : 2; // 5:4
            uint16_t CAD : 2; // 7:6
            uint16_t CBU : 2; // 9:8
            uint16_t CBD : 2; // 11:10
            uint16_t rsvd20 : 4; // 15:12
         }AQCTLB_bit;
    };
	/* SYS_PWMSS_AQSFRC register bit field */
    union {
        volatile uint16_t AQSFRC; // 0x1a
        volatile struct {
            uint16_t ACTSFA : 2; // 1:0
            uint16_t OTSFA : 1; // 2
            uint16_t ACTSFB : 2; // 4:3
            uint16_t OTSFB : 1; // 5
            uint16_t RLDCSF : 2; // 7:6
            uint16_t rsvd30 : 8; // 15:8
         }AQSFRC_bit;
    };
	/* SYS_PWMSS_AQCSFRC register bit field */
    union {
        volatile uint16_t AQCSFRC; // 0x1c
        volatile struct {
            uint16_t CSFA : 2; // 1:0
            uint16_t CSFB : 2; // 3:2
            uint16_t rsvd30 : 12; // 15:4
        }AQCSFRC_bit;
    };
	/* SYS_PWMSS_DBCTL register bit field */
    union {
        volatile uint16_t DBCTL; // 0x1e
        volatile struct {
            uint16_t OUT_MODE : 2; // 1:0
            uint16_t POLSEL : 2; // 3:2
            uint16_t IN_MODE : 2; // 5:4
            uint16_t rsvd40 : 10; // 15:6
        }DBCTL_bit;
    };
	/* SYS_PWMSS_DBRED register bit field */
    union {
        volatile uint16_t DBRED; // 0x20
        volatile struct {
            uint16_t DEL : 10; // 9:0
            uint16_t rsvd50 : 6; // 15:10
        }DBRED_bit;
    };
	/* SYS_PWMSS_DBFED register bit field */
    union {
        volatile uint16_t DBFED; // 0x22
        volatile struct {
            uint16_t DEL : 10; // 9:0
            uint16_t rsvd50 : 6; // 15:10
        }DBFED_bit;
    };
	/* SYS_PWMSS_TZSEL register bit field */
    union {
        volatile uint16_t TZSEL; // 0x24
        volatile struct {
            uint16_t CBC : 6; // 0:5
            uint16_t rsvd60 : 2; // 7:6
            uint16_t OSHT : 6; // 8:13
            uint16_t rsvd61 : 2; // 15:14
        }TZSEL_bit;
    };
	uint8_t rsvd70[2]; // 0x26 - 0x27
	/* SYS_PWMSS_TZCTL register bit field */
    union {
        volatile uint16_t TZCTL; // 0x28
        volatile struct {
            uint16_t TZA : 2; // 1:0
            uint16_t TZB : 2; // 3:2
            uint16_t rsvd71 : 12; // 15:4
        }TZCTL_bit;
    };
	/* SYS_PWMSS_TZEINT register bit field */
    union {
        volatile uint16_t TZEINT; // 0x2a
        volatile struct {
            uint16_t rsvd80 : 1; // 0
            uint16_t CBC : 1; // 1
            uint16_t OST : 1; // 2
            uint16_t rsvd81 : 13; // 15:3
        }TZEINT_bit;
    };
	/* SYS_PWMSS_TZFLG register bit field */
    union {
        volatile uint16_t TZFLG; // 0x2c
        volatile struct {
            uint16_t INT : 1; // 0
            uint16_t CBC : 1; // 1
            uint16_t OST : 1; // 2
            uint16_t rsvd90 : 13; // 15:3
        }TZFLG_bit;
    };
	/* SYS_PWMSS_TZCLR register bit field */
    union {
        volatile uint16_t TZCLR; // 0x2e
        volatile struct {
            uint16_t INT : 1; // 0
            uint16_t CBC : 1; // 1
            uint16_t OST : 1; // 2
            uint16_t rsvda0 : 13; // 15:3
        }TZCLR_bit;
    };
	/* SYS_PWMSS_TZFRC register bit field */
    union {
        volatile uint16_t TZFRC; // 0x30
        volatile struct {
            uint16_t rsvcb0 : 1; // 0
            uint16_t CBC : 1; // 1
            uint16_t OST : 1; // 2
            uint16_t rsvdb0 : 13; // 15:3
        }TZFRC_bit;
    };
	/* SYS_PWMSS_ETSEL register bit field */
    union {
        volatile uint16_t ETSEL; // 0x32
        volatile struct {
            uint16_t INTSEL : 3; // 2:0
            uint16_t INTEN : 1; // 3
            uint16_t rsvdc0 : 4; // 7:4
            uint16_t SOCASEL : 3; // 10:8
            uint16_t SOCA : 1; // 11
            uint16_t SOCBSEL : 3; // 14:12
            uint16_t SOCB : 1; // 15
        }ETSEL_bit;
    };
	/* SYS_PWMSS_ETPS register bit field */
    union {
        volatile uint16_t ETPS; // 0x34
        volatile struct {
            uint16_t INTPRD : 2; // 1:0
            uint16_t INTCNT : 2; // 3:2
            uint16_t rsvdd0 : 4; // 7:4
            uint16_t SOCAPRD : 2; // 9:8
            uint16_t SOCACNT : 2; // 11:10
            uint16_t SOCBPRD : 2; // 13:12
            uint16_t SOCBCNT : 2; // 15:14
        }ETPS_bit;
    };
	/* SYS_PWMSS_ETFLG register bit field */
    union {
        volatile uint16_t ETFLG; // 0x36
        volatile struct {
            uint16_t INT : 1; // 0
            uint16_t rsvde0 : 1; // 1
            uint16_t SOCA : 1; // 2
            uint16_t SOCB : 1; // 3
            uint16_t rsvde1 : 12; // 15:4
        }ETFLG_bit;
    };
	/* SYS_PWMSS_ETCLR register bit field */
    union {
        volatile uint16_t ETCLR; // 0x38
        volatile struct {
            uint16_t INT : 1; // 0
            uint16_t rsvdf0 : 14; // 15:1
        }ETCLR_bit;
    };
	/* SYS_PWMSS_ETFRC register bit field */
    union {
        volatile uint16_t ETFRC; // 0x3a
        volatile struct {
            uint16_t INT : 1; // 0
            uint16_t rsvd10 : 1; // 1
            uint16_t SOCA : 1; // 2
            uint16_t SOCB : 1; // 3
            uint16_t rsvd11 : 12; // 15:4
        }ETFRC_bit;
    };
	/* SYS_PWMSS_PCCTL register bit field */
    union {
        volatile uint16_t PCCTL; // 0x3c
        volatile struct {
            uint16_t CHPEN : 1; // 0
            uint16_t OSHTWTH : 4; // 4:1
            uint16_t CHPFREQ : 3; // 7:5
            uint16_t CHPDUTY : 3; // 10:8
            uint16_t rsvd20 : 4; // 15:11
        }PCCTL_bit;
    };
	uint8_t rsvd3e[30]; // 0x3e - 0x5b
	/* SYS_PWMSS_PID register bit field */
	union {
		volatile uint32_t PID;

		volatile struct {
			uint32_t Y_MINOR : 6; // 5:0
			uint32_t CUSTOM : 2; // 7:6
			uint32_t X_MAJOR : 3; // 10:8
			uint32_t R_RTL : 5; // 15:11
			uint32_t FUNC : 12; // 27:16
			uint32_t rsvd30 : 2; // 29:28
			uint32_t SCHEME : 2; // 31:30
		} PID_bit;
	}; // 0x5c
} sys_pwmss;
#ifdef SOC_AM64X
    #define EPWM0 (*((volatile sys_pwmss*)0x23000000))
    #define EPWM1 (*((volatile sys_pwmss*)0x23010000))
    #define EPWM2 (*((volatile sys_pwmss*)0x23020000))
    #define EPWM3 (*((volatile sys_pwmss*)0x23030000))
    #define EPWM4 (*((volatile sys_pwmss*)0x23040000))
    #define EPWM5 (*((volatile sys_pwmss*)0x23050000))
    #define EPWM6 (*((volatile sys_pwmss*)0x23060000))
    #define EPWM7 (*((volatile sys_pwmss*)0x23070000))
    #define EPWM8 (*((volatile sys_pwmss*)0x23080000))
#endif
// Definitions are for J721e
#ifdef J721E_TDA4VM
    #define EPWM0 (*((volatile sys_pwmss*)0x03000000))
    #define EPWM1 (*((volatile sys_pwmss*)0x03010000))
    #define EPWM2 (*((volatile sys_pwmss*)0x03020000))
    #define EPWM3 (*((volatile sys_pwmss*)0x03030000))
    #define EPWM4 (*((volatile sys_pwmss*)0x03040000))
    #define EPWM5 (*((volatile sys_pwmss*)0x03050000))
    #define EPWM6 (*((volatile sys_pwmss*)0x03060000))
    #define EPWM7 (*((volatile sys_pwmss*)0x03070000))
    #define EPWM8 (*((volatile sys_pwmss*)0x03080000))
#endif
#endif /* _SYS_PWMSS_H_ */

