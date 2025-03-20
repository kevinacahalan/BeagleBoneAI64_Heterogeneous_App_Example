// This is a GPIO header for the BBAI64 to be used in conjunction with the TI J721e/TDA4VM RTOS 
// pdk CSL layer library, <ti/csl/csl_gpio.h>
//
// <ti/csl/csl_gpio.h> is located at: 
// `ti-processor-sdk-rtos-j721e-evm-10_00_00_05/pdk_jacinto_10_00_00_27/packages/ti/csl/csl_gpio.h`
//
// The compiled .a file is located at:
// `ti-processor-sdk-rtos-j721e-evm-10_00_00_05/pdk_jacinto_10_00_00_27/packages/ti/csl/lib/j721e/r5f/debug/ti.csl.aer5f`
//
// The Ti functions that you'll be calling are located in the <ti/csl/src/ip/gpio/V0/gpio.h>
// header file. That header file is included by ti/csl/csl_gpio.h, so you'll not need to include it.
// You will want to look at ti/csl/src/ip/gpio/V0/gpio.h to read the documentation. Note that some of 
// the limitations documented are incorrect for BBAI64, be aware of that.
//
//
// Example of flipping gpio pin P8_08 on:
//
// #include <ti/csl/csl_gpio.h>
// #include <ai64/bbai64_gpio.h>
// GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_08), GPIO_PIN_NUM(P8_08), GPIO_DIRECTION_OUTPUT);
// GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_08), GPIO_PIN_NUM(P8_08), GPIO_PIN_HIGH);

#ifndef BBAI64_GPIO_H
#define BBAI64_GPIO_H


// The macros that you probably don't need to touch
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define GPIO_BASE_ADDR_DETAIL(x) GPIO##x##_BASE_ADDR
#define GET_GPIO_BASE_ADDR_FROM_BLOCK_NUM(BLOCK_NUM) GPIO_BASE_ADDR_DETAIL(BLOCK_NUM)
#define GPIO_PIN_BLOCK_NUM(pin) CONCATENATE(pin, _GPIO_BLOCK_NUMBER)

// The macros that you want to use
#define GPIO_PIN_NUM(pin)   CONCATENATE(pin, _GPIO_NUMBER)
#define GPIO_PIN_BASE_ADDR(pin) GET_GPIO_BASE_ADDR_FROM_BLOCK_NUM(GPIO_PIN_BLOCK_NUM(pin))




// Table 2-2. GPIO Registers......(aka the base addresses of the difference GPIO instances)
#define GPIO0_BASE_ADDR 0x00600000  // On the BBAI64 Most of the exposed GPIO pins are in this instance
#define GPIO2_BASE_ADDR 0x00610000

// Bases address for the Table 2-3. GPIO Registers, all low bit defines are 
// the same as with the table above
#define GPIO1_BASE_ADDR   0x00601000 // On the BBAI64 there are 7 exposed GPIO pins in this instance
#define GPIO3_BASE_ADDR   0x00611000

// Bases address for the Table 2-4. GPIO Registers, all low bit defines are 
// the same as with the table above
#define GPIO4_BASE_ADDR   0x00620000
#define GPIO6_BASE_ADDR   0x00630000

// Bases address for the Table 2-5. GPIO Registers, all low bit defines are 
// the same as with the table above
#define GPIO5_BASE_ADDR 0x00621000
#define GPIO7_BASE_ADDR 0x00680000

// Bases address for the Table 2-6. GPIO Registers, all low bit defines are 
// the same as with the table above
#define WKUP_GPIO0_BASE_ADDR 0x42110000
#define WKUP_GPIO1_GPIO7_BASE_ADDR 0x42100000

