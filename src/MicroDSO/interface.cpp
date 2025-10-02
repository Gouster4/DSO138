
#include "hal.h"
#include "io.h"
#include "variables.h"
#include "display.h"
#include "MicroDSO.h"
#include "interface.h"
#include "zconfig.h"
#include "capture.h"

void resetParam(void);
void changeXCursor(int16_t xPos);
void changeYCursor(uint8_t num, int16_t yPos);
void calculateTraceZero(int waveID);
void incrementTimeBase(void);
void decrementTimeBase(void);
void incrementTT(void);
void decrementTT(void);
void setTriggerRising(void);
void setTriggerFalling(void);
void incrementTLevel(void);
void decrementTLevel(void);
void incrementWaves(void);
void decrementWaves(void);

// ------------------------
const char* getTimebaseLabel(void)	{
// ------------------------
	return tbNames[currentTimeBase];
}



// interface operations defined below

long lastBtnPress = 0;



// ------------------------
void btn4ISR(void)	{
// ------------------------
	static bool pressed = false;
	static long pressedTime = 0;

	// btn pressed or released?
	if(!pressed && (digitalRead(BTN4) == LOW))	{
		// debounce
		if(millis() - pressedTime < BTN_DEBOUNCE_TIME)
			return;
		pressedTime = millis();
		pressed = true;
	}
	
	
	if(pressed && (digitalRead(BTN4) == HIGH))	{
		// debounce
		if(millis() - pressedTime < 5)
			return;
		
		pressed = false;
		
		// is it a short press
		if(millis() - pressedTime < 1000)	{
			// toggle hold
			hold = !hold;
			repaintLabels();
		}
		else	{
			// long press reset parameter to default
			resetParam();
		}
	}
}



// ------------------------
void readESwitchISR(void)	{
// ------------------------
	// debounce
	if(millis() - lastBtnPress < BTN_DEBOUNCE_TIME)
		return;
	lastBtnPress = millis();

	// select different parameters to change
	focusNextLabel();
	
	// request repainting of screen labels
	repaintLabels();
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}


// ------------------------
void cycleBufferSize(void) {
// ------------------------
    // Free existing buffers if they exist
    if(ch1Capture) {
        free(ch1Capture);
        free(ch2Capture);
        free(bitStore);
        ch1Capture = ch2Capture = bitStore = NULL;
    }
    
    // Cycle to next buffer mode
    bufferMode = (bufferMode + 1) % 4;
    currentBufferSize = bufferSizes[bufferMode];
    
    // Allocate new buffers
    ch1Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    ch2Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    bitStore = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    
    // Initialize buffers to zero
    if(ch1Capture && ch2Capture && bitStore) {
        memset(ch1Capture, 0, currentBufferSize * sizeof(uint16_t));
        memset(ch2Capture, 0, currentBufferSize * sizeof(uint16_t));
        memset(bitStore, 0, currentBufferSize * sizeof(uint16_t));
    } else {
        // Handle allocation failure
        DBG_PRINTLN("Buffer allocation failed!");
        // Fall back to full buffer
        bufferMode = BUF_FULL;
        currentBufferSize = NUM_SAMPLES;
        // Re-allocate full buffers
        ch1Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
        ch2Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
        bitStore = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    }
    
    // Reset sampling indices
    sIndex = 0;
    tIndex = 0;
    triggered = false;
    
    // Adjust xCursor to stay within new buffer bounds
    uint16_t maxXCursor = (currentBufferSize > GRID_WIDTH) ? 
                         (currentBufferSize - GRID_WIDTH) : 0;
    if(xCursor > maxXCursor) {
        xCursor = maxXCursor;
    }
    
    // Check if current zoom is compatible with new buffer size
    adjustZoomForBufferSize();
    
    saveParameter(PARAM_BUFSIZE, bufferMode);
    repaintLabels();
    
    DBG_PRINT("Buffer changed to: ");
    DBG_PRINT(currentBufferSize);
    DBG_PRINT(" samples (");
    DBG_PRINT(currentBufferSize * 3 * 2);
    DBG_PRINTLN(" bytes)");
}

// ------------------------
void initializeBuffers(void) {
// ------------------------
    // Allocate initial buffers
    ch1Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    ch2Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    bitStore = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    
    if(ch1Capture && ch2Capture && bitStore) {
        memset(ch1Capture, 0, currentBufferSize * sizeof(uint16_t));
        memset(ch2Capture, 0, currentBufferSize * sizeof(uint16_t));
        memset(bitStore, 0, currentBufferSize * sizeof(uint16_t));
        DBG_PRINT("Initial buffers allocated: ");
        DBG_PRINT(currentBufferSize);
        DBG_PRINTLN(" samples");
    } else {
        DBG_PRINTLN("CRITICAL: Initial buffer allocation failed!");
    }
}

