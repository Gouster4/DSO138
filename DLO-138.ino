#include <EEPROM.h>
#include "global.h"
#include "variables.h"
#include "control.h"
#include "io.h"
#include "display.h"

// ------------------------
void setup()	{
// ------------------------
	DBG_INIT(SERIAL_BAUD_RATE);
	DBG_PRINT("Dual channel O Scope with two logic channels, ver: ");
	DBG_PRINTLN(FIRMWARE_VERSION);

	// set digital and analog stuff
	initIO();
	
	// load scope config or factory reset to defaults
	loadConfig(digitalRead(BTN4) == LOW);
	
	// init the IL9341 display
	initDisplay();
}



// ------------------------
void loop()	{
// ------------------------
	controlLoop();
}
