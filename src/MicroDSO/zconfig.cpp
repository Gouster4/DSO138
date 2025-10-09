#include "hal.h"
#include "variables.h" 
#include "MicroDSO.h"
#include "io.h"
#include "capture.h"
#include "interface.h"
#include "zconfig.h"
#include "display.h"

#include "src/EEPROM/eeprom.h"

EEPROM_ EEPROM;

// zconfig: since we are referencing variables defined in other files 


void formatSaveConfig(void);


// ------------------------
void loadConfig(bool reset)	{
// ------------------------
	DBG_PRINTLN("Loading stored config...");

	if(EEPROM.init() != EEPROM_OK)	{
		loadDefaults();
		formatSaveConfig();
		return;
	}
	
	// read preamble
	if(reset || (EEPROM.read(PARAM_PREAMBLE) != PREAMBLE_VALUE))	{
		loadDefaults();
		formatSaveConfig();
		return;
	}	
	
	// load all the parameters from EEPROM
	uint16_t data;
	
	data = EEPROM.read(PARAM_TIMEBASE);
	setTimeBase(data, false);
	
	data = EEPROM.read(PARAM_TRIGTYPE);
	if(data <= TRIGGER_SINGLE) {
		setTriggerType(data);
	} else {
		setTriggerType(TRIGGER_AUTO); // Default if invalid
	}

	data = EEPROM.read(PARAM_TRIGDIR);
	setTriggerRising(data == 1);
	
	data = EEPROM.read(PARAM_XCURSOR);
	xCursor = data;
	
	data = EEPROM.read(PARAM_YCURSOR);
	yCursors[0] = data;
	
	data = EEPROM.read(PARAM_YCURSOR + 1);
	yCursors[1] = data;
	
	data = EEPROM.read(PARAM_YCURSOR + 2);
	yCursors[2] = data;

	data = EEPROM.read(PARAM_YCURSOR + 3);
	yCursors[3] = data;

	data = EEPROM.read(PARAM_WAVES);
	waves[0] = data;
	
	data = EEPROM.read(PARAM_WAVES + 1);
	waves[1] = data;
	
	data = EEPROM.read(PARAM_WAVES + 2);
	waves[2] = data;
	
	data = EEPROM.read(PARAM_WAVES + 3);
	waves[3] = data;
	
	data = EEPROM.read(PARAM_OPERATION_MODE);
	if(data <= MODE_XY) {
		operationMode = data;
	} else {
		operationMode = MODE_OSCILLOSCOPE;
	}

	// Apply mode-specific settings
	setOperationMode(operationMode);  // This will initialize properly
	
	data = EEPROM.read(PARAM_TLEVEL);
	setTriggerLevel(data);
	
	printStats = EEPROM.read(PARAM_STATS);
	zeroVoltageA1 = EEPROM.read(PARAM_ZERO1);
	zeroVoltageA2 = EEPROM.read(PARAM_ZERO2);
	
	// Load buffer size setting
	data = EEPROM.read(PARAM_BUFSIZE);
	if(data <= BUF_EIGHTH) {
		bufferMode = data;
	} else {
		bufferMode = BUF_FULL;
	}
	currentBufferSize = bufferSizes[bufferMode];
	
	// Free any existing buffers and allocate new ones
	if(ch1Capture) {
		free(ch1Capture);
		free(ch2Capture);
		free(bitStore);
	}
	ch1Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
	ch2Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
	bitStore = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
	
	if(ch1Capture && ch2Capture && bitStore) {
		memset(ch1Capture, 0, currentBufferSize * sizeof(uint16_t));
		memset(ch2Capture, 0, currentBufferSize * sizeof(uint16_t));
		memset(bitStore, 0, currentBufferSize * sizeof(uint16_t));
	}
	// Adjust xCursor to stay within new buffer bounds
	uint16_t maxXCursor = (currentBufferSize > GRID_WIDTH) ? 
                     (currentBufferSize - GRID_WIDTH) : 0;
	if(xCursor > maxXCursor) {
		xCursor = maxXCursor;
	}

	// Check zoom compatibility
	adjustZoomForBufferSize();
	
	// Load zoom factor
	data = EEPROM.read(PARAM_ZOOM);
	if(data >= ZOOM_MIN && data <= ZOOM_MAX) {
		zoomFactor = data;
	} else {
		zoomFactor = ZOOM_DEFAULT;
	}
	
	data = EEPROM.read(PARAM_TAILLENGTH);
	if(data <= currentBufferSize - 1) {
		tailLength = data;
	} else {
		tailLength = min(DEFAULT_TAIL_LENGTH, currentBufferSize - 1);
		saveParameter(PARAM_TAILLENGTH, tailLength, false);
	}
    
	data = EEPROM.read(PARAM_XYLINES);
	xylines = (data != 0);
	
	DBG_PRINTLN("Loaded config:");
	DBG_PRINT("Timebase: ");DBG_PRINTLN(currentTimeBase);
	DBG_PRINT("Trigger Rising: ");DBG_PRINTLN(triggerRising);
	DBG_PRINT("Trigger Type: ");DBG_PRINTLN(triggerType);
	DBG_PRINT("X Cursor Pos: ");DBG_PRINTLN(xCursor);
	DBG_PRINT("Y Cursors: ");DBG_PRINT(yCursors[0]);DBG_PRINT(", ");DBG_PRINT(yCursors[1]);DBG_PRINT(", ");DBG_PRINT(yCursors[2]);DBG_PRINT(", ");DBG_PRINTLN(yCursors[3]);
	DBG_PRINT("Waves: ");DBG_PRINT(waves[0]);DBG_PRINT(", ");DBG_PRINT(waves[1]);DBG_PRINT(", ");DBG_PRINT(waves[2]);DBG_PRINT(", ");DBG_PRINTLN(waves[3]);
	DBG_PRINT("Trigger Level: ");DBG_PRINTLN(data);
	DBG_PRINT("Print Stats: ");DBG_PRINTLN(printStats);
	DBG_PRINT("Wave1 Zero: ");DBG_PRINTLN(zeroVoltageA1);
	DBG_PRINT("Wave2 Zero: ");DBG_PRINTLN(zeroVoltageA2);
	DBG_PRINT("Buffer mode: ");DBG_PRINTLN(bufferModeNames[bufferMode]);
	DBG_PRINT("Buffer Size: ");DBG_PRINTLN(bufferModeNames[bufferMode]);
	DBG_PRINT("Zoom Factor: ");DBG_PRINTLN(zoomFactor);
	DBG_PRINT("Tail Length: "); DBG_PRINTLN(tailLength);
	DBG_PRINT("XY Lines: "); DBG_PRINTLN(xylines);
}