// ------------------------
void adjustZoomForBufferSize(void) {
// ------------------------
    // Calculate minimum zoom that can be displayed with current buffer
    uint16_t minZoomForBuffer = calculateMinZoomForBuffer();
    
    if(zoomFactor < minZoomForBuffer) {
        // Current zoom is too high for buffer, adjust to maximum possible
        zoomFactor = minZoomForBuffer;
        saveParameter(PARAM_ZOOM, zoomFactor);
        adjustXCursorForZoom();
    }
}

// ------------------------
uint16_t calculateMinZoomForBuffer(void) {
// ------------------------
    // Returns the minimum zoom factor that can display the entire buffer
    // We want at least GRID_WIDTH samples visible
    if(currentBufferSize <= GRID_WIDTH) {
        return 10; // 1.0x - can't zoom out further
    }
    
    // Calculate the zoom needed to fit the buffer on screen
    float requiredZoom = (float)GRID_WIDTH / currentBufferSize * 10.0;
    uint16_t minZoom = (uint16_t)requiredZoom;
    
    // Ensure we don't go below minimum zoom
    if(minZoom < ZOOM_MIN) {
        return ZOOM_MIN;
    }
    
    return minZoom;
}

// ------------------------
void incrementZoom(void) {
// ------------------------
    uint16_t newZoom = zoomFactor;
    
    if(zoomFactor < ZOOM_MAX) {
        if(zoomFactor < 10) {
            // 0.1x to 0.9x: increment by 1 (0.1 steps)
            newZoom++;
        } else {
            // 1x to 9x: increment by 10 (1.0 steps)
            newZoom += 10;
            // Don't exceed maximum (100 = 10x)
            if(newZoom > ZOOM_MAX) newZoom = ZOOM_MAX;
        }
        
        // Check if new zoom is compatible with current buffer size
        if(isZoomCompatibleWithBuffer(newZoom)) {
            zoomFactor = newZoom;
            saveParameter(PARAM_ZOOM, zoomFactor);
            repaintLabels();
        }
    }
}

// ------------------------
void decrementZoom(void) {
// ------------------------
    uint16_t newZoom = zoomFactor;
    
    if(zoomFactor > ZOOM_MIN) {
        if(zoomFactor <= 10) {
            // 0.2x to 1.0x: decrement by 1 (0.1 steps)
            newZoom--;
        } else {
            // 2x to 10x: decrement by 10 (1.0 steps)
            newZoom -= 10;
            // Don't go below 1.0x
            if(newZoom < 10) newZoom = 10;
        }
        
        // Always allow zooming in (higher zoom factors)
        // Only check compatibility when zooming out
        if(newZoom > zoomFactor || isZoomCompatibleWithBuffer(newZoom)) {
            zoomFactor = newZoom;
            saveParameter(PARAM_ZOOM, zoomFactor);
            repaintLabels();
        }
    }
}

// ------------------------
void adjustXCursorForZoom(void) {
// ------------------------
    float zoomMultiplier = getZoomMultiplier();
    uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
    uint16_t maxXCursor = (currentBufferSize > visibleSamples) ? 
                         (currentBufferSize - visibleSamples) : 0;
    
    if(xCursor > maxXCursor) {
        xCursor = maxXCursor;
        saveParameter(PARAM_XCURSOR, xCursor);
    }
}

// ------------------------
float getZoomMultiplier(void) {
// ------------------------
    // Convert internal zoom factor to multiplier
    // zoomFactor 10 = 1.0x, 20 = 2.0x, 5 = 0.5x, etc.
    return zoomFactor / 10.0;
}

// ------------------------
bool isZoomCompatibleWithBuffer(uint16_t zoom) {
// ------------------------
    float zoomMultiplier = zoom / 10.0;
    uint16_t requiredSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
    return (requiredSamples <= currentBufferSize);
}

