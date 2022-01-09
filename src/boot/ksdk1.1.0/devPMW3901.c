#include <stdint.h>

/*
 *      config.h needs to come first
 */
#include "config.h"

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devPMW3901.h"

volatile uint8_t        inBuffer[];
volatile uint8_t        payloadBytes[];


/*
 *      Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
        kPMW3901PinMOSI         = GPIO_MAKE_PIN(HW_GPIOA, 8),
        kPMW3901PinSCK          = GPIO_MAKE_PIN(HW_GPIOA, 9),
        kPMW3901PinCSn          = GPIO_MAKE_PIN(HW_GPIOA, 5),		// Here we use a different chip select pin to the SSD1331 Display
        kPMW3901PinMISO         = GPIO_MAKE_PIN(HW_GPIOA, 6),		// And also require use of the MISO pin to read
};


// Writes to PMW3901 registers
WarpStatus
writeSensorRegisterPMW3901(uint8_t Register_Byte, uint8_t Command_Byte)
{
        spi_status_t status;

        warpScaleSupplyVoltage(1800);
	
	// All other SPI Chip Selects should be high already (mainly the SSD1331 needs to be high)

        /*
                Drive /CS low.
                Set high first to creat high to low transition
        */

        GPIO_DRV_SetPinOutput(kPMW3901PinCSn);
        OSA_TimeDelay(10);
        GPIO_DRV_ClearPinOutput(kPMW3901PinCSn);

        uint8_t Byte1 = (Register_Byte | 0x80);		// First bit has to be a 1 so as to write instead of read
        uint8_t Byte2 = Command_Byte;

        payloadBytes[0] = Byte1;
        payloadBytes[1] = Byte2;

        
        status = SPI_DRV_MasterTransferBlocking(0       /* master instance */,
                                                                                        NULL    /* spi_master_user_config_t*/,
                                                                                        (const uint8_t * restrict)&payloadBytes,
                                                                                        (uint8_t * restrict) &inBuffer,
                                                                                        2       /* transfer size */,
                                                                                        2000    /* timeout in microseconds */);

        /*
                Drive /CS high
        */

        OSA_TimeDelay(10);
        GPIO_DRV_SetPinOutput(kPMW3901PinCSn);

                if (status != kStatus_SPI_Success)
        {
                return kWarpStatusDeviceCommunicationFailed;
        }

        return kWarpStatusOK;
}


// Read the registers of the PMW3901
uint8_t
readSensorRegisterPMW3901(uint8_t Register_Byte)       /* W_R = 0/1 (Write/Read) */
{
        spi_status_t status;

        warpScaleSupplyVoltage(1800);

       	// All other SPI CS pins should already be high

        /*
                Drive /CS low.
                CS should already be high so no need to set high again as this causes delays.
        */

        GPIO_DRV_ClearPinOutput(kPMW3901PinCSn);

        uint8_t Byte1 = (Register_Byte & (~0x80));	// First bit needs to be 0 for a read from registers
        uint8_t Byte2 = 0x00;				// Second byte is 0 for read

        payloadBytes[0] = Byte1;
        payloadBytes[1] = Byte2;

        
        status = SPI_DRV_MasterTransferBlocking(0       /* master instance */,
                                                                                        NULL    /* spi_master_user_config_t*/,
                                                                                        (const uint8_t * restrict)&payloadBytes,
                                                                                        (uint8_t * restrict) &inBuffer,
                                                                                        2       /* transfer size */,
                                                                                        2000    /* timeout in microseconds */);

        /*
         *      Drive /CS high
         */
        OSA_TimeDelay(10);
        GPIO_DRV_SetPinOutput(kPMW3901PinCSn);

                if (status != kStatus_SPI_Success)
        {
                return kWarpStatusDeviceCommunicationFailed;
        }

        return inBuffer[1];	// Returns 8-bit registers (value stored in second byte)
}


