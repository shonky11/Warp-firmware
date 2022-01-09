# 4B25: Embedded Bike Speedometer

This project for describes the designing, coding and building of a functioning prototype of a bicycle speedometer. It makes use of the Freedom development board with the KL03 microchip and with the Warp Firmware on which this code base is based. It also uses a Pimoroni PMW3901 Optical Flow sensor which measures the velocity of the bike relative to the road, as well as the SSD1331 OLED display to show the speed, time and distance data to the cyclist. The device is designed to be mounted on the handlebars of any bike with the Flow sensor pointed at the road.

## Source File Descriptions
This project has been adapted from the Warp Firmware project, from which this repository has been forked. It can be found here: https://github.com/physical-computation/Warp-firmware. The full description of all source files can be found there. Only relevant changes have been described below.


##### `boot.c`
This is the main file with the core of the firmware and which is run on boot of the Freedom board. It contains the code that is called at runtime, including the boot setup code and the main while loop. It calls upon several other libraries and device drivers. The relevant changes are shown here, where the program for the speedometer runs. A discription of how it works is shown in the comments.

```C
	devSSD1331init(); 			// Initialize Display
	uint32_t start_time = RTC->TSR;		// Store start time on boot from RTC
	uint32_t time_elapsed = 0;		// Initialize speed, distance and time variables
	int16_t time_diff = 0;			// time_diff stores the time difference between subsequent speed readings (needed for integration)
	int16_t speed = 0;
	int32_t distance = 0;
	int16_t distance_km = 0;

	// While on loop, executed after booting.
	while (1){
		time_diff = RTC->TSR - time_elapsed - start_time;	// Calculate time difference between subsequent speed readings using RTC
		time_elapsed = RTC->TSR - start_time;			// Calculate current time elapsed
		
		// average the speed over 10 readings for greater accuracy
		int i = 0;
		speed = 0;
		for(i = 0; i < 10; ++i){
			speed += readMotionX();
			OSA_TimeDelay(10);	
		}
		speed /= 10;

		// Calculate distance by continuously adding the latest average speed * time reading (*1.609 to convert to km)
		// Then divide by seconds per hour (this is done after in a different variable so as to prevent losing the cumulative distance data.
		distance += speed * 1609 * time_diff / 1000;
		distance_km = (int16_t)(distance / 3600);
		
		display_speed(speed, distance_km, time_elapsed);	// Speed distance and time are then sent to the display driver to be displayed.
	}	

```

##### `CMakeLists.txt`
The 3 new driver files and their header files: 'devSSD1331.c' and 'devSSD1331.h'; 'devPMW3901.c' and 'devPMW3901.h'; and 'devTEXT.c' and 'devTEXT.h' have all been included here to be built by the compiler.

##### `warp.h`
Constant and data structure definitions. The PMW3901 sensor has also been defined here.

##### `devPMW3901.c`
This is the driver for the PMW3901 Optical Flow sensor. This sensor uses an SPI connection to the KL03. It measures the speed of a surface moving parallel to it in both x and y directions, measured in rad/s. These x and y speed values are stored in 2 8-bit registers each. This driver reads those values from those registers and converts the speed to miles per hour using an equation descibed in the masters thesis by Marcus Grief. It also writes various values to the many configurationg and calibration registers to get accurate readings. These are taken from the PMW3901 Datasheet, but unfortunately, since information about these registers is Adafruit's proprietary information, there is not much information about what exactly they do. The sensor is able to give readings in miles per hour accurate up to the nearest 0.1mph. Since the display also uses SPI, this has its own Chip Select Pin but shares all other SPI pins with the display. Driving this chip select low while all others are high will read and write to this peripheral.

A detailed description of the various functions and houw the work can be seen in the comments of the code.

Marcus Gried Thesis: https://lup.lub.lu.se/luur/download?func=downloadFile&recordOId=8905295&fileOId=8905299
PMW3901 Datasheet: https://www.codico.com/media/productattach/p/m/pmw3901mb-txqt_-_productbrief_2451186_7.pdf

