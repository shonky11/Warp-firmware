# 4B25: Embedded Bike Speedometer

This project for describes the designing, coding and building of a functioning prototype of a bicycle speedometer. It makes use of the Freedom development board with the KL03 microchip and with the Warp Firmware on which this code base is based. It also uses a Pimoroni PMW3901 Optical Flow sensor which measures the velocity of the bike relative to the road, as well as the SSD1331 OLED display to show the speed, time and distance data to the cyclist. The device is designed to be mounted on the handlebars of any bike with the Flow sensor pointed at the road.

## Source File Descriptions
This project has been adapted from the Warp Firmware project, from which this repository has been forked. It can be found here: https://github.com/physical-computation/Warp-firmware. The full description of all source files can be found there. Only relevant changes have been described below.


##### `boot.c`
This is the main file with the core of the firmware and which is run on boot of the Freedom board. It contains the code that is called at runtime, including the boot setup code and the main while loop. It calls upon several other libraries and device drivers.

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
The header file for the PMW3901 driver. It displays data by writing to the various registers of the display driver. The devSSD1331init() function initiaializes the display by writing the necessary values to the configuration and calibration registers of the SSD1331. These values and registers are taken from the datasheet linked below.

##### `devSSD1331.c`
This is the display driver code for the SSD1331 OLED display. It also uses SPI and hence must be written to using a different chip select pin. 

##### `devINA219.c`
This is a library for the Texas Instruments INA219 Current, Voltage and Power meter which uses an I2C connection. It is included here as it was used to measure the voltage drops and total current draw of the device and its various components.

##### `devINA219.h`
This is the header file for the INA219 driver.
