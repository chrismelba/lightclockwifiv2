

#pragma once

#include <Arduino.h>

struct lightclock
{
    // ------------------------------------------------------------------------
    // Construct a Clock using colors and blendpoint
    // ------------------------------------------------------------------------
	lightclock(RgbColor hourcolorin, RgbColor minutecolorin, uint8_t blendpointin) :
		hourcolor(hourcolorin), minutecolor(minutecolorin), blendpoint(blendpointin)
		{

		};	
    // ------------------------------------------------------------------------

	void save(uint8_t partition);

    // ------------------------------------------------------------------------
	//save the clock to eeprom at the dedicated point in memory
    // ------------------------------------------------------------------------
	void load(uint8_t partition);

    // ------------------------------------------------------------------------
    //load the clock from eeprom at the dedicated point in memory
    // ------------------------------------------------------------------------
};