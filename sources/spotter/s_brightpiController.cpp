#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "s_brightpiController.h"

static void initI2C(I2CBus* brightPi) {
    //----- OPEN THE I2C BUS -----
    char *filename = (char*)"/dev/i2c-1";
    if ((brightPi->file_i2c = open(filename, O_RDWR)) < 0)
    {
        //ERROR HANDLING: you can check errno to see what went wrong
        printf("Failed to open the i2c bus");
        return;
    }

    int addr = 0x70;          //<<<<<The I2C address of the slave
    if (ioctl(brightPi->file_i2c, I2C_SLAVE, addr) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        //ERROR HANDLING; you can check errno to see what went wrong
        return;
    }
}

static void switchIROn(I2CBus* brightPi) {
    i32 length = 0;
    unsigned char buffer[2] = {};

    //----- Turn on all IR LEDs -----
    buffer[0] = 0x00;
    buffer[1] = 0xa5;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Turn gain up to max -----
    buffer[0] = 0x09;
    buffer[1] = 0x0f;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 1. IR Group -----
    buffer[0] = 0x01;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 2. IR Group -----
    buffer[0] = 0x03;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 3. IR Group -----
    buffer[0] = 0x06;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 4. IR Group -----
    buffer[0] = 0x08;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }
}

static void switchVisableOn(I2CBus* brightPi) {
    i32 length = 0;
    unsigned char buffer[2] = {};

    //----- Turn on all IR LEDs -----
    buffer[0] = 0x00;
    buffer[1] = 0x5a;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Turn gain up to max -----
    buffer[0] = 0x09;
    buffer[1] = 0x0f;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 1. IR Group -----
    buffer[0] = 0x02;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 2. IR Group -----
    buffer[0] = 0x04;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 3. IR Group -----
    buffer[0] = 0x05;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }

    //----- Set max brightness for 4. IR Group -----
    buffer[0] = 0x07;
    buffer[1] = 0x39;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }
}

static void switchLEDsOff(I2CBus* brightPi) {
    i32 length = 0;
    unsigned char buffer[2] = {};
    
    //----- Switch off all LEDs -----
    buffer[0] = 0x00;
    buffer[1] = 0x00;
    length = 2;			
    if (write(brightPi->file_i2c, buffer, length) != length)		
    {
        printf("Failed to write to the i2c bus.\n");
    }
}