// Block zero GPIO pins
#define P9_11_GPIO_BLOCK_NUMBER 0
#define P9_11_GPIO_NUMBER 1
#define P9_13_GPIO_BLOCK_NUMBER 0
#define P9_13_GPIO_NUMBER 2
#define P8_17_GPIO_BLOCK_NUMBER 0
#define P8_17_GPIO_NUMBER 3
#define P8_18_GPIO_BLOCK_NUMBER 0
#define P8_18_GPIO_NUMBER 4
#define P8_22_GPIO_BLOCK_NUMBER 0
#define P8_22_GPIO_NUMBER 5
#define P8_24_GPIO_BLOCK_NUMBER 0
#define P8_24_GPIO_NUMBER 6
#define P8_34_GPIO_BLOCK_NUMBER 0
#define P8_34_GPIO_NUMBER 7
#define P8_36_GPIO_BLOCK_NUMBER 0
#define P8_36_GPIO_NUMBER 8
#define P8_38b_GPIO_BLOCK_NUMBER 0
#define P8_38b_GPIO_NUMBER 9
#define P9_23_GPIO_BLOCK_NUMBER 0
#define P9_23_GPIO_NUMBER 10
#define P8_37b_GPIO_BLOCK_NUMBER 0
#define P8_37b_GPIO_NUMBER 11
#define P9_26b_GPIO_BLOCK_NUMBER 0
#define P9_26b_GPIO_NUMBER 12
#define P9_24b_GPIO_BLOCK_NUMBER 0
#define P9_24b_GPIO_NUMBER 13
#define P8_08_GPIO_BLOCK_NUMBER 0
#define P8_08_GPIO_NUMBER 14
#define P8_07_GPIO_BLOCK_NUMBER 0
#define P8_07_GPIO_NUMBER 15
#define P8_10_GPIO_BLOCK_NUMBER 0
#define P8_10_GPIO_NUMBER 16
#define P8_09_GPIO_BLOCK_NUMBER 0
#define P8_09_GPIO_NUMBER 17
#define P9_42b_GPIO_BLOCK_NUMBER 0
#define P9_42b_GPIO_NUMBER 18
#define P8_03_GPIO_BLOCK_NUMBER 0
#define P8_03_GPIO_NUMBER 20
#define P8_35a_GPIO_BLOCK_NUMBER 0
#define P8_35a_GPIO_NUMBER 24
#define P8_33a_GPIO_BLOCK_NUMBER 0
#define P8_33a_GPIO_NUMBER 25
#define P8_32a_GPIO_BLOCK_NUMBER 0
#define P8_32a_GPIO_NUMBER 26
#define P9_17a_GPIO_BLOCK_NUMBER 0
#define P9_17a_GPIO_NUMBER 28
#define P8_21_GPIO_BLOCK_NUMBER 0
#define P8_21_GPIO_NUMBER 30
#define P8_23_GPIO_BLOCK_NUMBER 0
#define P8_23_GPIO_NUMBER 31
#define P8_31a_GPIO_BLOCK_NUMBER 0
#define P8_31a_GPIO_NUMBER 32
#define P8_05_GPIO_BLOCK_NUMBER 0
#define P8_05_GPIO_NUMBER 33
#define P8_06_GPIO_BLOCK_NUMBER 0
#define P8_06_GPIO_NUMBER 34
#define P8_25_GPIO_BLOCK_NUMBER 0
#define P8_25_GPIO_NUMBER 35
#define P9_22a_GPIO_BLOCK_NUMBER 0
#define P9_22a_GPIO_NUMBER 38
#define P9_21a_GPIO_BLOCK_NUMBER 0
#define P9_21a_GPIO_NUMBER 39
#define P9_18a_GPIO_BLOCK_NUMBER 0
#define P9_18a_GPIO_NUMBER 40
#define P9_28b_GPIO_BLOCK_NUMBER 0
#define P9_28b_GPIO_NUMBER 43
#define P9_30b_GPIO_BLOCK_NUMBER 0
#define P9_30b_GPIO_NUMBER 44
#define P9_12_GPIO_BLOCK_NUMBER 0
#define P9_12_GPIO_NUMBER 45
#define P9_27a_GPIO_BLOCK_NUMBER 0
#define P9_27a_GPIO_NUMBER 46
#define P9_15_GPIO_BLOCK_NUMBER 0
#define P9_15_GPIO_NUMBER 47
#define P8_04_GPIO_BLOCK_NUMBER 0
#define P8_04_GPIO_NUMBER 48
#define P9_33b_GPIO_BLOCK_NUMBER 0
#define P9_33b_GPIO_NUMBER 50
#define P8_26_GPIO_BLOCK_NUMBER 0
#define P8_26_GPIO_NUMBER 51
#define P9_31b_GPIO_BLOCK_NUMBER 0
#define P9_31b_GPIO_NUMBER 52
#define P9_29b_GPIO_BLOCK_NUMBER 0
#define P9_29b_GPIO_NUMBER 53
#define P9_39b_GPIO_BLOCK_NUMBER 0
#define P9_39b_GPIO_NUMBER 54
#define P9_35b_GPIO_BLOCK_NUMBER 0
#define P9_35b_GPIO_NUMBER 55
#define P9_36b_GPIO_BLOCK_NUMBER 0
#define P9_36b_GPIO_NUMBER 56
#define P9_37b_GPIO_BLOCK_NUMBER 0
#define P9_37b_GPIO_NUMBER 57
#define P9_38b_GPIO_BLOCK_NUMBER 0
#define P9_38b_GPIO_NUMBER 58
#define P8_12_GPIO_BLOCK_NUMBER 0
#define P8_12_GPIO_NUMBER 59
#define P8_11_GPIO_BLOCK_NUMBER 0
#define P8_11_GPIO_NUMBER 60
#define P8_15_GPIO_BLOCK_NUMBER 0
#define P8_15_GPIO_NUMBER 61
#define P8_16_GPIO_BLOCK_NUMBER 0
#define P8_16_GPIO_NUMBER 62
#define P8_31b_GPIO_BLOCK_NUMBER 0
#define P8_31b_GPIO_NUMBER 63
#define P8_32b_GPIO_BLOCK_NUMBER 0
#define P8_32b_GPIO_NUMBER 64
#define P8_43_GPIO_BLOCK_NUMBER 0
#define P8_43_GPIO_NUMBER 65
#define P8_44_GPIO_BLOCK_NUMBER 0
#define P8_44_GPIO_NUMBER 66
#define P8_41_GPIO_BLOCK_NUMBER 0
#define P8_41_GPIO_NUMBER 67
#define P8_42_GPIO_BLOCK_NUMBER 0
#define P8_42_GPIO_NUMBER 68
#define P8_39_GPIO_BLOCK_NUMBER 0
#define P8_39_GPIO_NUMBER 69
#define P8_40_GPIO_BLOCK_NUMBER 0
#define P8_40_GPIO_NUMBER 70
#define P8_27_GPIO_BLOCK_NUMBER 0
#define P8_27_GPIO_NUMBER 71
#define P8_28_GPIO_BLOCK_NUMBER 0
#define P8_28_GPIO_NUMBER 72
#define P8_29_GPIO_BLOCK_NUMBER 0
#define P8_29_GPIO_NUMBER 73
#define P8_30_GPIO_BLOCK_NUMBER 0
#define P8_30_GPIO_NUMBER 74
#define P8_14_GPIO_BLOCK_NUMBER 0
#define P8_14_GPIO_NUMBER 75
#define P8_20_GPIO_BLOCK_NUMBER 0
#define P8_20_GPIO_NUMBER 76
#define P9_20b_GPIO_BLOCK_NUMBER 0
#define P9_20b_GPIO_NUMBER 77
#define P9_19b_GPIO_BLOCK_NUMBER 0
#define P9_19b_GPIO_NUMBER 78
#define P8_45_GPIO_BLOCK_NUMBER 0
#define P8_45_GPIO_NUMBER 79
#define P8_46_GPIO_BLOCK_NUMBER 0
#define P8_46_GPIO_NUMBER 80
#define P9_40b_GPIO_BLOCK_NUMBER 0
#define P9_40b_GPIO_NUMBER 81
#define P8_19_GPIO_BLOCK_NUMBER 0
#define P8_19_GPIO_NUMBER 88
#define P8_13_GPIO_BLOCK_NUMBER 0
#define P8_13_GPIO_NUMBER 89
#define P9_21b_GPIO_BLOCK_NUMBER 0
#define P9_21b_GPIO_NUMBER 90
#define P9_22b_GPIO_BLOCK_NUMBER 0
#define P9_22b_GPIO_NUMBER 91
#define P9_14_GPIO_BLOCK_NUMBER 0
#define P9_14_GPIO_NUMBER 93
#define P9_16_GPIO_BLOCK_NUMBER 0
#define P9_16_GPIO_NUMBER 94
#define P9_25b_GPIO_BLOCK_NUMBER 0
#define P9_25b_GPIO_NUMBER 104
#define P8_38a_GPIO_BLOCK_NUMBER 0
#define P8_38a_GPIO_NUMBER 105
#define P8_37a_GPIO_BLOCK_NUMBER 0
#define P8_37a_GPIO_NUMBER 106
#define P8_33b_GPIO_BLOCK_NUMBER 0
#define P8_33b_GPIO_NUMBER 111
#define P9_17b_GPIO_BLOCK_NUMBER 0
#define P9_17b_GPIO_NUMBER 115
#define P8_35b_GPIO_BLOCK_NUMBER 0
#define P8_35b_GPIO_NUMBER 116
#define P9_26a_GPIO_BLOCK_NUMBER 0
#define P9_26a_GPIO_NUMBER 118
#define P9_24a_GPIO_BLOCK_NUMBER 0
#define P9_24a_GPIO_NUMBER 119
#define P9_18b_GPIO_BLOCK_NUMBER 0
#define P9_18b_GPIO_NUMBER 120
#define P9_42a_GPIO_BLOCK_NUMBER 0
#define P9_42a_GPIO_NUMBER 123
#define P9_27b_GPIO_BLOCK_NUMBER 0
#define P9_27b_GPIO_NUMBER 124
#define P9_25a_GPIO_BLOCK_NUMBER 0
#define P9_25a_GPIO_NUMBER 127

