void		initINA219(const uint8_t i2cAddress, uint16_t operatingVoltageMillivolts);
WarpStatus	readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes);
WarpStatus	writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload);
WarpStatus	configureSensorINA219(uint16_t payload_CONF, uint16_t payload_CAL);
void		printSensorDataINA219(bool hexModeFlag);
