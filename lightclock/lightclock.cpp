
#include "lightclock.h"

lightclock::save(uint8_t partition)
{
	EEPROM.begin(512);
	delay(10);
	//write the hour color
	EEPROM.write(100+partition*50, hourcolor.R);
	EEPROM.write(100+partition*50, hourcolor.G);
	EEPROM.write(100+partition*50, hourcolor.B);
	
	//write the minute color
	EEPROM.write(100+partition*50, minutecolor.R);
	EEPROM.write(100+partition*50, minutecolor.G);
	EEPROM.write(100+partition*50, minutecolor.B);
	
	//write the blend point
	EEPROM.write(100+partition*50, blendpoint);
	
	EEPROM.commit();
	delay(1000);

}


lightclock::load(uint8_t partition)
{
	EEPROM.begin(512);
	delay(10);
	//write the hour color
	hourcolor.R = EEPROM.read(100+partition*50);
	hourcolor.G = EEPROM.read(100+partition*50);
	hourcolor.B = EEPROM.read(100+partition*50);
	
	//write the minute color
	minutecolor.R = EEPROM.read(100+partition*50);
	minutecolor.G = EEPROM.read(100+partition*50);
	minutecolor.B = EEPROM.read(100+partition*50);
	
	//write the blend point
	blendpoint = EEPROM.read(100+partition*50);

}


