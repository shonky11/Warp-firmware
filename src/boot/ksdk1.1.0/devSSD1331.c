#include <stdint.h>
#include "config.h"

// Include charachter writing library
#include "devTEXT.h"

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

volatile uint8_t	inBuffer[32];
volatile uint8_t	payloadBytes[32];


// Override Warp's use of these pins for SSD1331 SPI connection
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 11),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

int
writePixel(uint16_t commandByte)
{
        spi_status_t status;

        /*
         *      Drive /CS low.
         *
         *      Make sure there is a high-to-low transition by first driving high, delay, then drive low.
         */
        GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

        // DC must be driven high when writing an individual pixel (command).
        GPIO_DRV_SetPinOutput(kSSD1331PinDC);


	unsigned char Byte1 = (unsigned char)((commandByte >> 8));	// Obtain first byte of colour
	unsigned char Byte2 = (unsigned char)((commandByte));		// Obtain second byte of colour

        payloadBytes[0] = Byte1;					// Send over SPI
	payloadBytes[1] = Byte2;
        status = SPI_DRV_MasterTransferBlocking(0       /* master instance */,
                                        NULL            /* spi_master_user_config_t */,
                                        (const uint8_t * restrict)&payloadBytes[0],
                                        (uint8_t * restrict)&inBuffer[0],
                                        2               /* transfer size */,
                                        1000            /* timeout in microseconds (unlike I2C which is ms) */);

        /*
         *      Drive /CS high
         */
        GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

        return status;
}


int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;



	/*
	 *	Drive /CS low.
	 *	
	 *	CS Pin is already set high so no need to set high and delay again. This should significantly speed up writes.
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}


// Writes a bunch of bytes as once to a memory buffer (collection of registers)

int
writeBuffer(uint8_t* commandBuffer, uint8_t bufferSize)
{
        spi_status_t status;

        /*
         *      Drive /CS low.
         *
         *      Make sure there is a high-to-low transition by first driving high, delay, then drive low.
         */
        GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
        GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

        /*
         *      Drive DC low (command).
         */
        GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

        status = SPI_DRV_MasterTransferBlocking(0       /* master instance */,
                                        NULL            /* spi_master_user_config_t */,
                                        (const uint8_t * restrict)commandBuffer,
                                        (uint8_t * restrict)&inBuffer[0],
                                        bufferSize               /* transfer size */,
                                        1000            /* timeout in microseconds (unlike I2C which is ms) */);

        /*
         *      Drive /CS high
         */
        GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

        return status;
}

// Displays the speed, distance and time data in large, clear letters, one at a time, sequentially, for easy reading..
void display_speed(int16_t speed, uint16_t distance, uint32_t time)
{
	reset_cursor();				// Reset the cursor to 0, 0
	writeCommand(kSSD1331CommandCLEAR);	// Clear the screen
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

    	uint8_t i;
    	uint8_t x = 0;				// Set cursor starting location
    	uint8_t y = 40;
    	
	int speed_text[8];			// Array that stores the individual speed digits and charachters to be written
	if(speed < 0)				// Can display +ve and -ve speed
	{
		speed_text[0] = '-';
	}
	else
	{
		speed_text[0] = '+';
	}
	speed_text[1] = (abs(speed) / 100) % 10 + 48;	// Extracting individual digits
	speed_text[2] = (abs(speed) / 10) % 10 + 48;
	speed_text[3] = '.';
	speed_text[4] = abs(speed) % 10 + 48;
	speed_text[5] = 'm';
	speed_text[6] = 'p';
	speed_text[7] = 'h';


	i = 0;

	for( i = 0; i < 8; i++) {			// Write characters to display
        	PutChar(x, y, speed_text[i]);
        	x += X_width;
    	}

	OSA_TimeDelay(1000);				// Delay 1s

	reset_cursor();					// Clear screen and reset cursor
	writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);


	i = 0;
	x = 0;
	y = 40;

	int dist_text[7];				// Get distance digits and display as with speed
        dist_text[0] = (distance / 1000) % 10 + 48;
        dist_text[1] = (distance / 100) % 10 + 48;
        dist_text[2] = (distance / 10) % 10 + 48;
	dist_text[3] = '.';
        dist_text[4] = distance % 10 + 48;
        dist_text[5] = 'k';
        dist_text[6] = 'm';

        i = 0;

        for( i = 0; i < 7; i++) {
                PutChar(x, y, dist_text[i]);
                x += X_width;
        }

	OSA_TimeDelay(1000);				// Delay 1s

	reset_cursor();					// Reset cursor and clear screen
        writeCommand(kSSD1331CommandCLEAR);
        writeCommand(0x00);
        writeCommand(0x00);
        writeCommand(0x5F);
        writeCommand(0x3F);


        i = 0;
        x = 0;
        y = 40;

        int time_text[8];				// Get Digits for time data (in hours, minutes, seconds)
        time_text[0] = (time / 36000) % 10 + 48;
        time_text[1] = (time / 3600) % 10 + 48;
	time_text[2] = 'h';
        time_text[3] = (time / 600) % 6 + 48;
        time_text[4] = (time / 60) % 10 + 48;
	time_text[5] = 'm';
        time_text[6] = (time / 10)  % 6 + 48;
	time_text[7] = time % 10 + 48;

        i = 0;

        for( i = 0; i < 8; i++) {			// Display time
                PutChar(x, y, time_text[i]);
                x += X_width;
        }

	OSA_TimeDelay(1000);				// Delay 1s

}


int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	warpEnableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 11u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);

	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
	//...

	//writeCommand(kSSD1331CommandDRAWRECT);
	//writeCommand(0x00);
	//writeCommand(0x00);
	//writeCommand(0x5F);
        //writeCommand(0x3F);

	//writeCommand(0x00);
	//writeCommand(0xFF);
	//writeCommand(0x00);

	//writeCommand(0x00);
        //writeCommand(0xFF);
        //writeCommand(0x00);


	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(14);

	SetFontSize(NORMAL); // set regular font size
    	foreground(toRGB(252,94,3)); // set text colour orange.

	reset_cursor();

	uint8_t i;
        uint8_t x = 40;
        uint8_t y = 0;

	// Draw a bike with characters as a splash screen :)
	char* start_image = " __    _\n  |_____)<\n (_)- (_)";

	for( i = 0; i < 29; i++)
      	{
              PutChar(x, y, (int)*(start_image + i));
              x += X_width;
      	}

	OSA_TimeDelay(3000);
	
	// Set large font size
	SetFontSize(WH);
       

	return 0;
}
