
#include "hal.h"
#include "io.h"
#include "variables.h"
#include "display.h"
#include "MicroDSO.h"
#include "interface.h"
#include "zconfig.h"
#include "capture.h"
#include "spectrum.h"

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
uint16_t calculateMaxZoomForBuffer(void) {
// ------------------------
    // Calculate maximum zoom based on buffer capacity
    float zoomMultiplier = (float)currentBufferSize / GRID_WIDTH;
    uint16_t maxZoom = (uint16_t)(zoomMultiplier * 10);
    
    if(maxZoom > ZOOM_MAX) return ZOOM_MAX;
    if(maxZoom < ZOOM_MIN) return ZOOM_MIN;
    return maxZoom;
}

// ------------------------
bool isZoomCompatibleWithBuffer(uint16_t zoom) {
// ------------------------
    float zoomMultiplier = zoom / 10.0;
    uint16_t requiredSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
    return (requiredSamples <= currentBufferSize);
}

// ------------------------
void incrementBufferSize(void) {
// ------------------------
    if(bufferMode > BUF_FULL) {
        uint8_t newBufferMode = bufferMode - 1;
        changeBufferSize(newBufferMode);
    }
}

// ------------------------
void decrementBufferSize(void) {
// ------------------------
    if(bufferMode < BUF_EIGHTH) {
        uint8_t newBufferMode = bufferMode + 1;
        changeBufferSize(newBufferMode);
    }
}