// ------------------------
void resetParam(void)	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
		case L_triggerLevel:
			// set trigger level to 0
			setTriggerLevel(0);
			saveParameter(PARAM_TLEVEL, 0);
			repaintLabels();
			break;
		case L_zoom:
			zoomFactor = ZOOM_DEFAULT;
			saveParameter(PARAM_ZOOM, zoomFactor);
			repaintLabels();
			break;
		case L_window:
			// set x in the middle
			changeXCursor((NUM_SAMPLES - GRID_WIDTH)/2);
			break;
		case L_vPos1:
			// zero the trace base
			calculateTraceZero(0);
			changeYCursor(0, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos2:
			// zero the trace base
			calculateTraceZero(1);
			changeYCursor(1, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos3:
			changeYCursor(2, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos4:
			changeYCursor(3, -GRID_HEIGHT/2 - 1);
			break;
		case L_triggerType:
			saveParameter(PARAM_PREAMBLE, PREAMBLE_VALUE, true); //salve parameters to flash
			clearWaves();
			break;
		case L_bufferSize:
			cycleBufferSize();
			break;
		default:
			// toggle stats printing
			printStats = !printStats;
			saveParameter(PARAM_STATS, printStats);
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}




// ------------------------
void calculateTraceZero(int waveID)		{
// ------------------------
	// calculate zero only if switch is in GND position
	if(couplingPos != CPL_GND)
		return;

	if(waveID > 1)
		return;
	
	uint16_t *wave = (waveID == 0)? ch1Capture : ch2Capture;
	
	// zero the trace
	int32_t sumSamples = 0;

	for(uint16_t k = 0; k < NUM_SAMPLES; k++)	{
		sumSamples += wave[k];
	}

	uint16_t Vavr = sumSamples/NUM_SAMPLES;

	if(waveID == 0)	{
		zeroVoltageA1 = Vavr;
		saveParameter(PARAM_ZERO1, zeroVoltageA1);
	}
	else	{
		zeroVoltageA2 = Vavr;
		saveParameter(PARAM_ZERO2, zeroVoltageA2);
	}
}




// ------------------------
void encoderChanged(int steps)	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
		case L_timebase:
			if(steps > 0) decrementTimeBase(); else	incrementTimeBase();
			break;
		case L_triggerType:
			if(steps > 0) incrementTT(); else decrementTT();
			break;
		case L_triggerEdge:
			if(steps > 0) setTriggerRising(); else setTriggerFalling();
			break;
		case L_bufferSize:  
			cycleBufferSize();
			break;
		case L_triggerLevel:
			if(steps > 0) incrementTLevel(); else decrementTLevel();
			break;
		case L_waves:
			if(steps > 0) incrementWaves(); else decrementWaves();
			break;
		case L_window:
			if(steps > 0) changeXCursor(xCursor + XCURSOR_STEP); else changeXCursor(xCursor - XCURSOR_STEP);
			break;
		case L_zoom:
			if(steps > 0) incrementZoom(); else decrementZoom();
			break;
		case L_vPos1:
			if(steps > 0) changeYCursor(0, yCursors[0] - YCURSOR_STEP); else changeYCursor(0, yCursors[0] + YCURSOR_STEP);
			break;
		case L_vPos2:
			if(steps > 0) changeYCursor(1, yCursors[1] - YCURSOR_STEP); else changeYCursor(1, yCursors[1] + YCURSOR_STEP);
			break;
		case L_vPos3:
			if(steps > 0) changeYCursor(2, yCursors[2] - YCURSOR_STEP); else changeYCursor(2, yCursors[2] + YCURSOR_STEP);
			break;
		case L_vPos4:
			if(steps > 0) changeYCursor(3, yCursors[3] - YCURSOR_STEP); else changeYCursor(3, yCursors[3] + YCURSOR_STEP);
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}



// ------------------------
void incrementTLevel(void)	{
// ------------------------
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL + 5);
	saveParameter(PARAM_TLEVEL, tL);
	repaintLabels();
}



// ------------------------
void decrementTLevel(void)	{
// ------------------------
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL - 5);
	saveParameter(PARAM_TLEVEL, tL);
	repaintLabels();
}


void incrementWaves(void) {
    // Cycles through analog modes: A1+A2, A1, A2, none
    // waves[0] = A1, waves[1] = A2
    if (waves[0] && waves[1]) {
        // Current state: A1 + A2 -> Next state: A1
        waves[1] = false;
        saveParameter(PARAM_WAVES + 1, waves[1]);
    } else if (waves[0]) {
        // Current state: A1 -> Next state: A2
        waves[0] = false;
        waves[1] = true;
        saveParameter(PARAM_WAVES + 0, waves[0]);
        saveParameter(PARAM_WAVES + 1, waves[1]);
    } else if (waves[1]) {
        // Current state: A2 -> Next state: none
        waves[1] = false;
        saveParameter(PARAM_WAVES + 1, waves[1]);
    } else {
        // Current state: none -> Next state: A1 + A2
        waves[0] = true;
        waves[1] = true;
        saveParameter(PARAM_WAVES + 0, waves[0]);
        saveParameter(PARAM_WAVES + 1, waves[1]);
    }

    repaintLabels();
}

