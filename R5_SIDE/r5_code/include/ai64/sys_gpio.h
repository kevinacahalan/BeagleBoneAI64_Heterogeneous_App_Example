#ifndef _SYS_GPIO_H_
#define _SYS_GPIO_H_

// From memory mapped GPIO Registers Table 12-127 am64x_tech_ref.pdf page 4981
#ifdef SOC_AM64X // SK-AM64B
    #define GPIO0_OFFSET              0x00600000
    #define GPIO1_OFFSET              0x00601000
    #define MCU_GPIO_OFFSET           0x04201000
#endif
#ifdef J721E_TDA4VM
    #define GPIO0_OFFSET              0x00600000
    #define GPIO1_OFFSET              0x00601000
    #define GPIO2_OFFSET              0x00610000
    #define GPIO3_OFFSET              0x00611000
#endif
#ifdef BBB_AM335X
    #define GPIO0_OFFSET              0x44E07000
    #define GPIO1_OFFSET              0x4804C000
    #define GPIO2_OFFSET              0x481AC000
    #define GPIO3_OFFSET              0x481AE000
#endif

// BBB P8 PRU0 pins
#define GPIO1_P8_11_BIT13        0x00002000
#define GPIO1_P8_12_BIT12        0x00001000
#define GPIO1_P8_15_BIT15        0x00008000
#define GPIO1_P8_16_BIT14        0x00004000

#define R30_P8_11_BIT15          0x00008000
#define R30_P8_12_BIT14          0x00004000
#define R31_P8_15_BIT15          0x00008000
#define P31_P8_16_BIT14          0x00004000

//BBB P9 PRU0 pins
#define GPIO3_P9_24_BIT15       0x00008000
#define GPIO3_P9_25_BIT21       0x00200000
#define GPIO3_P9_27_bit25       0x00080000
#define GPIO3_P9_28_bit23       0x00010000
#define GPIO3_P9_29_BIT15       0x00008000
#define GPIO3_P9_30_bit22       0x00010000
#define GPIO3_P9_31_BIT14       0x00004000

#define R31_P9_25_BIT7          0x00000080
#define R31_P9_27_BIT5          0x00000020
#define R31_P9_28_BIT3          0x00000008
#define R31_P9_29_BIT1          0x00000002
#define R31_P9_30_BIT2          0x00000004
#define R31_P9_31_BIT0          0x00000001

// BBB P8 PRU1_pins
#define GPIO0_P9_26_BIT14       0x00004000

/* SK-AM64B pin use defined in motor_control.syscfg
Name              GPIO Periherals  Pins                  Ball  Connector Pin
------------      ---------------  --------------------  ----  ----------------
CONFIG_GPIO0      MCU_GPIO         GPIO_Pin(MCU_GPIO0_6  B7    MCU J9_10
CONFIG_GPIO1      MCU_GPIO         GPIO_Pin(MCU_GPIO0_7  D7    MCU J9_11
CONFIG_GPIO2      MCU_GPIO         GPIO_Pin(MCU_GPIO0_9  C8    MCU J9_14
CONFIG_PRU_ICCS0  PRU_ICSSG0_PRU0  PRG0_PRU0_GPO19       W1    PRU J10_32
CONFIG_PRU_ICCS1  PRU_ICSSG0_PRU1  PRG0_PRU1_GPO14       U6    PRU J10_49
*/


#define CLKFREQ                200u
#define DEBUG_PIN_PRU0         GPIO1_P8_16_BIT14 // BBB pulse for scope debug, lets me no where I am in code
#define DEBUG_PIN_PRU1         GPIO0_P9_26_BIT14 // BBB pulse for scope debug, lets me no where I am in code

void initializeGpio_pru0(void);
void scopeDebug_pru0(int periodUs);
void initializeGpio_pru1(void);
void scopeDebug_pru1(int periodUs);

/* Declare gpio structure examples
  * volatile far gpio *GPIO0 = (volatile gpio *)(GPIO0_OFFSET);
  * volatile far gpio *GPIO1 = (volatile gpio *)(GPIO1_OFFSET);
  * volatile far gpio *MCU_GPIO = (volatile gpio *)(MCU_GPIO_OFFSET);
  */
