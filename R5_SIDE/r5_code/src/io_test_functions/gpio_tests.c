#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ti/csl/csl_gpio.h>
#include <ti/csl/soc.h>

#include <ai64/bbai64_gpio.h>
#include <ai64/sys_gpio.h>

#include <io_test_functions/gpio_tests.h>


#include <ti/csl/csl_gpio.h>
#include <ti/csl/csl_gpioAux.h>
#include <ti/csl/cslr_gpio.h>


#include <ti/csl/arch/r5/interrupt.h>  // for Intc_* APIs
// ... other headers ...

// #define GPIO0_BASE_ADDR   (/* base address of GPIO0 on J721E, from soc header */)
// #define PIN_NUMBER        (80U)   // We're focusing on gpio0_80
#define INTR_NUM          (257)

static void myGpioISR(void *arg);


// I have not gotten this stuff to work yet. I have been spending too much time.
void gpioIntrTestInit(void)
{
    // 1. Set the direction as input
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03), GPIO_DIRECTION_INPUT);

    // 2. Configure the interrupt trigger. For example, rising-edge:
    GPIOSetIntrType_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03), GPIO_INTR_MASK_RISE_EDGE);

    // 3. Enable the interrupt in GPIO (bank-level interrupt, plus edge detection).
    GPIOIntrEnable_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03), GPIO_INTR_MASK_RISE_EDGE);

    // 4. At the interrupt controller (VIM) level, register our ISR.
    Intc_IntRegister(INTR_NUM, myGpioISR, NULL);

    // 5. Enable the interrupt at the VIM.
    Intc_IntEnable(INTR_NUM);

    // 6. Globally enable CPU interrupts (IRQ/FIQ) if not already done.
    Intc_SystemEnable();

    // Now, whenever the GPIO0_20 (P8_03) sees a rising edge, it should call myGpioISR().
}

static void myGpioISR(void *arg)
{
    (void)arg;
    // 1. Clear the interrupt status in GPIO
    GPIOIntrClear_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03));

    // 2. Do something: toggle an LED, log a message, etc.
    //    For example:
    printf("GPIO0_80 triggered a rising edge interrupt!\n");
}


#define PIN_MOSI   P8_09
#define PIN_SCLK   P8_08
#define PIN_CS     P8_06

void bitbang_spi_init() {
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_SCLK), GPIO_PIN_NUM(PIN_SCLK), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_MOSI), GPIO_PIN_NUM(PIN_MOSI), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_CS), GPIO_PIN_NUM(PIN_CS), GPIO_DIRECTION_OUTPUT);

    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_SCLK), GPIO_PIN_NUM(PIN_SCLK), GPIO_PIN_LOW);
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_MOSI), GPIO_PIN_NUM(PIN_MOSI), GPIO_PIN_LOW);
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_CS), GPIO_PIN_NUM(PIN_CS), GPIO_PIN_HIGH);
}

void bitbang_spi_send(uint8_t data) {
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_CS), GPIO_PIN_NUM(PIN_CS), GPIO_PIN_LOW);  // Assert CS

    for (int i = 0; i < 8; i++) {
        // Write bit to MOSI
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_MOSI), GPIO_PIN_NUM(PIN_MOSI), (data & 0x80) ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
        data <<= 1;
        for(volatile int j=0; j<15; j++);
        // Pulse SCLK
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_SCLK), GPIO_PIN_NUM(PIN_SCLK), GPIO_PIN_HIGH);
        for(volatile int j=0; j<15; j++);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_SCLK), GPIO_PIN_NUM(PIN_SCLK), GPIO_PIN_LOW);
    }

    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_CS), GPIO_PIN_NUM(PIN_CS), GPIO_PIN_HIGH);  // De-assert CS
    for(volatile int i=0; i< 20; i++);
}

void bitbang_spi_send_array(uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        bitbang_spi_send(data[i]);
    }
}

// I got bit banged SPI running at around 2.5Mhz, going higher gets flaky. Playing with
// the delay loops could possibly squeeze out a slightly higher clock
void test_bitbang_spi() {
    bitbang_spi_init();
    
    uint8_t data[] = {0xaa,0x55,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x55,0xaa};

    for(int i = 0; i < 100; i++){
        for(volatile int j=0; j< 1000; j++);
        bitbang_spi_send_array(data, sizeof(data)/sizeof(data[0]));
    }
}

void test_register_level_gpio() {
    GPIO0.DIR01_bit.OE_bit15 = 0; // P8_7 GPIO0_15
    GPIO0.DIR23_bit.OE_bit30 = 0; // P8_16 GPIO0_62
    GPIO0.SET_DATA01_bit.SD_bit15 = 1; // P8_7 GPIO0_15
    GPIO0.OUT_DATA23_bit.DO_bit30 = 1; // P8_16 GPIO0_62
    while(1)
    {
        for(volatile int i = 0; i<50000; i++);
        GPIO0.OUT_DATA01_bit.DO_bit15 = 1; // P8_7  GPIO0_15 HI
        GPIO0.OUT_DATA23_bit.DO_bit30 = 1; // P8_16 GPIO0_62 HI
        for(volatile int i = 0; i<50000; i++);
        GPIO0.OUT_DATA01_bit.DO_bit15 = 0; // P8_7  GPIO0_15 LO
        GPIO0.OUT_DATA23_bit.DO_bit30 = 0; // P8_16 GPIO0_62 LO
    }
}

void runToggle (int n)
{
    #define DELAY 20000000

    printf ("\n");
    printf ("Inside runToggle \n");
    printf ("At %d\n", __LINE__);

    // set pin directions
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_45), GPIO_PIN_NUM(P8_45), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_44), GPIO_PIN_NUM(P8_44), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_43), GPIO_PIN_NUM(P8_43), GPIO_DIRECTION_OUTPUT);

    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_04), GPIO_PIN_NUM(P8_04), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_05), GPIO_PIN_NUM(P8_05), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(P8_07), GPIO_PIN_NUM(P8_07), GPIO_DIRECTION_OUTPUT);

    printf ("At %d, the LED loop\n", __LINE__);
    for(int i = 0;i < n; i++)
    {
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_45), GPIO_PIN_NUM(P8_45), GPIO_PIN_HIGH);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_44), GPIO_PIN_NUM(P8_44), GPIO_PIN_LOW);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_43), GPIO_PIN_NUM(P8_43), GPIO_PIN_HIGH);

        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03), GPIO_PIN_HIGH);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_04), GPIO_PIN_NUM(P8_04), GPIO_PIN_LOW);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_05), GPIO_PIN_NUM(P8_05), GPIO_PIN_HIGH);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_07), GPIO_PIN_NUM(P8_07), GPIO_PIN_LOW);
        for(volatile int j=0; j< DELAY; j++);

        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_45), GPIO_PIN_NUM(P8_45), GPIO_PIN_LOW);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_44), GPIO_PIN_NUM(P8_44), GPIO_PIN_HIGH);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_43), GPIO_PIN_NUM(P8_43), GPIO_PIN_LOW);

        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_03), GPIO_PIN_NUM(P8_03), GPIO_PIN_LOW);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_04), GPIO_PIN_NUM(P8_04), GPIO_PIN_HIGH);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_05), GPIO_PIN_NUM(P8_05), GPIO_PIN_LOW);
        GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(P8_07), GPIO_PIN_NUM(P8_07), GPIO_PIN_HIGH);

        for(volatile int j=0; j< DELAY; j++);

        printf ("At %d, i = %d\n", __LINE__, i);
    }
    printf ("Done runToggle\n");
	return;
}