// ------------------------

void decrementWaves(void) {
    // Cycles through digital modes: D1+D2, D1, D2, none
    // waves[2] = D1, waves[3] = D2
    if (waves[2] && waves[3]) {
        // Current state: D1 + D2 -> Next state: D1
        waves[3] = false;
        saveParameter(PARAM_WAVES + 3, waves[3]);
    } else if (waves[2]) {
        // Current state: D1 -> Next state: D2
        waves[2] = false;
        waves[3] = true;
        saveParameter(PARAM_WAVES + 2, waves[2]);
        saveParameter(PARAM_WAVES + 3, waves[3]);
    } else if (waves[3]) {
        // Current state: D2 -> Next state: none
        waves[3] = false;
        saveParameter(PARAM_WAVES + 3, waves[3]);
    } else {
        // Current state: none -> Next state: D1 + D2
        waves[2] = true;
        waves[3] = true;
        saveParameter(PARAM_WAVES + 2, waves[2]);
        saveParameter(PARAM_WAVES + 3, waves[3]);
    }

    repaintLabels();
}

// ------------------------
void setTriggerRising(void)	{
// ------------------------
	if(triggerRising)
		return;
	
	triggerRising=true;
	saveParameter(PARAM_TRIGDIR, triggerRising);
	repaintLabels();
        setTriggerRising(triggerRising);
}



// ------------------------
void setTriggerFalling(void)	{
// ------------------------
	if(!triggerRising)
		return;
	
	triggerRising=false;
	saveParameter(PARAM_TRIGDIR, triggerRising);
	repaintLabels();
        setTriggerRising(triggerRising);
}




// ------------------------
void incrementTT(void)	{
// ------------------------
	if(triggerType == TRIGGER_SINGLE)
		return;
	
	setTriggerType(triggerType + 1);
	// trigger type is not saved
	// saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}



// ------------------------
void decrementTT(void)	{
// ------------------------
	if(triggerType == TRIGGER_AUTO)
		return;
	setTriggerType(triggerType - 1);
	// trigger type is not saved
	// saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}




// ------------------------
void incrementTimeBase(void)	{
// ------------------------
	if(currentTimeBase == T50MS)
		return;
	
	setTimeBase(currentTimeBase + 1);
}



// ------------------------
void decrementTimeBase(void)	{
// ------------------------
	if(currentTimeBase == T20US)
		return;
	
	setTimeBase(currentTimeBase - 1);
}



// ------------------------
void setTimeBase(uint8_t timeBase, bool save) {
// ------------------------
	currentTimeBase = timeBase;
	setSamplingRate(timeBase);
	if(save) saveParameter(PARAM_TIMEBASE, currentTimeBase);
	// request repainting of screen labels
	repaintLabels();
}




// ------------------------
void toggleWave(uint8_t num)	{
// ------------------------
	waves[num] = !waves[num];
	saveParameter(PARAM_WAVES + num, waves[num]);
	repaintLabels();
}



// ------------------------
void changeYCursor(uint8_t num, int16_t yPos)	{
// ------------------------
	if(yPos > 0)
		yPos = 0;
	
	if(yPos < -GRID_HEIGHT)
		yPos = -GRID_HEIGHT;

	yCursors[num] = yPos;
	saveParameter(PARAM_YCURSOR + num, yCursors[num]);
	repaintLabels();
}



// ------------------------
void changeXCursor(int16_t xPos) {
// ------------------------
    // Calculate maximum xCursor based on current buffer and zoom
    float zoomMultiplier = getZoomMultiplier();
    uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
    uint16_t maxXCursor = (currentBufferSize > visibleSamples) ? 
                         (currentBufferSize - visibleSamples) : 0;
    
    if(xPos < 0)
        xPos = 0;
    
    if(xPos > maxXCursor)
        xPos = maxXCursor;
    
    xCursor = xPos;
    saveParameter(PARAM_XCURSOR, xCursor);
    repaintLabels();
}