typedef struct
{
    union
    {
        volatile unsigned GPIO_PID;
        volatile struct
        {
            unsigned MINOR : 6;
            unsigned CUSTOM: 2;
            unsigned MAJOR: 3;
            unsigned RTL: 5;
            unsigned FUNC: 12;
            unsigned rsrvd: 2;
            unsigned SCHEME: 2;
        } GPIO_PID_bit;
    } ; //0h - 3h - reset 44832905h
    union
    {
        volatile unsigned PCR;
        volatile struct
        {
            unsigned FREE: 1;
            unsigned SOFT: 1;
            unsigned rsrvd: 30;
        } PCR_bit;
    } ; //4h-7h, reset: Xh
    union
    {
        volatile unsigned BINTEN;
        volatile struct
        {
            unsigned EN: 16;
            unsigned rsrvd: 16;
        } BINTEN_bit;
    } ; //8h-bh, reset: 0h
    unsigned rsrvd0; //ch-fh
    union
    {
        volatile unsigned DIR01;
        volatile struct
        {
            unsigned OE_bit0: 1; // 0
            unsigned OE_bit1: 1; // 1
            unsigned OE_bit2: 1; // 2
            unsigned OE_bit3: 1; // 3

            unsigned OE_bit4: 1; // 2
            unsigned OE_bit5: 1; // 5
            unsigned OE_bit6: 1; // 6
            unsigned OE_bit7: 1; // 7

            unsigned OE_bit8: 1;
            unsigned OE_bit9: 1;
            unsigned OE_bit10: 1; // 10
            unsigned OE_bit11: 1; // 11

            unsigned OE_bit12: 1; // 12
            unsigned OE_bit13: 1; // 13
            unsigned OE_bit14: 1; // 14
            unsigned OE_bit15: 1; // 15

            unsigned OE_bit16: 1; // 16
            unsigned OE_bit17: 1; // 17
            unsigned OE_bit18: 1; // 18
            unsigned OE_bit19: 1; // 19

            unsigned OE_bit20: 1; // 20
            unsigned OE_bit21: 1; // 21
            unsigned OE_bit22: 1; // 22
            unsigned OE_bit23: 1; // 23

            unsigned OE_bit24: 1; // 24
            unsigned OE_bit25: 1; // 25
            unsigned OE_bit26: 1; // 26
            unsigned OE_bit27: 1; //27

            unsigned OE_bit28: 1; // 28
            unsigned OE_bit29: 1; // 29
            unsigned OE_bit30: 1; // 30
            unsigned OE_bit31: 1; // 31
        } DIR01_bit;
    }; //10h-13h, reset: ffffffffh
    union
    {
        volatile unsigned OUT_DATA01;
        volatile struct
        {
            unsigned DO_bit0: 1;
            unsigned DO_bit1: 1;
            unsigned DO_bit2: 1;
            unsigned DO_bit3: 1;

            unsigned DO_bit4: 1;
            unsigned DO_bit5: 1;
            unsigned DO_bit6: 1;
            unsigned DO_bit7: 1;

            unsigned DO_bit8: 1;
            unsigned DO_bit9: 1;
            unsigned DO_bit10: 1;
            unsigned DO_bit11: 1;

            unsigned DO_bit12: 1;
            unsigned DO_bit13: 1;
            unsigned DO_bit14: 1;
            unsigned DO_bit15: 1;

            unsigned DO_bit16: 1;
            unsigned DO_bit17: 1;
            unsigned DO_bit18: 1;
            unsigned DO_bit19: 1;

            unsigned DO_bit20: 1;
            unsigned DO_bit21: 1;
            unsigned DO_bit22: 1;
            unsigned DO_bit23: 1;

            unsigned DO_bit24: 1;
            unsigned DO_bit25: 1;
            unsigned DO_bit26: 1;
            unsigned DO_bit27: 1;
      
            unsigned DO_bit28: 1;
            unsigned DO_bit29: 1;
            unsigned DO_bit30: 1;
            unsigned DO_bit31: 1;
        } OUT_DATA01_bit;
    };//14h-17h, reset: 0h
    union
    {
        volatile unsigned SET_DATA01;
        volatile struct
        {
            unsigned SD_bit0: 1;
            unsigned SD_bit1: 1;
            unsigned SD_bit2: 1;
            unsigned SD_bit3: 1;

            unsigned SD_bit4: 1;
            unsigned SD_bit5: 1;
            unsigned SD_bit6: 1;
            unsigned SD_bit7: 1;

            unsigned SD_bit8: 1;
            unsigned SD_bit9: 1;
            unsigned SD_bit10: 1;
            unsigned SD_bit11: 1;

            unsigned SD_bit12: 1;
            unsigned SD_bit13: 1;
            unsigned SD_bit14: 1;
            unsigned SD_bit15: 1;

            unsigned SD_bit16: 1;
            unsigned SD_bit17: 1;
            unsigned SD_bit18: 1;
            unsigned SD_bit19: 1;

            unsigned SD_bit20: 1;
            unsigned SD_bit21: 1;
            unsigned SD_bit22: 1;
            unsigned SD_bit23: 1;

            unsigned SD_bit24: 1;
            unsigned SD_bit25: 1;
            unsigned SD_bit26: 1;
            unsigned SD_bit27: 1;

            unsigned SD_bit28: 1;
            unsigned SD_bit29: 1;
            unsigned SD_bit30: 1;
            unsigned SD_bit31: 1;
        } SET_DATA01_bit;
    }; //18h-1Bh
    union
    {
        volatile unsigned CLR_DATA01;
        volatile struct
        {
            unsigned CD_bit0: 1;
            unsigned CD_bit1: 1;
            unsigned CD_bit2: 1;
            unsigned CD_bit3: 1;

            unsigned CD_bit4: 1;
            unsigned CD_bit5: 1;
            unsigned CD_bit6: 1;
            unsigned CD_bit7: 1;

            unsigned CD_bit8: 1;
            unsigned CD_bit9: 1;
            unsigned CD_bit10: 1;
            unsigned CD_bit11: 1;

            unsigned CD_bit12: 1;
            unsigned CD_bit13: 1;
            unsigned CD_bit14: 1;
            unsigned CD_bit15: 1;

            unsigned CD_bit16: 1;
            unsigned CD_bit17: 1;
            unsigned CD_bit18: 1;
            unsigned CD_bit19: 1;

            unsigned CD_bit20: 1;
            unsigned CD_bit21: 1;
            unsigned CD_bit22: 1;
            unsigned CD_bit23: 1;

            unsigned CD_bit24: 1;
            unsigned CD_bit25: 1;
            unsigned CD_bit26: 1;
            unsigned CD_bit27: 1;

            unsigned CD_bit28: 1;
            unsigned CD_bit29: 1;
            unsigned CD_bit30: 1;
            unsigned CD_bit31: 1;
        } CLR_DATA01_bit;
    }; //1Ch-1Fh
    union
    {
        volatile unsigned IN_DATA01;
        volatile struct
        {
            unsigned DI_bit0: 1;
            unsigned DI_bit1: 1;
            unsigned DI_bit2: 1;
            unsigned DI_bit3: 1;

            unsigned DI_bit4: 1;
            unsigned DI_bit5: 1;
            unsigned DI_bit6: 1;
            unsigned DI_bit7: 1;
            unsigned DI_bit8: 1;
            unsigned DI_bit9: 1;
            unsigned DI_bit10: 1;
            unsigned DI_bit11: 1;

            unsigned DI_bit12: 1;
            unsigned DI_bit13: 1;
            unsigned DI_bit14: 1;
            unsigned DI_bit15: 1;

            unsigned DI_bit16: 1;
            unsigned DI_bit17: 1;
            unsigned DI_bit18: 1;
            unsigned DI_bit19: 1;

            unsigned DI_bit20: 1;
            unsigned DI_bit21: 1;
            unsigned DI_bit22: 1;
            unsigned DI_bit23: 1;

            unsigned DI_bit24: 1;
            unsigned DI_bit25: 1;
            unsigned DI_bit26: 1;
            unsigned DI_bit27: 1;

            unsigned DI_bit28: 1;
            unsigned DI_bit29: 1;
            unsigned DI_bit30: 1;
            unsigned DI_bit31: 1;
        } IN_DATA01_bit;
    };//20h-23h, reset: 0h
    volatile unsigned SET_RIS_TRIG01; //24h-27h, reset: 0h
    volatile unsigned CLR_RIS_TRIG01; //28ch-2Bh, reset: 0h
    volatile unsigned SET_FALL_TRIG01; //2Ch-2Fh, reset: 0h
    volatile unsigned CLR_FALL_TRIG01; //30h-33fh, reset: 0h
    volatile unsigned INTSTAT01; // 34h-37h
    union
    {
        volatile unsigned DIR23;
        volatile struct
        {
            unsigned OE_bit0: 1;  // 32
            unsigned OE_bit1: 1;  // 33
            unsigned OE_bit2: 1;  // 34
            unsigned OE_bit3: 1;  // 35

            unsigned OE_bit4: 1;  // 36
            unsigned OE_bit5: 1;  // 37
            unsigned OE_bit6: 1;  // 38
            unsigned OE_bit7: 1;  // 39

            unsigned OE_bit8: 1;  // 40
            unsigned OE_bit9: 1;  // 41
            unsigned OE_bit10: 1; // 42
            unsigned OE_bit11: 1; // 43

            unsigned OE_bit12: 1; // 44
            unsigned OE_bit13: 1; // 45
            unsigned OE_bit14: 1; // 46
            unsigned OE_bit15: 1; // 47

            unsigned OE_bit16: 1; // 48
            unsigned OE_bit17: 1; // 49
            unsigned OE_bit18: 1; // 50
            unsigned OE_bit19: 1; // 51

            unsigned OE_bit20: 1; // 52
            unsigned OE_bit21: 1; // 53
            unsigned OE_bit22: 1; // 54
            unsigned OE_bit23: 1; // 55

            unsigned OE_bit24: 1; // 56
            unsigned OE_bit25: 1; // 57
            unsigned OE_bit26: 1; // 58
            unsigned OE_bit27: 1; // 59

            unsigned OE_bit28: 1; // 60
            unsigned OE_bit29: 1; // 61
            unsigned OE_bit30: 1; // 62
            unsigned OE_bit31: 1; // 63
        } DIR23_bit;
    }; //38h-3bh, reset: ffffffffh
    union
    {
        volatile unsigned OUT_DATA23;
        volatile struct
        {
            unsigned DO_bit0: 1;
            unsigned DO_bit1: 1;
            unsigned DO_bit2: 1;
            unsigned DO_bit3: 1;

            unsigned DO_bit4: 1;
            unsigned DO_bit5: 1;
            unsigned DO_bit6: 1;
            unsigned DO_bit7: 1;

            unsigned DO_bit8: 1;
            unsigned DO_bit9: 1;
            unsigned DO_bit10: 1;
            unsigned DO_bit11: 1;

            unsigned DO_bit12: 1;
            unsigned DO_bit13: 1;
            unsigned DO_bit14: 1;
            unsigned DO_bit15: 1;

            unsigned DO_bit16: 1;
            unsigned DO_bit17: 1;
            unsigned DO_bit18: 1;
            unsigned DO_bit19: 1;

            unsigned DO_bit20: 1;
            unsigned DO_bit21: 1;
            unsigned DO_bit22: 1;
            unsigned DO_bit23: 1;

            unsigned DO_bit24: 1;
            unsigned DO_bit25: 1;
            unsigned DO_bit26: 1;
            unsigned DO_bit27: 1;

            unsigned DO_bit28: 1;
            unsigned DO_bit29: 1;
            unsigned DO_bit30: 1;
            unsigned DO_bit31: 1;
        } OUT_DATA23_bit;
    };// 3ch-3fh, reset: 0h
    union
    {
        volatile unsigned SET_DATA23;
        volatile struct
        {
            unsigned SD_bit0: 1;
            unsigned SD_bit1: 1;
            unsigned SD_bit2: 1;
            unsigned SD_bit3: 1;

            unsigned SD_bit4: 1;
            unsigned SD_bit5: 1;
            unsigned SD_bit6: 1;
            unsigned SD_bit7: 1;

            unsigned SD_bit8: 1;
            unsigned SD_bit9: 1;
            unsigned SD_bit10: 1;
            unsigned SD_bit11: 1;

            unsigned SD_bit12: 1;
            unsigned SD_bit13: 1;
            unsigned SD_bit14: 1;
            unsigned SD_bit15: 1;

            unsigned SD_bit16: 1;
            unsigned SD_bit17: 1;
            unsigned SD_bit18: 1;
            unsigned SD_bit19: 1;

            unsigned SD_bit20: 1;
            unsigned SD_bit21: 1;
            unsigned SD_bit22: 1;
            unsigned SD_bit23: 1;

            unsigned SD_bit24: 1;
            unsigned SD_bit25: 1;
            unsigned SD_bit26: 1;
            unsigned SD_bit27: 1;

            unsigned SD_bit28: 1;
            unsigned SD_bit29: 1;
            unsigned SD_bit30: 1;
            unsigned SD_bit31: 1;
        } SET_DATA23_bit;
    }; // 40h-43h
    union
    {
        volatile unsigned CLR_DATA23;
        volatile struct
        {
            unsigned CD_bit0: 1;
            unsigned CD_bit1: 1;
            unsigned CD_bit2: 1;
            unsigned CD_bit3: 1;

            unsigned CD_bit4: 1;
            unsigned CD_bit5: 1;
            unsigned CD_bit6: 1;
            unsigned CD_bit7: 1;

            unsigned CD_bit8: 1;
            unsigned CD_bit9: 1;
            unsigned CD_bit10: 1;
            unsigned CD_bit11: 1;

            unsigned CD_bit12: 1;
            unsigned CD_bit13: 1;
            unsigned CD_bit14: 1;
            unsigned CD_bit15: 1;

            unsigned CD_bit16: 1;
            unsigned CD_bit17: 1;
            unsigned CD_bit18: 1;
            unsigned CD_bit19: 1;

            unsigned CD_bit20: 1;
            unsigned CD_bit21: 1;
            unsigned CD_bit22: 1;
            unsigned CD_bit23: 1;

            unsigned CD_bit24: 1;
            unsigned CD_bit25: 1;
            unsigned CD_bit26: 1;
            unsigned CD_bit27: 1;

            unsigned CD_bit28: 1;
            unsigned CD_bit29: 1;
            unsigned CD_bit30: 1;
            unsigned CD_bit31: 1;
        } CLR_DATA23_bit;
    }; // 44h-47h
    union
    {
        volatile unsigned IN_DATA23;
        volatile struct
        {
            unsigned DI_bit0: 1;
            unsigned DI_bit1: 1;
            unsigned DI_bit2: 1;
            unsigned DI_bit3: 1;

            unsigned DI_bit4: 1;
            unsigned DI_bit5: 1;
            unsigned DI_bit6: 1;
            unsigned DI_bit7: 1;

            unsigned DI_bit8: 1;
            unsigned DI_bit9: 1;
            unsigned DI_bit10: 1;
            unsigned DI_bit11: 1;

            unsigned DI_bit12: 1;
            unsigned DI_bit13: 1;
            unsigned DI_bit14: 1;
            unsigned DI_bit15: 1;

            unsigned DI_bit16: 1;
            unsigned DI_bit17: 1;
            unsigned DI_bit18: 1;
            unsigned DI_bit19: 1;

            unsigned DI_bit20: 1;
            unsigned DI_bit21: 1;
            unsigned DI_bit22: 1;
            unsigned DI_bit23: 1;

            unsigned DI_bit24: 1;
            unsigned DI_bit25: 1;
            unsigned DI_bit26: 1;
            unsigned DI_bit27: 1;

            unsigned DI_bit28: 1;
            unsigned DI_bit29: 1;
            unsigned DI_bit30: 1;
            unsigned DI_bit31: 1;
        } IN_DATA23_bit;
    };// 48h-4bh, reset: 0h
    volatile unsigned SET_RIS_TRIG23; // 4ch-4fh, reset: 0h
    volatile unsigned CLR_RIS_TRIG23; // 50h-53h, reset: 0h
    volatile unsigned SET_FALL_TRIG23; //54h-57h, reset: 0h
    volatile unsigned CLR_FALL_TRIG23; //58h-5bh, reset: 0h
    volatile unsigned INTSTAT23; // 5ch-5fh
    union
    {
        volatile unsigned DIR45;
        volatile struct
        {
            unsigned OE_bit0: 1; // 64
            unsigned OE_bit1: 1; // 65
            unsigned OE_bit2: 1; // 66
            unsigned OE_bit3: 1; // 67

            unsigned OE_bit4: 1; // 68
            unsigned OE_bit5: 1; // 69
            unsigned OE_bit6: 1; // 70
            unsigned OE_bit7: 1; // 71

            unsigned OE_bit8: 1; // 72
            unsigned OE_bit9: 1; // 73
            unsigned OE_bit10: 1; // 74
            unsigned OE_bit11: 1; // 75

            unsigned OE_bit12: 1; // 76
            unsigned OE_bit13: 1; // 77
            unsigned OE_bit14: 1; // 78
            unsigned OE_bit15: 1; // 79

            unsigned OE_bit16: 1; // 80
            unsigned OE_bit17: 1; // 81
            unsigned OE_bit18: 1; // 82
            unsigned OE_bit19: 1; // 83

            unsigned OE_bit20: 1; // 84
            unsigned OE_bit21: 1; // 85
            unsigned OE_bit22: 1; // 86
            unsigned OE_bit23: 1; // 87

            unsigned OE_bit24: 1; // 88
            unsigned OE_bit25: 1; // 89
            unsigned OE_bit26: 1; // 90
            unsigned OE_bit27: 1; // 91

            unsigned OE_bit28: 1; // 92
            unsigned OE_bit29: 1; // 93
            unsigned OE_bit30: 1; // 94
            unsigned OE_bit31: 1; // 95
        } DIR45_bit;
    }; //38h-3bh, reset: ffffffffh
    union
    {
        volatile unsigned OUT_DATA45;
        volatile struct
        {
            unsigned DO_bit0: 1;
            unsigned DO_bit1: 1;
            unsigned DO_bit2: 1;
            unsigned DO_bit3: 1;

            unsigned DO_bit4: 1;
            unsigned DO_bit5: 1;
            unsigned DO_bit6: 1;
            unsigned DO_bit7: 1;

            unsigned DO_bit8: 1;
            unsigned DO_bit9: 1;
            unsigned DO_bit10: 1;
            unsigned DO_bit11: 1;

            unsigned DO_bit12: 1;
            unsigned DO_bit13: 1;
            unsigned DO_bit14: 1;
            unsigned DO_bit15: 1;

            unsigned DO_bit16: 1;
            unsigned DO_bit17: 1;
            unsigned DO_bit18: 1;
            unsigned DO_bit19: 1;

            unsigned DO_bit20: 1;
            unsigned DO_bit21: 1;
            unsigned DO_bit22: 1;
            unsigned DO_bit23: 1;

            unsigned DO_bit24: 1;
            unsigned DO_bit25: 1;
            unsigned DO_bit26: 1;
            unsigned DO_bit27: 1;

            unsigned DO_bit28: 1;
            unsigned DO_bit29: 1;
            unsigned DO_bit30: 1;
            unsigned DO_bit31: 1;
        } OUT_DATA45_bit;
    };// 3ch-3fh, reset: 0h
    union
    {
        volatile unsigned SET_DATA45;
        volatile struct
        {
            unsigned SD_bit0: 1;
            unsigned SD_bit1: 1;
            unsigned SD_bit2: 1;
            unsigned SD_bit3: 1;

            unsigned SD_bit4: 1;
            unsigned SD_bit5: 1;
            unsigned SD_bit6: 1;
            unsigned SD_bit7: 1;

            unsigned SD_bit8: 1;
            unsigned SD_bit9: 1;
            unsigned SD_bit10: 1;
            unsigned SD_bit11: 1;

            unsigned SD_bit12: 1;
            unsigned SD_bit13: 1;
            unsigned SD_bit14: 1;
            unsigned SD_bit15: 1;

            unsigned SD_bit16: 1;
            unsigned SD_bit17: 1;
            unsigned SD_bit18: 1;
            unsigned SD_bit19: 1;

            unsigned SD_bit20: 1;
            unsigned SD_bit21: 1;
            unsigned SD_bit22: 1;
            unsigned SD_bit23: 1;

            unsigned SD_bit24: 1;
            unsigned SD_bit25: 1;
            unsigned SD_bit26: 1;
            unsigned SD_bit27: 1;

            unsigned SD_bit28: 1;
            unsigned SD_bit29: 1;
            unsigned SD_bit30: 1;
            unsigned SD_bit31: 1;
        } SET_DATA45_bit;
    }; // 40h-43h
    union
    {
        volatile unsigned CLR_DATA45;
        volatile struct
        {
            unsigned CD_bit0: 1;
            unsigned CD_bit1: 1;
            unsigned CD_bit2: 1;
            unsigned CD_bit3: 1;

            unsigned CD_bit4: 1;
            unsigned CD_bit5: 1;
            unsigned CD_bit6: 1;
            unsigned CD_bit7: 1;

            unsigned CD_bit8: 1;
            unsigned CD_bit9: 1;
            unsigned CD_bit10: 1;
            unsigned CD_bit11: 1;

            unsigned CD_bit12: 1;
            unsigned CD_bit13: 1;
            unsigned CD_bit14: 1;
            unsigned CD_bit15: 1;

            unsigned CD_bit16: 1;
            unsigned CD_bit17: 1;
            unsigned CD_bit18: 1;
            unsigned CD_bit19: 1;

            unsigned CD_bit20: 1;
            unsigned CD_bit21: 1;
            unsigned CD_bit22: 1;
            unsigned CD_bit23: 1;

            unsigned CD_bit24: 1;
            unsigned CD_bit25: 1;
            unsigned CD_bit26: 1;
            unsigned CD_bit27: 1;

            unsigned CD_bit28: 1;
            unsigned CD_bit29: 1;
            unsigned CD_bit30: 1;
           unsigned CD_bit31: 1;
        } CLR_DATA45_bit;
    }; // 44h-47h
    union
    {
        volatile unsigned IN_DATA45;
        volatile struct
        {
            unsigned DI_bit0: 1;
            unsigned DI_bit1: 1;
            unsigned DI_bit2: 1;
            unsigned DI_bit3: 1;

            unsigned DI_bit4: 1;
            unsigned DI_bit5: 1;
            unsigned DI_bit6: 1;
            unsigned DI_bit7: 1;

            unsigned DI_bit8: 1;
            unsigned DI_bit9: 1;
            unsigned DI_bit10: 1;
            unsigned DI_bit11: 1;

            unsigned DI_bit12: 1;
            unsigned DI_bit13: 1;
            unsigned DI_bit14: 1;
            unsigned DI_bit15: 1;

            unsigned DI_bit16: 1;
            unsigned DI_bit17: 1;
            unsigned DI_bit18: 1;
            unsigned DI_bit19: 1;

            unsigned DI_bit20: 1;
            unsigned DI_bit21: 1;
            unsigned DI_bit22: 1;
            unsigned DI_bit23: 1;

            unsigned DI_bit24: 1;
            unsigned DI_bit25: 1;
            unsigned DI_bit26: 1;
            unsigned DI_bit27: 1;

            unsigned DI_bit28: 1;
            unsigned DI_bit29: 1;
            unsigned DI_bit30: 1;
            unsigned DI_bit31: 1;
        } IN_DATA45_bit;
    };// 48h-4bh, reset: 0h
    volatile unsigned SET_RIS_TRIG45; // 4ch-4fh, reset: 0h
    volatile unsigned CLR_RIS_TRIG45; // 50h-53h, reset: 0h
    volatile unsigned SET_FALL_TRIG45; //54h-57h, reset: 0h
    volatile unsigned CLR_FALL_TRIG45; //58h-5bh, reset: 0h
    volatile unsigned INTSTAT45; // 5ch-5fh
    union
    {
        volatile unsigned DIR67;
        volatile struct
        {
            unsigned OE_bit0: 1; // 96
            unsigned OE_bit1: 1; // 97
            unsigned OE_bit2: 1; // 98
            unsigned OE_bit3: 1; // 99

            unsigned OE_bit4: 1; // 100
            unsigned OE_bit5: 1; // 101
            unsigned OE_bit6: 1; // 102
            unsigned OE_bit7: 1; // 103

            unsigned OE_bit8: 1; // 104
            unsigned OE_bit9: 1; // 105
            unsigned OE_bit10: 1; // 106
            unsigned OE_bit11: 1; // 107

            unsigned OE_bit12: 1; // 108
            unsigned OE_bit13: 1; // 109
            unsigned OE_bit14: 1; // 110
            unsigned OE_bit15: 1; // 111

            unsigned OE_bit16: 1; // 112
            unsigned OE_bit17: 1; // 113
            unsigned OE_bit18: 1; // 114
            unsigned OE_bit19: 1; // 115

            unsigned OE_bit20: 1; // 116
            unsigned OE_bit21: 1; // 117
            unsigned OE_bit22: 1; // 118
            unsigned OE_bit23: 1; // 119

            unsigned OE_bit24: 1; // 120
            unsigned OE_bit25: 1; // 121
            unsigned OE_bit26: 1; // 122
            unsigned OE_bit27: 1; // 123

            unsigned OE_bit28: 1; // 124
            unsigned OE_bit29: 1; // 125
            unsigned OE_bit30: 1; // 126
            unsigned OE_bit31: 1; // 127
        } DIR67_bit;
    }; //38h-3bh, reset: ffffffffh
    union
    {
        volatile unsigned OUT_DATA67;
        volatile struct
        {
            unsigned DO_bit0: 1;
            unsigned DO_bit1: 1;
            unsigned DO_bit2: 1;
            unsigned DO_bit3: 1;

            unsigned DO_bit4: 1;
            unsigned DO_bit5: 1;
            unsigned DO_bit6: 1;
            unsigned DO_bit7: 1;

            unsigned DO_bit8: 1;
            unsigned DO_bit9: 1;
            unsigned DO_bit10: 1;
            unsigned DO_bit11: 1;

            unsigned DO_bit12: 1;
            unsigned DO_bit13: 1;
            unsigned DO_bit14: 1;
            unsigned DO_bit15: 1;

            unsigned DO_bit16: 1;
            unsigned DO_bit17: 1;
            unsigned DO_bit18: 1;
            unsigned DO_bit19: 1;

            unsigned DO_bit20: 1;
            unsigned DO_bit21: 1;
            unsigned DO_bit22: 1;
            unsigned DO_bit23: 1;

            unsigned DO_bit24: 1;
            unsigned DO_bit25: 1;
            unsigned DO_bit26: 1;
            unsigned DO_bit27: 1;

            unsigned DO_bit28: 1;
            unsigned DO_bit29: 1;
            unsigned DO_bit30: 1;
            unsigned DO_bit31: 1;
        } OUT_DATA67_bit;
    };// 3ch-3fh, reset: 0h
    union
    {
        volatile unsigned SET_DATA67;
        volatile struct
        {
            unsigned SD_bit0: 1;
            unsigned SD_bit1: 1;
            unsigned SD_bit2: 1;
            unsigned SD_bit3: 1;

            unsigned SD_bit4: 1;
            unsigned SD_bit5: 1;
            unsigned SD_bit6: 1;
            unsigned SD_bit7: 1;

            unsigned SD_bit8: 1;
            unsigned SD_bit9: 1;
            unsigned SD_bit10: 1;
            unsigned SD_bit11: 1;

            unsigned SD_bit12: 1;
            unsigned SD_bit13: 1;
            unsigned SD_bit14: 1;
            unsigned SD_bit15: 1;

            unsigned SD_bit16: 1;
            unsigned SD_bit17: 1;
            unsigned SD_bit18: 1;
            unsigned SD_bit19: 1;

            unsigned SD_bit20: 1;
            unsigned SD_bit21: 1;
            unsigned SD_bit22: 1;
            unsigned SD_bit23: 1;

            unsigned SD_bit24: 1;
            unsigned SD_bit25: 1;
            unsigned SD_bit26: 1;
            unsigned SD_bit27: 1;

            unsigned SD_bit28: 1;
            unsigned SD_bit29: 1;
            unsigned SD_bit30: 1;
            unsigned SD_bit31: 1;
        } SET_DATA67_bit;
    }; // 40h-43h
    union
    {
        volatile unsigned CLR_DATA67;
        volatile struct
        {
            unsigned CD_bit0: 1;
            unsigned CD_bit1: 1;
            unsigned CD_bit2: 1;
            unsigned CD_bit3: 1;

            unsigned CD_bit4: 1;
            unsigned CD_bit5: 1;
            unsigned CD_bit6: 1;
            unsigned CD_bit7: 1;

            unsigned CD_bit8: 1;
            unsigned CD_bit9: 1;
            unsigned CD_bit10: 1;
            unsigned CD_bit11: 1;

            unsigned CD_bit12: 1;
            unsigned CD_bit13: 1;
            unsigned CD_bit14: 1;
            unsigned CD_bit15: 1;

            unsigned CD_bit16: 1;
            unsigned CD_bit17: 1;
            unsigned CD_bit18: 1;
            unsigned CD_bit19: 1;

            unsigned CD_bit20: 1;
            unsigned CD_bit21: 1;
            unsigned CD_bit22: 1;
            unsigned CD_bit23: 1;

            unsigned CD_bit24: 1;
            unsigned CD_bit25: 1;
            unsigned CD_bit26: 1;
            unsigned CD_bit27: 1;

            unsigned CD_bit28: 1;
            unsigned CD_bit29: 1;
            unsigned CD_bit30: 1;
            unsigned CD_bit31: 1;
        } CLR_DATA67_bit;
    }; // 44h-47h
    union
    {
        volatile unsigned IN_DATA67;
        volatile struct
        {
            unsigned DI_bit0: 1;
            unsigned DI_bit1: 1;
            unsigned DI_bit2: 1;
            unsigned DI_bit3: 1;

            unsigned DI_bit4: 1;
            unsigned DI_bit5: 1;
            unsigned DI_bit6: 1;
            unsigned DI_bit7: 1;

            unsigned DI_bit8: 1;
            unsigned DI_bit9: 1;
            unsigned DI_bit10: 1;
            unsigned DI_bit11: 1;

            unsigned DI_bit12: 1;
            unsigned DI_bit13: 1;
            unsigned DI_bit14: 1;
            unsigned DI_bit15: 1;

            unsigned DI_bit16: 1;
            unsigned DI_bit17: 1;
            unsigned DI_bit18: 1;
            unsigned DI_bit19: 1;

            unsigned DI_bit20: 1;
            unsigned DI_bit21: 1;
            unsigned DI_bit22: 1;
            unsigned DI_bit23: 1;

            unsigned DI_bit24: 1;
            unsigned DI_bit25: 1;
            unsigned DI_bit26: 1;
            unsigned DI_bit27: 1;

            unsigned DI_bit28: 1;
            unsigned DI_bit29: 1;
            unsigned DI_bit30: 1;
            unsigned DI_bit31: 1;
        } IN_DATA67_bit;
        volatile unsigned SET_RIS_TRIG67; // 4ch-4fh, reset: 0h
        volatile unsigned CLR_RIS_TRIG67; // 50h-53h, reset: 0h
        volatile unsigned SET_FALL_TRIG67; //54h-57h, reset: 0h
        volatile unsigned CLR_FALL_TRIG67; //58h-5bh, reset: 0h
        volatile unsigned INTSTAT67; // 5ch-5fh
    };
} gpio_t; //size = 40h;// 48h-4bh, reset: 0h
// From memory mapped GPIO Registers Table 12-127 am64x_tech_ref.pdf page 4981
#define MCU_GPIO (*((volatile gpio_t *)MCU_GPIO0_OFFSET))
#define GPIO0 (*((volatile gpio_t *)GPIO0_OFFSET))
#define GPIO1 (*((volatile gpio_t *)GPIO1_OFFSET))


#endif // _SYS_GPIO_H_
