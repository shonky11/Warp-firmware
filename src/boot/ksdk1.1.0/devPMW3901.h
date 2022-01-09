int     devPMW3901init(void);
uint8_t         readSensorRegisterPMW3901(uint8_t Register_Byte);
WarpStatus      writeSensorRegisterPMW3901(uint8_t Register_Byte, uint8_t Command_Byte);
void            printSensorDataPMW3901(void);
int16_t         readMotionY(void);
int16_t         readMotionX(void);
