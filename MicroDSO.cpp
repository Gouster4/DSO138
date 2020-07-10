
#include "MicroDSO.h"
#include "variables.h"
#include "io.h"
#include "display.h"
#include "capture.h"
#include "zconfig.h"

uint8_t triggerType;


void captureDisplayCycle(boolean wTimeOut);


// ------------------------
void setTriggerType(uint8_t tType)	{
// ------------------------
	triggerType = tType;
	// break any running capture loop
	keepSampling = false;
}

// ------------------------
void MicroDSO_Setup(void)  {
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
void MicroDSO_Loop(void)	{
// ------------------------
	// start by reading the state of analog system
	readInpSwitches();

	if(triggerType == TRIGGER_AUTO)	{
		captureDisplayCycle(true);
	}
	
	else if(triggerType == TRIGGER_NORM)	{
		captureDisplayCycle(false);
	}
	
	else	{
		// single trigger
		clearWaves();
		indicateCapturing();
		// blocking call - until trigger
		sampleWaves(false);
		indicateCapturingDone();
		hold = true;
		// request repainting of screen labels in next draw cycle
		repaintLabels();
		// draw the waveform
		drawWaves();
		blinkLED();
		// dump captured data on serial port
		dumpSamples();

		// freeze display
		while(hold);
		
		// update display indicating hold released
		drawLabels();
	}

	// process any long pending operations which cannot be serviced in ISR
}




// ------------------------
void captureDisplayCycle(boolean wTimeOut)	{
// ------------------------
	indicateCapturing();
	// blocking call - until timeout or trigger
	sampleWaves(wTimeOut);
	// draw the waveform
	indicateCapturingDone();
	drawWaves();
	// inter wait before next sampling
	if(triggered)
		blinkLED();
	
	if(hold)	{
		// update UI labels
		drawLabels();
		// dump captured data on serial port
		dumpSamples();
	}
	
	// freeze display if requested
	while(hold);
}