// ------------------------
void loadDefaults(void)	{
// ------------------------
	DBG_PRINTLN("Loading defaults");

	setTimeBase(T30US);
	setTriggerRising(true);
	setTriggerType(TRIGGER_AUTO);
	setTriggerLevel(0);
	
	// set x in the middle
	xCursor = (NUM_SAMPLES - GRID_WIDTH)/2;
 
	// set y in the middle
	yCursors[0] = -70;
	yCursors[1] = -90;
	yCursors[2] = -110;
	yCursors[3] = -130;
	
	// show all waves
	waves[0] = true;
	waves[1] = true;
	waves[2] = true;
	waves[3] = true;
	
	printStats = false;
	
	zeroVoltageA1 = 1985;
	zeroVoltageA2 = 1985;
	bufferMode = BUF_FULL;
	currentBufferSize = NUM_SAMPLES;
	operationMode = MODE_OSCILLOSCOPE;
	zoomFactor = ZOOM_DEFAULT;
	tailLength = DEFAULT_TAIL_LENGTH;
	xylines = false;
}



// ------------------------
void formatSaveConfig(void)	{
// ------------------------
	DBG_PRINTLN("Formatting EEPROM");
	EEPROM.format();
	DBG_PRINTLN("Saving all config params....");
	
	saveParameter(PARAM_PREAMBLE, PREAMBLE_VALUE, false);
	
	saveParameter(PARAM_TIMEBASE, currentTimeBase, false);
	saveParameter(PARAM_TRIGTYPE, triggerType, false);
	saveParameter(PARAM_TRIGDIR, triggerRising, false);
	saveParameter(PARAM_XCURSOR, xCursor, false);
	saveParameter(PARAM_YCURSOR, yCursors[0], false);
	saveParameter(PARAM_YCURSOR + 1, yCursors[1], false);
	saveParameter(PARAM_YCURSOR + 2, yCursors[2], false);
	saveParameter(PARAM_YCURSOR + 3, yCursors[3], false);
	
	saveParameter(PARAM_WAVES, waves[0], false);
	saveParameter(PARAM_WAVES + 1, waves[1], false);
	saveParameter(PARAM_WAVES + 2, waves[2], false);
	saveParameter(PARAM_WAVES + 3, waves[3], false);
	
	saveParameter(PARAM_OPERATION_MODE, operationMode, false);
	
	saveParameter(PARAM_TLEVEL, getTriggerLevel(), false);
 	saveParameter(PARAM_STATS, printStats, false);
	
	saveParameter(PARAM_ZERO1, zeroVoltageA1, false);
	saveParameter(PARAM_ZERO2, zeroVoltageA2, true);
	saveParameter(PARAM_BUFSIZE, bufferMode, false);
	saveParameter(PARAM_ZOOM, zoomFactor, false);
	saveParameter(PARAM_TAILLENGTH, tailLength, false);
    saveParameter(PARAM_XYLINES, xylines, true);
}



// ------------------------
void saveParameter(uint16_t param, uint16_t data, bool flash_write)	{
// ------------------------

	uint16_t status = EEPROM.write(param, data, flash_write);
	if(status != EEPROM_OK)	{
		DBG_PRINT("Unable to save param in EEPROM, code: ");DBG_PRINTLN(status);
	}
}