// ------------------------
void changeBufferSize(uint8_t newBufferMode) {
// ------------------------
    // Store current values
    uint16_t oldZoom = zoomFactor;
    uint16_t oldTail = tailLength;
    
    // Free existing buffers
    if(ch1Capture) {
        free(ch1Capture);
        free(ch2Capture);
        free(bitStore);
        ch1Capture = ch2Capture = bitStore = NULL;
    }
    
    // Set new buffer
    bufferMode = newBufferMode;
    currentBufferSize = bufferSizes[bufferMode];
    
    // Allocate new buffers
    ch1Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    ch2Capture = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    bitStore = (uint16_t*)malloc(currentBufferSize * sizeof(uint16_t));
    
    // Initialize buffers
    if(ch1Capture && ch2Capture && bitStore) {
        memset(ch1Capture, 0, currentBufferSize * sizeof(uint16_t));
        memset(ch2Capture, 0, currentBufferSize * sizeof(uint16_t));
        memset(bitStore, 0, currentBufferSize * sizeof(uint16_t));
    } else {
        // Fallback to full buffer
        bufferMode = BUF_FULL;
        currentBufferSize = NUM_SAMPLES;
        // Re-allocate...
    }
    
    // Reset sampling
    sIndex = 0;
    tIndex = 0;
    triggered = false;
    
    // Adjust zoom if needed
    uint16_t maxZoom = calculateMaxZoomForBuffer();
    if(zoomFactor > maxZoom) {
        zoomFactor = maxZoom;
    }
    
	if(tailLength >= currentBufferSize) {
    tailLength = currentBufferSize - 1;
    saveParameter(PARAM_TAILLENGTH, tailLength);
    }
    // Adjust xCursor
    adjustXCursorForZoom();
    
    saveParameter(PARAM_BUFSIZE, bufferMode);
    saveParameter(PARAM_ZOOM, zoomFactor);
    saveParameter(PARAM_TAILLENGTH, tailLength);
    repaintLabels();
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
uint16_t calculateMaxTailForBuffer(void) {
// ------------------------
    // Maximum tail length is buffer size minus 1 (for current point)
    return currentBufferSize - 1;
}

// ------------------------
bool isTailCompatibleWithBuffer(uint16_t tail) {
// ------------------------
    return (tail <= currentBufferSize - 1);
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
    float zoomMultiplier = getZoomMultiplier();
    uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
    
    // Calculate TRUE center position in the buffer (0 to currentBufferSize-1)
    uint16_t centerSample = xCursor + visibleSamples / 2;
    
    uint16_t newZoom = zoomFactor;
    
    if(zoomFactor < ZOOM_MAX) {
        if(zoomFactor < 10) {
            newZoom++;
        } else {
            newZoom = (newZoom/10)*10;
            newZoom += 10;
            if(newZoom > ZOOM_MAX) newZoom = ZOOM_MAX;
        }
        
        if(!isZoomCompatibleWithBuffer(newZoom)) {
            if(bufferMode > BUF_FULL) {
                uint16_t desiredZoom = newZoom;
                incrementBufferSize();
                zoomFactor = desiredZoom;
                saveParameter(PARAM_ZOOM, zoomFactor);
                adjustXCursorForZoom();
                repaintLabels();
                return;
            } else {
                newZoom = calculateMaxZoomForBuffer();
            }
        }
        
        if(newZoom != zoomFactor) {
            zoomFactor = newZoom;
            
            // Calculate new visible samples with new zoom
            float newZoomMultiplier = getZoomMultiplier();
            uint16_t newVisibleSamples = (uint16_t)(GRID_WIDTH * newZoomMultiplier);
            
            // Calculate new xCursor to maintain the SAME CENTER position
            uint16_t newXCursor = centerSample - newVisibleSamples / 2;
            
            // Ensure xCursor stays within bounds
            uint16_t maxXCursor = (currentBufferSize > newVisibleSamples) ? 
                                 (currentBufferSize - newVisibleSamples) : 0;
            if(newXCursor > maxXCursor) newXCursor = maxXCursor;
            if((int16_t)newXCursor < 0) newXCursor = 0;
            
            xCursor = newXCursor;
            
            saveParameter(PARAM_ZOOM, zoomFactor);
            saveParameter(PARAM_XCURSOR, xCursor);
            repaintLabels();
        }
    }
}

// ------------------------
void decrementZoom(void) {
// ------------------------
    float zoomMultiplier = getZoomMultiplier();
    uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
    
    // Calculate TRUE center position in the buffer
    uint16_t centerSample = xCursor + visibleSamples / 2;
    
    uint16_t newZoom = zoomFactor;
    
    if(zoomFactor > ZOOM_MIN) {
        if(zoomFactor <= 10) {
            newZoom--;
        } else {
            newZoom = (newZoom/10)*10;
            newZoom -= 10;
            if(newZoom < 10) newZoom = 10;
        }
        
        if(newZoom != zoomFactor) {
            zoomFactor = newZoom;
            
            // Calculate new visible samples with new zoom
            float newZoomMultiplier = getZoomMultiplier();
            uint16_t newVisibleSamples = (uint16_t)(GRID_WIDTH * newZoomMultiplier);
            
            // Calculate new xCursor to maintain the SAME CENTER position
            uint16_t newXCursor = centerSample - newVisibleSamples / 2;
            
            // Ensure xCursor stays within bounds
            uint16_t maxXCursor = (currentBufferSize > newVisibleSamples) ? 
                                 (currentBufferSize - newVisibleSamples) : 0;
            if(newXCursor > maxXCursor) newXCursor = maxXCursor;
            if((int16_t)newXCursor < 0) newXCursor = 0;
            
            xCursor = newXCursor;
            
            saveParameter(PARAM_ZOOM, zoomFactor);
            saveParameter(PARAM_XCURSOR, xCursor);
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
				{
				// Store center position before reset
				float zoomMultiplier = getZoomMultiplier();
				uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
				uint16_t centerSample = xCursor + visibleSamples / 2;
				
				zoomFactor = ZOOM_DEFAULT;
				
				// Adjust xCursor to keep center position
				float newZoomMultiplier = getZoomMultiplier();
				uint16_t newVisibleSamples = (uint16_t)(GRID_WIDTH * newZoomMultiplier);
				uint16_t newXCursor = centerSample - newVisibleSamples / 2;
				
				// Ensure xCursor stays within bounds
				uint16_t maxXCursor = (currentBufferSize > newVisibleSamples) ? 
									(currentBufferSize - newVisibleSamples) : 0;
				if(newXCursor > maxXCursor) newXCursor = maxXCursor;
				if(newXCursor < 0) newXCursor = 0;
				
				xCursor = newXCursor;
				
				saveParameter(PARAM_ZOOM, zoomFactor);
				saveParameter(PARAM_XCURSOR, xCursor);
				repaintLabels();
				}
			break;
		case L_window:
    // set x in the middle based on current buffer size and zoom
    {
        float zoomMultiplier = getZoomMultiplier();
        uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);

        // Ensure visible samples doesn't exceed buffer size
        if(visibleSamples > currentBufferSize) {
            visibleSamples = currentBufferSize;
        }

        // Calculate maximum xCursor position
        uint16_t maxXCursor = (currentBufferSize > visibleSamples) ? 
                            (currentBufferSize - visibleSamples) : 0;

        // Center the view - put the buffer center in the middle of the screen
        uint16_t bufferCenter = currentBufferSize / 2;
        uint16_t newXCursor = bufferCenter - visibleSamples / 2;
        
        // Constrain to valid range
        if(newXCursor > maxXCursor) newXCursor = maxXCursor;
        if((int16_t)newXCursor < 0) newXCursor = 0;
        
        changeXCursor(newXCursor);
    }
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
			if(operationMode == MODE_XY) {
                // Reset tail length in XY mode
                tailLength = DEFAULT_TAIL_LENGTH;
                repaintLabels();
            } else {
				// Reset tail length in XY mode
				triggerType=TRIGGER_AUTO;
				saveParameter(PARAM_TRIGTYPE, triggerType);
			}
			break;
		 case L_bufferSize:
				saveParameter(PARAM_PREAMBLE, PREAMBLE_VALUE, true); //salve parameters to flash
				clearWaves();
			break;
		case L_triggerEdge:
			if(operationMode == MODE_XY) {
			// Reset to dots mode in XY mode
			xylines = false;
			repaintLabels();
			}
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
            if(steps > 0) incrementBufferSize(); else decrementBufferSize();
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
	
	// manually update display if frozen - BUT DON'T BREAK SAMPLING IN HOLD MODE
	if(hold) {
		drawWaves();
	} else {
		// Only break sampling if not in hold mode and not auto trigger
		if(triggerType != TRIGGER_AUTO) {
			keepSampling = false;
		}
	}
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


// ------------------------
void incrementWaves(void) {
// ------------------------
    // Cycles through analog modes only: A1+A2, A1, A2, XY, none
	if (operationMode == MODE_OSCILLOSCOPE) {
		if (waves[0] && waves[1]) {
			// Current: A1+A2 -> Next: A1 only
			waves[1] = false;
			saveParameter(PARAM_WAVES + 1, waves[1]);
		} else if (waves[0] && !waves[1]) {
			// Current: A1 -> Next: A2 only  
			waves[0] = false;
			waves[1] = true;
			saveParameter(PARAM_WAVES + 0, waves[0]);
			saveParameter(PARAM_WAVES + 1, waves[1]);
		} else if (!waves[0] && waves[1]) {
			// Current: A2 -> Next: XY mode
			waves[0] = false;
			waves[0] = true;
			setOperationMode(MODE_XY);
		} else {
			// Current: none -> Next: A1+A2
			waves[0] = true;
			waves[1] = true;
			saveParameter(PARAM_WAVES + 0, waves[0]);
			saveParameter(PARAM_WAVES + 1, waves[1]);
		}
	} else if (operationMode == MODE_XY) {
		// Current: XY mode -> Next: spectrum
		setOperationMode(MODE_SPECTRUM);
	} else if (operationMode == MODE_SPECTRUM) {
		// Current: Spectrum mode -> Next: none
		setOperationMode(MODE_OSCILLOSCOPE);
		waves[0] = false;
		waves[1] = false;
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
    if(operationMode == MODE_XY) {
        xylines=true;
		saveParameter(PARAM_XYLINES, xylines);
        repaintLabels();
        return;
    }
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
    if(operationMode == MODE_XY) {
        xylines=false;
		saveParameter(PARAM_XYLINES, xylines);
        repaintLabels();
        return;
    }
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
if(operationMode == MODE_XY) {
    // In XY mode, timebase control adjusts tail length
	tailLength = (tailLength/5)*5;
	uint16_t newTail = tailLength + 5;
    
    // Check if tail is compatible with current buffer
    if(!isTailCompatibleWithBuffer(newTail)) {
        // Try to automatically increase buffer size
        if(bufferMode > BUF_FULL) {  // If we can increase buffer size
			uint16_t desiredTail = newTail;
            incrementBufferSize();
			setTailLength(desiredTail);
			repaintLabels();
            // Tail will be handled in the next encoder turn
            return;
        } else {
            // Already at maximum buffer, limit tail
            newTail = calculateMaxTailForBuffer();
        }
    }
    
    if(newTail != tailLength) {
        setTailLength(newTail);
    }
    repaintLabels();
    return;
}
	if(triggerType == TRIGGER_SINGLE)
		return;
	
	setTriggerType(triggerType + 1);
	saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}



// ------------------------
void decrementTT(void)	{
// ------------------------
	if(operationMode == MODE_XY) {
	tailLength = (tailLength/5)*5;
    if(tailLength >= 5) {
            setTailLength(tailLength - 5);
        } else if(tailLength > 0) {
            setTailLength(0);
        }
        repaintLabels();
        return;
    }
	if(triggerType == TRIGGER_AUTO)
		return;
	setTriggerType(triggerType - 1);
	saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}




// ------------------------
void incrementTimeBase(void)	{
// ------------------------
    if(operationMode == MODE_XY) {
        if(currentTimeBase == T50MS) return;
        setTimeBase(currentTimeBase + 1, true);
        return;
    }
    if(currentTimeBase == T50MS) return;
    setTimeBase(currentTimeBase + 1);
}



// ------------------------
void decrementTimeBase(void)	{
// ------------------------	
    if(operationMode == MODE_XY) {
        if(currentTimeBase == T20US) return;
        setTimeBase(currentTimeBase - 1, true);
        return;
    }
    if(currentTimeBase == T20US) return;
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
void changeXCursor(int16_t xPos)	{
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


// ------------------------
void setTailLength(uint16_t length) {
// ------------------------
tailLength = constrain(length, 0, currentBufferSize - 1);
    saveParameter(PARAM_TAILLENGTH, tailLength);
    repaintLabels();
}

// ------------------------
void clearXYBuffer() {
// ------------------------
    if(ch1Capture && ch2Capture) {
        // Fill with center values (zero position)
        int16_t centerValue = ADC_MAX_VAL / 2;
        for(uint16_t i = 0; i < currentBufferSize; i++) {
            ch1Capture[i] = centerValue;
            ch2Capture[i] = centerValue;
        }
    }
}

void setOperationMode(uint8_t newMode) {
    uint8_t oldMode = operationMode;
    operationMode = newMode;
    
    // Mode-specific initialization
    switch(newMode) {
        case MODE_OSCILLOSCOPE:
            // Normal oscilloscope mode
            directSamplingMode = false;
            // Ensure proper channel states
            waves[0] = true;  // Keep A1 enabled by default
            waves[1] = true;  // Keep A2 enabled by default  
            break;
            
        case MODE_XY:
            // XY mode initialization
            directSamplingMode = true;
            // Reset sampling state
            directSampleCount = 0;
            lastDirectDrawTime = 0;
            // Force both analog channels enabled
            waves[0] = true;
            waves[1] = true;
            break;
		case MODE_SPECTRUM:
            directSamplingMode = false;
            // Reset spectrum buffers when entering spectrum mode
            cleanupSpectrum();
            break;
    }
    
    // Save if mode actually changed
    if(oldMode != operationMode) {
        saveParameter(PARAM_OPERATION_MODE, operationMode);
    }
    
    repaintLabels();
}
// ------------------------
void setDirectSampling(bool enable) {
// ------------------------
    directSamplingMode = enable;
    // Could be used for other real-time modes in the future
    repaintLabels();
}