// Block 1 GPIO pins
#define P9_41_GPIO_BLOCK_NUMBER 1
#define P9_41_GPIO_NUMBER 0
#define P9_19a_GPIO_BLOCK_NUMBER 1
#define P9_19a_GPIO_NUMBER 1
#define P9_20a_GPIO_BLOCK_NUMBER 1
#define P9_20a_GPIO_NUMBER 2
#define P9_28a_GPIO_BLOCK_NUMBER 1
#define P9_28a_GPIO_NUMBER 11
#define P9_31a_GPIO_BLOCK_NUMBER 1
#define P9_31a_GPIO_NUMBER 12
#define P9_30a_GPIO_BLOCK_NUMBER 1
#define P9_30a_GPIO_NUMBER 13
#define P9_29a_GPIO_BLOCK_NUMBER 1
#define P9_29a_GPIO_NUMBER 14




// How that math works... (what goes on inside the TI library)
//
// The GPIO pin P8_08 corresponds to gpio0_14, aka gpio instance 0 pin 14...( gpio[instance_number]_[pin_number] )
// To find the correct registers to manipulate this pin, we divide the GPIO number by 32.
//
// So 14 / 32 = 0
//
// If the result is:
// 0, we use the "01" registers for pins 0 to 31.    (the case for P8_08)
// 1, we use the "23" registers for pins 32 to 63.
// 2, we use the "45" registers for pins 64 to 95.
//
// We thus use the 01 registers for P8_08
//
// To find the bit we want to manipulate we do [pin_number] % 32. For p8_08 we do 14 % 32
// which is 14.
//
// So, to control P8_08 we will to manipulate bit 14 in the "01" registers on GPIO instance 0

#define P8_08_REG_BIT 1<<14
#define P8_08_ENABLE_REG (GPIO0_BASE_ADDR | GPIO_DIR01)
#define P8_08_SET_REG (GPIO0_BASE_ADDR | GPIO_SET_DATA01)
#define P8_08_CLEAR_REG (GPIO0_BASE_ADDR | GPIO_CLR_DATA01)
#define P8_08_ENABLE (*(volatile uint32_t *)(P8_08_ENABLE_REG) &= ~P8_08_REG_BIT)
#define P8_08_SET (*(volatile uint32_t *)(P8_08_SET_REG) = P8_08_REG_BIT)
#define P8_08_CLEAR (*(volatile uint32_t *)(P8_08_CLEAR_REG) = P8_08_REG_BIT)
#define P8_08_WRITE(value) do { \
    if (value) { \
        P8_08_SET; \
    } else { \
        P8_08_CLEAR; \
    } \
} while (0)


#endif // BBAI64_GPIO_H


