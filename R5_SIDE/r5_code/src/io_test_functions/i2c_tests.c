#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti/csl/soc.h>
#include <ti/drv/i2c/src/csl/csl_i2c.h>
#include <ti/drv/i2c/i2c.h>

#define I2C_INSTANCE 2  // Use I2C instance 2
#define I2C_SLAVE_ADDRESS 0x68  // Slave address

I2C_Handle initI2C()
{
    I2C_Params i2cParams;
    I2C_Handle i2cHandle;

    // Initialize the I2C driver
    I2C_init();

    // Set default parameters
    I2C_Params_init(&i2cParams);
    i2cParams.transferMode = I2C_MODE_BLOCKING;
    i2cParams.bitRate = I2C_100kHz;

    // Open I2C instance
    i2cHandle = I2C_open(I2C_INSTANCE, &i2cParams);
    if (i2cHandle == NULL)
    {
        printf("Failed to open I2C instance\n");
        return NULL;
    }

    uint32_t i2cDelay = 5000;
    I2C_control(i2cHandle, I2C_CMD_RECOVER_BUS, &i2cDelay); // call I2C_v1_recoverBus() from i2c_api.c

    return i2cHandle;
}

int bcdToDecimal(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void readTime(I2C_Handle i2cHandle)
{
    I2C_Transaction i2cTransaction;
    uint8_t txBuffer[1] = {0x00};  // Register address to start reading from
    uint8_t rxBuffer[3];

    // Set up the write transaction to set the register address
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = sizeof(txBuffer);
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;
    i2cTransaction.slaveAddress = I2C_SLAVE_ADDRESS;
    i2cTransaction.timeout = 5000; // Increase timeout for testing purposes

    // Perform the I2C write transaction
    int status = I2C_transfer(i2cHandle, &i2cTransaction);
    if (status != I2C_STS_SUCCESS)
    {
        printf("Failed to write register address to I2C device. Status code: %d\n", status);
        return;
    }

    // Set up the read transaction to get the data
    i2cTransaction.writeBuf = NULL;
    i2cTransaction.writeCount = 0;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = sizeof(rxBuffer);
    i2cTransaction.slaveAddress = I2C_SLAVE_ADDRESS;
    i2cTransaction.timeout = 5000; // Increase timeout for testing purposes

    // Perform the I2C read transaction
    status = I2C_transfer(i2cHandle, &i2cTransaction);
    if (status == I2C_STS_SUCCESS)
    {
        int second = bcdToDecimal(rxBuffer[0]);
        int minute = bcdToDecimal(rxBuffer[1]);
        int hour = bcdToDecimal(rxBuffer[2] & 0x3F);  // Mask to handle 24-hour mode

        printf("Time: %02d:%02d:%02d\n", hour, minute, second);
    }
    else if (status == I2C_STS_ERR_TIMEOUT)
    {
        printf("I2C transfer timed out. Possible causes: incorrect slave address, bus issue, or missing pull-up resistors.\n");
    }
    else
    {
        printf("Failed to read time from I2C device. Status code: %d\n", status);
    }
}

void testI2C()
{
    // Initialize I2C
    I2C_Handle i2cHandle = initI2C();
    if (i2cHandle == NULL)
    {
        return;
    }

    // Read the time once
    readTime(i2cHandle);

    // Close the I2C instance
    I2C_close(i2cHandle);
}
