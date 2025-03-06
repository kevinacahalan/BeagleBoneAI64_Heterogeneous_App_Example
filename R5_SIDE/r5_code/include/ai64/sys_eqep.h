#ifndef _SYS_EQEP_H_
#define _SYS_EQEP_H_

#ifdef AM64X // SK-AM64B
	#define EQEP0_OFFSET              0x23200000
	#define EQEP1_OFFSET              0x23210000
	#define EQEP2_OFFSET              0x23220000
#endif
#ifdef J721E_TDA4VM // BBAI-64
	#define EQEP0_OFFSET              0x03200000
	#define EQEP1_OFFSET              0x03210000
	#define EQEP2_OFFSET              0x03220000
#endif
#ifdef AM335x
    #define EQEP0_OFFSET0             0x48300180
    #define EQEP0_OFFSET1             0x48302180
    #define EQEP0_OFFSET2             0x48304180
#endif

typedef struct
{
    union
    {    // QEP Position counter register
        volatile unsigned QPOSCNT;
        volatile struct
        {
            unsigned QPOSCNT : 32;
        } QPOSCNT_bit;
    } ; //00h - 03h - reset 44832905h
    union
    {   // QEP Position counter initialization register
        volatile unsigned QPOSINIT;
        volatile struct
        {
            unsigned INITPOS: 32;
        } QPOSINIT_bit;
    } ; //04h-07h, reset: Xh
    union
    {   // Position counter max position
        volatile unsigned QPOSMAX;
        volatile struct
        {
            unsigned QPOSMAX: 32;
        } QPOSMAX_bit;
    } ; //08h-0bh, reset: 0h
    union
    {   // Position counter compare
        volatile unsigned QPOSCMP;
        volatile struct
        {
            unsigned POSCMP: 32;
         } QPOSCMP_bit;
    }; //0ch-0fh, reset: 0h
    union
    {   // Index position latch register
        volatile unsigned QPOSILAT;
        volatile struct
        {
            unsigned IPOSILAT: 32;
        } QPOSILAT_bit;
    }; //10h-13h
    union
    {   // Strobe position latch register
        volatile unsigned QPOSSLAT;
        volatile struct
        {
            unsigned SPOSLAT: 32;
        } QPOSSLAT_bit;
    }; //14h-17h
    union
    {   // QEP position counter latch register
        volatile unsigned QPOSLAT;
        volatile struct
        {
            unsigned POSLAT: 32;
        } QPOSLAT_bit;
    }; //18h-1bh
    union
    {   // QEP unit timer register
        volatile unsigned QUTMR;
        volatile struct
        {
            unsigned UNITTMR: 32; // 0
        } QUTMR_bit;
    }; //1ch-1fh
    union
    {   // QEP unit period register
        volatile unsigned QUPRD;
        volatile struct
        {
            unsigned UNITMR: 32; // 0
        } QUPRD_bit;
    }; // 20h-23h
    union
    {   // QEP watch dog timer amd period register
        volatile unsigned QWD_TMR_PRD;
        volatile struct
        {
            unsigned QWDTMR: 16;
            unsigned QWDPRD: 16;
        } QWD_TMR_PRD_bit;
    }; // 24h-27h
    union
    {   // Quadrature decoder and QEP control register
        volatile unsigned QDEC_QEP_CTL;
        volatile struct
        {
            unsigned rsvrd: 5; // 0-4
            unsigned QSP: 1;   // 5  QSP input polarity
            unsigned QIP: 1;   // 6  QEPI input polarity
            unsigned QBP: 1;   // 7  QEPB input polarity
            unsigned QAP: 1;   // 8  QEPA input polarity
            unsigned IGATE: 1; // 9  Index pulse gating option
            unsigned SWAP: 1;  // 10 Swaps EQEPA with EQEPB
            unsigned XCR: 1;   // 11 Position counter used to measure FREQ, 2x EQEPA
            unsigned SPSEL: 1; // 12 Selects EQEP index or strobe pin
            unsigned SOEN: 1;  // 13 Position compare SYNC mode
            unsigned QSRC: 2;  // 14 Position counter source selection
            unsigned WDE: 1;   // 16 QEP watch dog enable
            unsigned UTE: 1;   // 17 QEP unit timer enable
            unsigned QCLM: 1;  // 18 EQEP capture latch mode
            unsigned QPEN: 1;  // 19 Quadrature position counter enable/software reset
            unsigned IEL: 2;   // 20 Index event latch of position counter
            unsigned SEL: 1;   // 22 Strobe event latch of position counter
            unsigned SWI: 1;   // 23 Software initialization of position counter
            unsigned IEI: 2;   // 24 Index event initialization of position counter
            unsigned SEI: 2;   // 26 Strobe event initialization  of position counter
            unsigned PCRM: 2;  // 28 Position counter reset mode
            unsigned FREE_SOFT: 2; // 30 Position counter behavior
        } QDEC_QEP_CTL_bit;
    }; // 28h-2Bh
    union
    {   // QEP capture and position compare control register
        volatile unsigned QCAP_QPOS_CTL;
        volatile struct
        {
            unsigned UPPS: 4;   // 0-3 Unit position event pre-scalar
            unsigned CCPS: 3;   // 4-6 EQEP capture timer pre-scalar
            unsigned rsrvd: 8;  // 7-14
            unsigned CEN: 1;    // 15 Enable EQEP capture
            unsigned PCSPW: 12; // 16 Select pulse width period in EQEP_FICLK cycles
            unsigned PCE: 1;    // 28 Position compare enable/disable
            unsigned PCPOL: 1;  // 29 Polarity of sync output
            unsigned PCLOAD: 1; //30 Position compare shadow load mode
            unsigned PCSHDW: 1; // 31 Position compare shadow event
        } QCAP_QPOS_CTL_bit;
    }; // 2Ch-2Fh
    union
    {   // QEP interrupt control and flag register
        volatile unsigned QINT_EN_FLG;
        volatile struct
        {
            unsigned rsrvd0: 1; // 0
            unsigned PCEI_EN: 1; // 1
            unsigned QPEI_EN: 1; // 2
            unsigned QDCI_EN: 1; // 3
            unsigned WTOI_EN: 1; // 4
            unsigned PCUI_EN: 1; // 5
            unsigned PCOI_EN: 1; //6
            unsigned PCRI_EN: 1; // 7
            unsigned PCMI_EN: 1; // 8
            unsigned SELI_EN: 1; // 9
            unsigned IELI_EN: 1; // 10
            unsigned UTOI_EN: 1; // 11
            unsigned rsrvd1: 4; // 12-15
            unsigned INT_FLG: 1; // 16
            unsigned PCEI_FLG: 1; // 17
            unsigned QPEI_FLG: 1; // 18
            unsigned QDCI_FLG: 1; // 19
            unsigned WTOI_FLG: 1; // 20
            unsigned PCUI_FLG: 1; // 21
            unsigned PCOI_FLG: 1; // 22
            unsigned PCRI_FLG: 1; // 23
            unsigned PCMI_FLG: 1; // 24
            unsigned SELI_FLG: 1; // 25
            unsigned IELI_FLG: 1; // 26
            unsigned UTOI_FLG: 1; // 27
            unsigned rsrvd2: 4; // 28-31
        } QINT_EN_FLG_bit;
    }; // 30h - 33h
    union
    {   // QEP interrupt control and flag register
        volatile unsigned QINT_CLR_FRC;
        volatile struct
        {
            unsigned INT_CLR: 1; // 0
            unsigned PCEI_CLR: 1; // 1
            unsigned QPEI_CLR: 1; // 2
            unsigned QDCI_CLR: 1; // 3
            unsigned WTOI_CLR: 1; // 4
            unsigned PCUI_CLR: 1; // 5
            unsigned PCOI_CLR: 1; // 6
            unsigned PCRI_CLR: 1; // 7
            unsigned PCMI_CLR: 1; // 8
            unsigned SELI_CLR: 1; // 9
            unsigned IELI_CLR: 1; // 10
            unsigned UTOI_CLR: 1; // 11
            unsigned rsrvd0: 5; //12-16
            unsigned PCEI_FRC: 1; // 17
            unsigned QPEI_FRC: 1; // 18
            unsigned QDCI_FRC: 1; // 19
            unsigned WTOI_FRC: 1; // 20
            unsigned PCUI_FRC: 1; // 21
            unsigned PCOI_FRC: 1; // 22
            unsigned PCRI_FRC: 1; // 23
            unsigned PCMI_FRC: 1; // 24
            unsigned SELI_FRC: 1; // 25
            unsigned IELI_FRC: 1; // 26
            unsigned UTOI_FRC: 1; // 27
            unsigned rsrvd1: 4; // 28-31
        } QINT_CLR_FRC_bit;
    }; // 34h-37h
    union
    {   // QEP status and capture timer register
        volatile unsigned QEP_STS_CT; // 38h-3Bh
        volatile struct
        {
            unsigned PCEF: 1; // 0
            unsigned FIMF: 1; // 1
            unsigned CDEF: 1; // 2
            unsigned COEF: 1; // 3
            unsigned QDLF: 1; // 4
            unsigned QDF: 1; // 5
            unsigned FIDF: 1; // 6
            unsigned rsrvd: 9; // 7-15
            unsigned QCTMR: 16; // 16-31
        } QEP_STS_CT_bit;
    };
    union
    {   // QEP capture period and timer latch register
        volatile unsigned QC_PRD_TLAT; // 3Ch-3Fh
        volatile struct
        {
            unsigned QCPRD: 16;
            unsigned QCTMRLAT: 16;
        } QC_PRD_TLAT_bit;

    };
    union
    {   // QEP capture period latch register
        volatile unsigned QCPRDLAT; // 40h-37h
        volatile struct
        {
            unsigned QCPRDLAT: 16;
            unsigned rsrvd: 16;
        } QCPRDLAT_bit;
    };
    union
    {   // Peripheral ID register
        volatile unsigned PID;
        volatile struct {
            unsigned REVISION : 32; // 0x44D31903
        } PID_bit;
    }; // 5Ch-5Fh
} eqep_t; //size = 60h
#define EQEP0 (*((volatile eqep_t *)EQEP0_OFFSET))
#define EQEP1 (*((volatile eqep_t *)EQEP1_OFFSET))
#endif /* _SYS_EQEP_H_ */