// Read optical flow speed in X direction (forward)
int16_t
readMotionX(void)
{
    	int16_t deltaX;
	int16_t speedX;

    	readSensorRegisterPMW3901(0x02);
	//  Read from 2 8-bit registers (0x04 is Higher byte, 0x03 is Lower) and combine into 16 bit signed int
    	deltaX = ((int16_t)readSensorRegisterPMW3901(0x04) << 8) | readSensorRegisterPMW3901(0x03);
    	speedX =  deltaX * 10/ 18.3; 	// (converted to miles per hour * 10) 
					//Calibration factor =  1 / (2.237 * 0.0733 (radians) / (30 (pixels) * 0.01 (s imestep)))  

    	return speedX;
}

// Read optical flow in Y direction
int16_t
readMotionY(void)
{
    int16_t deltaY;

    readSensorRegisterPMW3901(0x02);
    //	Read from 2 8-bit registers (0x06 is Higher byte, 0x05 is Lower) and combine into 16 bit signed int
    deltaY = ((int16_t)readSensorRegisterPMW3901(0x06) << 8) | readSensorRegisterPMW3901(0x05);

    return deltaY;
}

int
devPMW3901init(void)
{
	// Set all other SPI chip selects high
        warpDeasserAllSPIchipSelects();
        /*
         *      Override Warp firmware's use of these pins.
         *
         *      Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
         */
        PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
        PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);
        PORT_HAL_SetMuxMode(PORTA_BASE, 6u, kPortMuxAlt3);
        
        warpEnableSPIpins();
        /*
         *      Override Warp firmware's use of these pins.
         *
         *      Reconfigure to use as GPIO.
         */
        PORT_HAL_SetMuxMode(PORTA_BASE, 5u, kPortMuxAsGpio);

	// Config and Calibration registers written to with values described in the datasheet
	// These are adafruit's proprietary values and hence are not explained well in the datasheet
        writeSensorRegisterPMW3901(0x3A, 0x5A);
        OSA_TimeDelay(5);
        writeSensorRegisterPMW3901(0x7F, 0x00);
        writeSensorRegisterPMW3901(0x61, 0xAD);
        writeSensorRegisterPMW3901(0x7F, 0x03);
        writeSensorRegisterPMW3901(0x40, 0x00);
        writeSensorRegisterPMW3901(0x7F, 0x05);
        writeSensorRegisterPMW3901(0x41, 0xB3);
        writeSensorRegisterPMW3901(0x43, 0xF1);
        writeSensorRegisterPMW3901(0x45, 0x14);
        writeSensorRegisterPMW3901(0x5B, 0x32);
        writeSensorRegisterPMW3901(0x5F, 0x34);
        writeSensorRegisterPMW3901(0x7B, 0x08);
        writeSensorRegisterPMW3901(0x7F, 0x06);
        writeSensorRegisterPMW3901(0x44, 0x1B);
        writeSensorRegisterPMW3901(0x40, 0xBF);
        writeSensorRegisterPMW3901(0x4E, 0x3F);
        writeSensorRegisterPMW3901(0x7F, 0x08);
        writeSensorRegisterPMW3901(0x65, 0x20);
        writeSensorRegisterPMW3901(0x6A, 0x18);
        writeSensorRegisterPMW3901(0x7F, 0x09);
        writeSensorRegisterPMW3901(0x4F, 0xAF);
        writeSensorRegisterPMW3901(0x5F, 0x40);
        writeSensorRegisterPMW3901(0x48, 0x80);
        writeSensorRegisterPMW3901(0x49, 0x80);
        writeSensorRegisterPMW3901(0x57, 0x77);
        writeSensorRegisterPMW3901(0x60, 0x78);
        writeSensorRegisterPMW3901(0x61, 0x78);
        writeSensorRegisterPMW3901(0x62, 0x08);
        writeSensorRegisterPMW3901(0x63, 0x50);
        writeSensorRegisterPMW3901(0x7F, 0x0A);
        writeSensorRegisterPMW3901(0x45, 0x60);
        writeSensorRegisterPMW3901(0x7F, 0x00);
        writeSensorRegisterPMW3901(0x4D, 0x11);
        writeSensorRegisterPMW3901(0x55, 0x80);
        writeSensorRegisterPMW3901(0x74, 0x1F);
        writeSensorRegisterPMW3901(0x75, 0x1F);
        writeSensorRegisterPMW3901(0x4A, 0x78);
        writeSensorRegisterPMW3901(0x4B, 0x78);
        writeSensorRegisterPMW3901(0x44, 0x08);
        writeSensorRegisterPMW3901(0x45, 0x50);
        writeSensorRegisterPMW3901(0x64, 0xFF);
        writeSensorRegisterPMW3901(0x65, 0x1F);
        writeSensorRegisterPMW3901(0x7F, 0x14);
        writeSensorRegisterPMW3901(0x65, 0x60);
        writeSensorRegisterPMW3901(0x66, 0x08);
        writeSensorRegisterPMW3901(0x63, 0x78);
        writeSensorRegisterPMW3901(0x7F, 0x15);
        writeSensorRegisterPMW3901(0x48, 0x58);
        writeSensorRegisterPMW3901(0x7F, 0x07);
        writeSensorRegisterPMW3901(0x41, 0x0D);
        writeSensorRegisterPMW3901(0x43, 0x14);
        writeSensorRegisterPMW3901(0x4B, 0x0E);
        writeSensorRegisterPMW3901(0x45, 0x0F);
        writeSensorRegisterPMW3901(0x44, 0x42);
        writeSensorRegisterPMW3901(0x4C, 0x80);
        writeSensorRegisterPMW3901(0x7F, 0x10);
        writeSensorRegisterPMW3901(0x5B, 0x02);
        writeSensorRegisterPMW3901(0x7F, 0x07);
        writeSensorRegisterPMW3901(0x40, 0x41);
        writeSensorRegisterPMW3901(0x70, 0x00);

        OSA_TimeDelay(10);
        writeSensorRegisterPMW3901(0x32, 0x44);
        writeSensorRegisterPMW3901(0x7F, 0x07);
        writeSensorRegisterPMW3901(0x40, 0x40);
        writeSensorRegisterPMW3901(0x7F, 0x06);
        writeSensorRegisterPMW3901(0x62, 0xf0);
        writeSensorRegisterPMW3901(0x63, 0x00);
        writeSensorRegisterPMW3901(0x7F, 0x0D);
        writeSensorRegisterPMW3901(0x48, 0xC0);
        writeSensorRegisterPMW3901(0x6F, 0xd5);
        writeSensorRegisterPMW3901(0x7F, 0x00);
        writeSensorRegisterPMW3901(0x5B, 0xa0);
        writeSensorRegisterPMW3901(0x4E, 0xA8);
        writeSensorRegisterPMW3901(0x5A, 0x50);
        writeSensorRegisterPMW3901(0x40, 0x80);

	// Turn on LED for better sensing
        OSA_TimeDelay(240);
        writeSensorRegisterPMW3901(0x7f, 0x14);
        writeSensorRegisterPMW3901(0x6f, 0x1c);
        writeSensorRegisterPMW3901(0x7f, 0x00);

        return 0;
}

// Print sensor data to computer for testing
void printSensorDataPMW3901(void)
{
        int16_t        dX;
        int16_t        dY;

        dX = readMotionX();
        dY = readMotionY();

        warpPrint(" %d,", dX);
        warpPrint(" %d,", dY);
        warpPrint(" 0x%02x 0x%02x,", readSensorRegisterPMW3901(0x04), readSensorRegisterPMW3901(0x03));
        warpPrint(" 0x%02x 0x%02x,", readSensorRegisterPMW3901(0x06), readSensorRegisterPMW3901(0x05));

}

        