The relevant registers for measuring the speed is as follows:
| Register      | Description                         |
|:-------------:|:-----------------------------------:|
| 0x03          | Speed in X direction (Lower Byte)   |
| 0x04          | Speed in X direction (Higher Byte)  |
| 0x05          | Speed in Y direction (Lower Byte)   |
| 0x06          | Speed in Y direction (Higher Byte)  |

The wiring for the PMW3901 to the development board is as follows:
| KL03          | PMW3901            |
|:-------------:|:------------------:|
| 5v pin        | 3-5v pin           |
| Gnd           | Gnd                |
| PTA-5         | Chip Select (CS)   |
| PTA-9         | SPI Clock (SCK)    |
| PTA-8         | MOSI pin           |
| PTA-6         | MISO pin           |
| 5V pin        | Interrupt Pin      |

The functions `getMotionX()` and `getMotionY()` are called by other programs to get the X and Y velocities in mph * 10 (i.e. 12.4 mph is returned as 124)

##### `devPMW3901.h`
The header file for the PMW3901 driver.

##### `devSSD1331.c`
This is the display driver code for the SSD1331 OLED display. It also uses an SPI connection and hence must be written to using a different chip select pin. It displays data by writing to the various registers of the display driver. The `devSSD1331init()` function initiaializes the display by writing the necessary values to the configuration and calibration registers of the SSD1331. These values and registers are taken from the datasheet linked below. The display is the initialized with a splash screen that displays a bicycle. The function `display_speed()` is used to display the speed, distance and time when passed those parameters. It is passed these parameters in the while loop of boot.c, which in turn come from devPMW3901's `getMotionX()` function. `writeCommand()` writes a value to a register of the display over SPI, and `WriteBuffer()` writes multiple of these values to a group of adjacent registers. `writePixel()` is also writes to the display over SPI, but first sets the DC pin high instead of low which allows indixidual pixels to be set to different colours.

SSD1331 Datasheet: https://cdn-shop.adafruit.com/datasheets/SSD1331_1.2.pdf

This display driver utilizes the `devText` program to write individual characters to the display. This is further described below.

The wiring for the PMW3901 to the development board is as follows:
| KL03          | PMW3901            |
|:-------------:|:------------------:|
| 5v pin        | 3-5v pin           |
| Gnd           | Gnd                |
| PTB-11        | Chip Select (CS)   |
| PTA-9         | SPI Clock (SCK)    |
| PTA-8         | MOSI pin           |
| PTB-0         | Reset              |
| PTA-12        | DC pin             |


##### `devSSD1331.h`
The header file for the SSD1331 driver.

##### `devTEXT.c`
This is a library I adapted for the Warp Firmware from the MBED library for the SSD1331 written in C++. It uses a font with each character encoded as a 6x8 bit matrix. with a 1 where a pixel is on and 0 where off. The function `PutChar()` places a character at a given x and y coordinate on the screen by itterating over the 6x8 pixel matrix and calling writePixel from the display driver to turn them on. Initially writes to the display were very slow as even pixels that were off were written to set it to the background colour. By changing it to skip these pixels and clear the screen when updating it with new characters, the write speed is significantly optimized. It now writes in about 0.2 seconds which is sufficient for these purposes. I also removed delays from writing chip select to low which significantly improved write time too. `PutChar()` calls `pixel()` to write a particular pixel with a colour. This in turn calls the `writePixel()` function in the driver.

MBED SSD1331 library: https://os.mbed.com/users/star297/code/ssd1331/

##### `devINA219.c`
This is a library for the Texas Instruments INA219 Current, Voltage and Power meter which uses an I2C connection. It is included here as it was used to measure the voltage drops and total current draw of the device and its various components.

##### `devINA219.h`
This is the header file for the INA219 driver.
