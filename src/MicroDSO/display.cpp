#include "hal.h"
#include "variables.h"
#include "interface.h"
#include "display.h"
#include "capture.h" 
#include "MicroDSO.h"
#include "io.h"

#define DIGITAL_D1_MASK 0x2000
#define DIGITAL_D2_MASK 0x4000

//#include <Adafruit_GFX.h>
#include "../Adafruit_GFX_/Adafruit_GFX_.h"
#include "src/TFTLib/Adafruit_TFTLCD_8bit_STM32.h"

Adafruit_TFTLCD_8bit_STM32 tft;

// rendered waveform data is stored here for erasing
int16_t ch1Old[GRID_WIDTH] = {0};
int16_t ch2Old[GRID_WIDTH] = {0};
int8_t bitOld[GRID_WIDTH] = {0};

// grid variables
uint8_t hOffset = (TFT_WIDTH - GRID_WIDTH)/2;
uint8_t vOffset = (TFT_HEIGHT - GRID_HEIGHT)/2;
uint8_t dHeight = GRID_HEIGHT/8;

// plot variables -- modified by interface section
// controls which section of waveform is displayed on screen
// 0 < xCursor < (NUM_SAMPLES - GRID_WIDTH)
int16_t xCursor;
// controls the vertical positioning of waveform
int16_t yCursors[4];
// controls which waveforms are displayed
bool waves[4];
// prints waveform statistics on screen
bool printStats = true;
// repaint the labels on screen in draw loop
bool paintLabels = false;

// labels around the grid
uint8_t currentFocus = L_timebase;

void banner(void);
void clearStats(void);
void drawGrid(void);
void clearNDrawSignals(void);
void drawStats(void);
inline void plotLineSegment(int16_t transposedPt1, int16_t transposedPt2,  int index, uint16_t color);
void drawVoltage(float volt, int y, bool mvRange);

// ------------------------
void focusNextLabel(void)	{
// ------------------------
	currentFocus++;

  if(operationMode == MODE_XY) {
		// Skip controls that are hidden in XY mode
		if(currentFocus == L_zoom || 
		   currentFocus == L_window || 
		   currentFocus == L_vPos1 || 
		   currentFocus == L_vPos2 || 
		   currentFocus == L_vPos3 || 
		   currentFocus == L_vPos4 ||
		   currentFocus == L_triggerLevel) {
		   focusNextLabel();
		}
	}

	if((currentFocus == L_vPos1) && !waves[0])
		currentFocus++;

	if((currentFocus == L_vPos2) && !waves[1])
		currentFocus++;

	if((currentFocus == L_vPos3) && !waves[2])
		currentFocus++;

	if((currentFocus == L_vPos4) && !waves[3])
		currentFocus++;

	if(currentFocus > L_vPos4)
		currentFocus = L_timebase;
}




// ------------------------
void repaintLabels(void)	{
// ------------------------
	paintLabels = true;
}



// ------------------------
void initDisplay(void)	{
// ------------------------
	tft.reset();
	tft.begin(0x9341);
	tft.setRotation(LANDSCAPE);
	tft.fillScreen(ILI9341_BLACK);
	banner();

	delay(4000); 

	// and paint o-scope
	clearWaves();
}




// ------------------------
void drawWaves(void)	{
// ------------------------
	static bool printStatsOld = false;

	if(printStatsOld && !printStats)
		clearStats();
	
	printStatsOld = printStats;

	// draw the grid
	drawGrid();
	
	// clear and draw signal traces
	clearNDrawSignals();
	
	// if requested update the stats
	if(printStats)
		drawStats();

	// if label repaint requested - do so now
	if(paintLabels)	{
		drawLabels();
		paintLabels = false;
	}
}




// ------------------------
void clearWaves(void)	{
// ------------------------
	// clear screen
	tft.fillScreen(ILI9341_BLACK);
	// and paint o-scope
	drawGrid();
	drawLabels();
}



bool cDisplayed = false;

// ------------------------
void indicateCapturing(void)	{
// ------------------------
	if((currentTimeBase > T2MS) || (triggerType != TRIGGER_AUTO))	{
		cDisplayed = true;
		
		tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
		tft.setCursor(hOffset + 2, 4);
		tft.print("RUN");
	}
}



// ------------------------
void indicateCapturingDone(void)	{
// ------------------------
	if(cDisplayed)	{
		cDisplayed = false;
		tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
		tft.setCursor(hOffset + 2, 4);
		tft.print("RUN");
	}
}





// local operations below




// 0, 1 Analog channels. 2, 3 digital channels
// ------------------------
void clearNDrawSignals(void)	{
// ------------------------
	static bool wavesOld[4] = {false,};
	static int16_t yCursorsOld[4];
	static uint8_t zoomOld = 1;
	
	// snap the values to prevent interrupt from changing mid-draw
	int16_t xCursorSnap = xCursor;
	int16_t zeroVoltageA1Snap = zeroVoltageA1;
	int16_t zeroVoltageA2Snap = zeroVoltageA2;
	int16_t yCursorsSnap[4];
	bool wavesSnap[4];
	uint8_t zoomSnap = zoomFactor;  // Capture zoom factor
	
	yCursorsSnap[0] = yCursors[0];
	yCursorsSnap[1] = yCursors[1];
	yCursorsSnap[2] = yCursors[2];
	yCursorsSnap[3] = yCursors[3];
	wavesSnap[0] = waves[0];
	wavesSnap[1] = waves[1];
	wavesSnap[2] = waves[2];
	wavesSnap[3] = waves[3];
	bool xyModeSnap = (operationMode == MODE_XY);

	// Calculate visible samples based on zoom
	float zoomMultiplier = zoomFactor / 10.0f;
	uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);

	// If buffer is too small for the requested zoom, use maximum possible
	if(visibleSamples > currentBufferSize) {
		visibleSamples = currentBufferSize;
	}

	// Calculate step for zoom
	float samplesPerPixel = (float)visibleSamples / GRID_WIDTH;
	uint16_t step = (uint16_t)samplesPerPixel;
	if(step < 1) step = 1;

	// For zoom < 1x, we need to sample multiple points per pixel
	bool zoomedOut = (zoomMultiplier < 1.0f);

	// Adjust xCursor if it goes beyond valid range for current zoom
	uint16_t maxXCursor = (currentBufferSize > visibleSamples) ? 
						 (currentBufferSize - visibleSamples) : 0;
	if(xCursorSnap > maxXCursor) {
		xCursorSnap = maxXCursor;
	}

	// draw the visible section of the waveform from xCursorSnap
	int16_t val1, val2;
	int16_t transposedPt1, transposedPt2;
	uint8_t shiftedVal;

	// sampling stopped at sIndex - 1
	int j = sIndex + xCursorSnap;
	if(j >= currentBufferSize)
		j = j - currentBufferSize;
	
	// Clear only the waveform area (not the grid)
	// We clear each segment individually as we draw to avoid clearing the grid
	// go through all the visible data points with zoom step
	for(int i = 1; i < GRID_WIDTH - 1; i++) {
		int sampleIndex1, sampleIndex2;
	
		if(zoomedOut) {
			// For zoom < 1x: use multiple samples per pixel
			uint16_t startSample = j + (uint16_t)(i * samplesPerPixel);
			uint16_t endSample = j + (uint16_t)((i + 1) * samplesPerPixel);
		
			if(startSample >= currentBufferSize) startSample -= currentBufferSize;
			if(endSample >= currentBufferSize) endSample -= currentBufferSize;
		
			sampleIndex1 = startSample;
			sampleIndex2 = endSample;
		} else {
			// For zoom >= 1x: normal sampling with step
			sampleIndex1 = j + (i * step);
			sampleIndex2 = j + ((i + 1) * step);
			
			if(sampleIndex1 >= currentBufferSize) sampleIndex1 -= currentBufferSize;
			if(sampleIndex2 >= currentBufferSize) sampleIndex2 -= currentBufferSize;
		}
	
		// erase old line segments if zoom changed
		if(zoomOld != zoomSnap) {
			// Clear all old segments when zoom changes
			if(wavesOld[3])	{
				val1 = (bitOld[i] & 0b10000000) ? dHeight : 0;
				val2 = (bitOld[i + 1] & 0b10000000) ? dHeight : 0;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[3] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[3] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
	
			if(wavesOld[2])	{
				val1 = (bitOld[i] & 0b01000000) ? dHeight : 0;
				val2 = (bitOld[i + 1] & 0b01000000) ? dHeight : 0;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[2] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[2] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
				
			if(wavesOld[1])	{
				val1 = (ch2Old[i] * GRID_HEIGHT)/ADC_2_GRID;
				val2 = (ch2Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[1] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[1] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
	
			if(wavesOld[0])	{
				val1 = (ch1Old[i] * GRID_HEIGHT)/ADC_2_GRID;
				val2 = (ch1Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[0] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[0] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
	if(xyModeSnap && wavesOld[0] && wavesOld[1]) {
		// Clear entire grid area when zoom changes in XY mode
		tft.fillRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_BLACK);
		drawGrid(); // Redraw grid
	}
		} else {
			// Normal erase - only if zoom didn't change
			if(wavesOld[3])	{
				val1 = (bitOld[i] & 0b10000000) ? dHeight : 0;
				val2 = (bitOld[i + 1] & 0b10000000) ? dHeight : 0;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[3] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[3] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
	
			if(wavesOld[2])	{
				val1 = (bitOld[i] & 0b01000000) ? dHeight : 0;
				val2 = (bitOld[i + 1] & 0b01000000) ? dHeight : 0;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[2] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[2] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
				
			if(wavesOld[1])	{
				val1 = (ch2Old[i] * GRID_HEIGHT)/ADC_2_GRID;
				val2 = (ch2Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[1] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[1] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
	
			if(wavesOld[0])	{
				val1 = (ch1Old[i] * GRID_HEIGHT)/ADC_2_GRID;
				val2 = (ch1Old[i + 1] * GRID_HEIGHT)/ADC_2_GRID;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsOld[0] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsOld[0] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, ILI9341_BLACK);
			}
			if(xyModeSnap && wavesOld[0] && wavesOld[1]) {
		// For normal erase in XY mode, we need to clear the old XY trace
		// Since XY mode draws directly, we need to clear the entire waveform area
		// but only if we're switching out of XY mode or the display changed
		if(!wavesSnap[0] || !wavesSnap[1] || !xyModeSnap) {
			tft.fillRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_BLACK);
			drawGrid(); // Redraw grid
		}
	}
		}
		
		// draw new segments with current zoom
		if(wavesSnap[3])	{
			shiftedVal = bitStore[sampleIndex1] >> 7;
			val1 = (shiftedVal & 0b10000000) ? dHeight : 0;
			val2 = ((bitStore[sampleIndex2] >> 7) & 0b10000000) ? dHeight : 0;
			bitOld[i] &= 0b01000000;
			bitOld[i] |= shiftedVal & 0b10000000;
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[3] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[3] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL2);
		}
		
		if(wavesSnap[2])	{
			shiftedVal = bitStore[sampleIndex1] >> 7;
			val1 = (shiftedVal & 0b01000000) ? dHeight : 0;
			val2 = ((bitStore[sampleIndex2] >> 7) & 0b01000000) ? dHeight : 0;
			bitOld[i] &= 0b10000000;
			bitOld[i] |= shiftedVal & 0b01000000;
			transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[2] - val1;
			transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[2] - val2;
			plotLineSegment(transposedPt1, transposedPt2, i, DG_SIGNAL1);
		}
	
		if(!xyModeSnap) {
			// Normal time-based display
			if(wavesSnap[1])	{
				val1 = ((ch2Capture[sampleIndex1] - zeroVoltageA2Snap) * GRID_HEIGHT)/ADC_2_GRID;
				val2 = ((ch2Capture[sampleIndex2] - zeroVoltageA2Snap) * GRID_HEIGHT)/ADC_2_GRID;
				ch2Old[i] = ch2Capture[sampleIndex1] - zeroVoltageA2Snap;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[1] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[1] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, AN_SIGNAL2);
			}
	
			if(wavesSnap[0])	{
				val1 = ((ch1Capture[sampleIndex1] - zeroVoltageA1Snap) * GRID_HEIGHT)/ADC_2_GRID;
				val2 = ((ch1Capture[sampleIndex2] - zeroVoltageA1Snap) * GRID_HEIGHT)/ADC_2_GRID;
				ch1Old[i] = ch1Capture[sampleIndex1] - zeroVoltageA1Snap;
				transposedPt1 = GRID_HEIGHT + vOffset + yCursorsSnap[0] - val1;
				transposedPt2 = GRID_HEIGHT + vOffset + yCursorsSnap[0] - val2;
				plotLineSegment(transposedPt1, transposedPt2, i, AN_SIGNAL1);
			}
			} else {
			// XY Mode - A1 = X-axis, A2 = Y-axis
				if(wavesSnap[0] && wavesSnap[1]) {
				// Only clear if zoom changed or explicitly requested
					if(zoomOld != zoomSnap) {
					tft.fillRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_BLACK);
					drawGrid();
				}
		
				// Calculate current XY point from the latest sample
				int latestSample = sIndex - 1;
				if(latestSample < 0) latestSample = currentBufferSize - 1;
		
				int16_t currentX = ((ch1Capture[latestSample] - zeroVoltageA1Snap) * GRID_WIDTH) / (ADC_2_GRID * 2);
				int16_t currentY = ((ch2Capture[latestSample] - zeroVoltageA2Snap) * GRID_HEIGHT) / ADC_2_GRID;
		
				// Center and clamp to grid bounds
				int16_t screenX = constrain(GRID_WIDTH/2 + currentX + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
				int16_t screenY = constrain(GRID_HEIGHT/2 + vOffset - currentY, vOffset, vOffset + GRID_HEIGHT - 1);
		
				// Draw tail - previous points with fading intensity
				if(tailEnabled && tailLength > 0) {
					for(uint16_t tailIdx = 1; tailIdx <= tailLength; tailIdx++) {
						int sampleIdx = latestSample - tailIdx;
						if(sampleIdx < 0) sampleIdx += currentBufferSize;
				
						// Skip if we've wrapped around to new data
						if(sampleIdx > latestSample && latestSample < tailLength) continue;
				
						// Calculate tail point
						int16_t tailX = ((ch1Capture[sampleIdx] - zeroVoltageA1Snap) * GRID_WIDTH) / (ADC_2_GRID * 2);
						int16_t tailY = ((ch2Capture[sampleIdx] - zeroVoltageA2Snap) * GRID_HEIGHT) / ADC_2_GRID;
				
						int16_t tailScreenX = constrain(GRID_WIDTH/2 + tailX + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
						int16_t tailScreenY = constrain(GRID_HEIGHT/2 + vOffset - tailY, vOffset, vOffset + GRID_HEIGHT - 1);
				
						// Calculate fade color (darker for older points)
						uint8_t fade = 255 - (tailIdx * 255 / tailLength);
						uint16_t tailColor = tft.color565(fade, fade / 2, fade); // Cyan fading to black
				
						// Draw tail point
						tft.drawPixel(tailScreenX, tailScreenY, tailColor);
				
						// Draw line to next point in tail for smoother appearance
						if(tailIdx < tailLength) {
							int nextSampleIdx = sampleIdx + 1;
							if(nextSampleIdx >= currentBufferSize) nextSampleIdx = 0;
					
							int16_t nextX = ((ch1Capture[nextSampleIdx] - zeroVoltageA1Snap) * GRID_WIDTH) / (ADC_2_GRID * 2);
							int16_t nextY = ((ch2Capture[nextSampleIdx] - zeroVoltageA2Snap) * GRID_HEIGHT) / ADC_2_GRID;
					
					int16_t nextScreenX = constrain(GRID_WIDTH/2 + nextX + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
					int16_t nextScreenY = constrain(GRID_HEIGHT/2 + vOffset - nextY, vOffset, vOffset + GRID_HEIGHT - 1);
					
					tft.drawLine(tailScreenX, tailScreenY, nextScreenX, nextScreenY, tailColor);
				}
			}
		}
		
		// Draw current point in bright color
		tft.drawPixel(screenX, screenY, AN_SIGNALX);
		tft.drawCircle(screenX, screenY, 2, AN_SIGNALX); // Make current point more visible
		
		// Store old values
		ch1Old[i] = ch1Capture[sampleIndex1] - zeroVoltageA1Snap;
		ch2Old[i] = ch2Capture[sampleIndex1] - zeroVoltageA2Snap;
	}
}
	}
	
	// store the drawn parameters to old storage
	zoomOld = zoomSnap;
	wavesOld[0] = wavesSnap[0];
	wavesOld[1] = wavesSnap[1];
	wavesOld[2] = wavesSnap[2];
	wavesOld[3] = wavesSnap[3];
	yCursorsOld[0] = yCursorsSnap[0];
	yCursorsOld[1] = yCursorsSnap[1];
	yCursorsOld[2] = yCursorsSnap[2];
	yCursorsOld[3] = yCursorsSnap[3];
}



// ------------------------
inline void plotLineSegment(int16_t transposedPt1, int16_t transposedPt2,  int index, uint16_t color)	{
// ------------------------
	// range checks
	if(transposedPt1 > (GRID_HEIGHT + vOffset))
		transposedPt1 = GRID_HEIGHT + vOffset;
	if(transposedPt1 < vOffset)
		transposedPt1 = vOffset;
	if(transposedPt2 > (GRID_HEIGHT + vOffset))
		transposedPt2 = GRID_HEIGHT + vOffset;
	if(transposedPt2 < vOffset)
		transposedPt2 = vOffset;

	// draw the line segments
	tft.drawLine(index + hOffset, transposedPt1, index + hOffset, transposedPt2, color);
}





// ------------------------
void drawVCursor(int channel, uint16_t color, bool highlight)	{
// ------------------------
	int cPos = GRID_HEIGHT + vOffset + yCursors[channel];
    tft.fillTriangle(0, cPos - 5, hOffset, cPos, 0, cPos + 5, color);
	if(highlight)
		tft.drawRect(0, cPos - 7, hOffset, 14, ILI9341_WHITE);
}




// ------------------------
void drawGrid(void)	{
// ------------------------
	uint8_t hPacing = GRID_WIDTH / 12;
	uint8_t vPacing = GRID_HEIGHT / 8;

	for(int i = 1; i < 12; i++)
		tft.drawFastVLine(i * hPacing + hOffset, vOffset, GRID_HEIGHT, GRID_COLOR);

	for(int i = 1; i < 8; i++)
		tft.drawFastHLine(hOffset, i * vPacing + vOffset, GRID_WIDTH, GRID_COLOR);

	for(int i = 1; i < 5*8; i++)
		tft.drawFastHLine(hOffset + GRID_WIDTH/2 - 3, i * vPacing/5 + vOffset, 7, GRID_COLOR);

	for(int i = 1; i < 5*12; i++)
		tft.drawFastVLine(i * hPacing/5 + hOffset, vOffset + GRID_HEIGHT/2 - 4, 7, GRID_COLOR);

	tft.drawRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_WHITE);
}




// ------------------------
void drawLabels(void)	{
// ------------------------
	// draw the static labels around the grid

	// erase top bar
	tft.fillRect(hOffset, 0, TFT_WIDTH, vOffset, ILI9341_BLACK);
	tft.fillRect(hOffset + GRID_WIDTH, 0, hOffset, TFT_HEIGHT, ILI9341_BLACK);

	// paint run/hold information
	// -----------------
	tft.setCursor(hOffset + 2, 4);

	if(hold)	{
		tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
		tft.print(" HOLD ");
	}
	else	{
		// Only show RUN if not currently sampling (sampling will override this)
		if(!cDisplayed) {
			tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
			tft.print("RUN");
		}
	}

	// draw zoom factor
	// -----------------
	tft.setCursor(55, 4);
	if(currentFocus == L_zoom)
	 	tft.drawRect(45, 0, 35, vOffset, ILI9341_WHITE);
	
	tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
	if(!operationMode == MODE_XY) {
		//zoom
		tft.print("x");
	
		// Display zoom factor properly
		if(zoomFactor < 10) {
			// Display as decimal: 0.1x to 0.9x
			tft.print("0.");
			tft.print(zoomFactor);
		} else {
			// Display as integer: 1x to 10x
			tft.print(zoomFactor / 10);
		}
	}

	// draw x-window at top, range = 200px
	// -----------------
	if(!operationMode == MODE_XY) {
		int sampleSizePx = 160;
		float lOffset = (TFT_WIDTH - sampleSizePx)/2;
		tft.drawFastVLine(lOffset, 3, vOffset - 6, ILI9341_GREEN);
		tft.drawFastVLine(lOffset + sampleSizePx, 3, vOffset - 6, ILI9341_GREEN);
		tft.drawFastHLine(lOffset, vOffset/2, sampleSizePx, ILI9341_GREEN);

		// Calculate window size and position based on center-based zoom
		float zoomMultiplier = zoomFactor / 10.0f;
		uint16_t visibleSamples = (uint16_t)(GRID_WIDTH * zoomMultiplier);
		if(visibleSamples > currentBufferSize) {
			visibleSamples = currentBufferSize;
		}

		// Calculate window size and position (center-based)
		float windowRatio = (float)visibleSamples / currentBufferSize;
		float windowSize = sampleSizePx * windowRatio;
		// Calculate center position in the buffer
		float centerRatio = (float)(xCursor + visibleSamples / 2) / currentBufferSize;
		float centerPx = centerRatio * sampleSizePx + lOffset;
		// Calculate xCursor position based on center
		float xCursorPx = centerPx - windowSize / 2;

		if(currentFocus == L_window)
			tft.drawRect(xCursorPx, 4, windowSize, vOffset - 8, ILI9341_WHITE);
		else
			tft.fillRect(xCursorPx, 4, windowSize, vOffset - 8, ILI9341_GREEN);

	} else {
		// In XY mode, clear the area where scroll bar would be
		tft.fillRect(0, 0, hOffset, vOffset, ILI9341_BLACK);
	}

	// print active wave indicators
	// -----------------
	tft.setCursor(250, 4);
	if(operationMode == MODE_XY) {
			// XY mode - show with dark grey background ONLY on the indicator
			tft.setTextColor(AN_SIGNALX, ILI9341_BLACK);
			tft.print("X-Y mode");
	} else {
	
		if(waves[0]) {
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			tft.print("A1 ");
		} else {
			tft.print("   ");
		}

		if(waves[1]) {
			tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
			tft.print("A2 ");
		} else {
			tft.print("   ");
		}
		if(waves[2]) {
			tft.setTextColor(DG_SIGNAL1, ILI9341_BLACK);
			tft.print("D1 ");
		} else {
			tft.print("   ");
		}

		if(waves[3]) {
			tft.setTextColor(DG_SIGNAL2, ILI9341_BLACK);
			tft.print("D2");
		} else {
			tft.print("  ");
		}
	}


	if(currentFocus == L_waves)
		tft.drawRect(247, 0, 72, vOffset, ILI9341_WHITE);
	// erase left side of grid
	tft.fillRect(0, 0, hOffset, TFT_HEIGHT, ILI9341_BLACK);
	
	// draw new wave cursors
	// -----------------
	if(!operationMode == MODE_XY) {
		if(waves[3])
			drawVCursor(3, DG_SIGNAL2, (currentFocus == L_vPos4));
		if(waves[2])
			drawVCursor(2, DG_SIGNAL1, (currentFocus == L_vPos3));
		if(waves[1])
			drawVCursor(1, AN_SIGNAL2, (currentFocus == L_vPos2));
		if(waves[0])
			drawVCursor(0, AN_SIGNAL1, (currentFocus == L_vPos1));

	}
	// erase bottom bar
	tft.fillRect(hOffset, GRID_HEIGHT + vOffset, TFT_WIDTH, vOffset, ILI9341_BLACK);

	// print input switch pos
	// -----------------
	tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
	tft.setCursor(hOffset + 10, GRID_HEIGHT + vOffset + 4);
	tft.print(rngNames[rangePos]);
	tft.setCursor(hOffset + 50, GRID_HEIGHT + vOffset + 4);
	tft.print(cplNames[couplingPos]);
	
	// print new timebase
	// -----------------
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setCursor(145, GRID_HEIGHT + vOffset + 4);
	if(currentFocus == L_timebase)
		tft.drawRect(140, GRID_HEIGHT + vOffset, 45, vOffset, ILI9341_WHITE);
	tft.print(getTimebaseLabel());

	// print trigger type
	// -----------------
	tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
	tft.setCursor(230, GRID_HEIGHT + vOffset + 4);
	if(currentFocus == L_triggerType)
		tft.drawRect(225, GRID_HEIGHT + vOffset, 35, vOffset, ILI9341_WHITE);
	
	if(operationMode == MODE_XY) {
		// Show tail length in XY mode
		tft.print(tailLength);
	} else {
		//draw trigger type in normal mode
		switch(triggerType) {
			case TRIGGER_AUTO: tft.print("AUTO"); break;
			case TRIGGER_NORM: tft.print("NORM"); break;
			case TRIGGER_SINGLE: tft.print("SING"); break;
		}
	}

	// draw trigger edge
	// -----------------
	if(currentFocus == L_triggerEdge)
		tft.drawRect(266, GRID_HEIGHT + vOffset, 15, vOffset + 4, ILI9341_WHITE);

	int trigX = 270;

	if(operationMode == MODE_XY) {
		// Show connection mode in XY mode
			if(xylines) {
				// Connected lines mode - show line
				tft.drawFastHLine(trigX, TFT_HEIGHT - 10, 8, ILI9341_GREEN);
				tft.drawFastHLine(trigX, TFT_HEIGHT - 8, 8, ILI9341_GREEN);
			} else {
				// Dots only mode - show dots
				tft.drawPixel(trigX + 2, TFT_HEIGHT - 10, ILI9341_GREEN);
				tft.drawPixel(trigX + 4, TFT_HEIGHT - 8, ILI9341_GREEN);
				tft.drawPixel(trigX + 6, TFT_HEIGHT - 10, ILI9341_GREEN);
			}
	} else {
		// Show trigger edge in normal mode
		if(triggerRising) {
			tft.drawFastHLine(trigX, TFT_HEIGHT - 3, 5, ILI9341_GREEN);
			tft.drawFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
			tft.drawFastHLine(trigX + 4, TFT_HEIGHT -vOffset + 2, 5, ILI9341_GREEN);
			tft.fillTriangle(trigX + 2, 232, trigX + 4, 230, trigX + 6, 232, ILI9341_GREEN);
		} else {
			tft.drawFastHLine(trigX + 4, TFT_HEIGHT - 3, 5, ILI9341_GREEN);
			tft.drawFastVLine(trigX + 4, TFT_HEIGHT -vOffset + 2, vOffset - 4, ILI9341_GREEN);
			tft.drawFastHLine(trigX - 1, TFT_HEIGHT -vOffset + 2, 5, ILI9341_GREEN);
			tft.fillTriangle(trigX + 2, 231, trigX + 4, 233, trigX + 6, 231, ILI9341_GREEN);
		}
	}
	
	//draw buffer size
	tft.setCursor(285, GRID_HEIGHT + vOffset + 4);
	if(currentFocus == L_bufferSize)
	tft.drawRect(280, GRID_HEIGHT + vOffset, 30, vOffset, ILI9341_WHITE);

	switch(bufferMode) {
		case BUF_FULL:
			tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
			break;
		case BUF_HALF:
			tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
		break;
		case BUF_QUARTER:
			tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
			break;
		case BUF_EIGHTH:
			tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
			break;
	}
	tft.print(bufferModeNames[bufferMode]);
	
	// draw trigger level on right side
	// -----------------
	if(!operationMode == MODE_XY) {
		int cPos = GRID_HEIGHT + vOffset + yCursors[0] - getTriggerLevel()/3;
		tft.fillTriangle(TFT_WIDTH, cPos - 5, TFT_WIDTH - hOffset, cPos, TFT_WIDTH, cPos + 5, AN_SIGNAL1);
		if(currentFocus == L_triggerLevel)
			tft.drawRect(GRID_WIDTH + hOffset, cPos - 7, hOffset, 14, ILI9341_WHITE);
	}
}


// #define DRAW_TIMEBASE

// ------------------------
void drawStats(void)	{
// ------------------------
	static long lastCalcTime = 0;
	static bool lastWaves[4] = {false, false, false, false};
	bool clearStats = false;
	bool channelsChanged = false;
	
	// Check if channels changed
	for(int i = 0; i < 4; i++) {
		if(waves[i] != lastWaves[i]) {
			channelsChanged = true;
			lastWaves[i] = waves[i];
		}
	}
	
	// calculate stats once a while
	if(millis() - lastCalcTime > 300)	{
		lastCalcTime = millis();	
		calculateStats();
		clearStats = true;
	}

	// Check if any channels are active
	bool hasActiveChannels = waves[0] || waves[1] || waves[2] || waves[3];
	bool hasAnalog = waves[0] || waves[1];

	// Clear only on channel change or when stats are updated
	if(channelsChanged || clearStats) {
		// Clear frequency values area (left side) - make room for up to 4 channels with proper spacing
		tft.fillRect(50, 20, 160, 40, ILI9341_BLACK);
		
		// Clear voltage values area (right side) - only for analog channels
		if(hasAnalog) {
			// Clear the full dual channel area but allow to go closer to grid edge
			int maxX = hOffset + GRID_WIDTH - 1; // Only 1px margin from the right white border
			int voltageWidth = 74; // Increased from 70 to 74 to use the extra space
			int voltageX = 240; // Values position (unchanged)
			
			// Adjust if voltage display would go outside grid
			if(voltageX + voltageWidth > maxX) {
				voltageWidth = maxX - voltageX;
			}
			if(voltageWidth > 0) {
				tft.fillRect(voltageX, 20, voltageWidth, 50, ILI9341_BLACK);
			}
		} else {
			// No analog channels - only clear the area where labels would be, don't touch white strip
			tft.fillRect(211, 20, 54, 50, ILI9341_BLACK); // Adjusted for new label position
		}
	}

	// Draw stat labels (white)
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

	// Check if we should show info screen (no active channels)
	if(!hasActiveChannels) {
		// Show info screen instead of frequency labels
		tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
		tft.setCursor(15, 20);
		tft.print("       ");
		tft.print("DSO138");
		
		tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
		tft.setCursor(15, 30);
		tft.print("       ");
		tft.print("Firmware: ");
		tft.print(FIRMWARE_VERSION);
		
		tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
		tft.setCursor(15, 40);
		tft.print("       ");
		tft.print(PROJECT_URL);
	} else {
		// Normal mode - show frequency labels
		tft.setCursor(15, 20);
		tft.print("Freq:");
		tft.setCursor(15, 30);
		tft.print("Cycle:");
		tft.setCursor(15, 40);
		tft.print("PW:");
		tft.setCursor(15, 50);
		tft.print("Duty:");
	}

	// Right side labels (voltage stats - only for analog channels) - MOVED 1px LEFT
	if(hasAnalog) {
		tft.setCursor(211, 20); // Moved 1px left (212->211)
		tft.print("Vmax:");
		tft.setCursor(211, 30); // Moved 1px left
		tft.print("Vmin:");
		tft.setCursor(211, 40); // Moved 1px left
		tft.print("Vavr:");
		tft.setCursor(211, 50); // Moved 1px left
		tft.print("Vpp:");
		tft.setCursor(211, 60); // Moved 1px left
		tft.print("Vrms:");
	} else {
		// No analog channels - don't show voltage labels, already cleared above
	}
	
	// Display frequency statistics for all active channels (analog and digital)
	if(hasActiveChannels) {
		displayAllChannelFrequency(clearStats || channelsChanged);
	} else {
		// Clear frequency values area when no channels active
		tft.fillRect(50, 20, 160, 40, ILI9341_BLACK);
	}
	
	// Display voltage statistics only for analog channels
	if(hasAnalog) {
		displayDualChannelVoltages(clearStats || channelsChanged);
	} else {
		// No analog channels - don't show voltage values
		// Clear only the values area (not beyond white strip)
		int maxX = hOffset + GRID_WIDTH - 1;
		int voltageWidth = 74;
		int voltageX = 240; // Values stay at same position
		
		// Only clear if it wouldn't affect the white strip
		if(voltageX + voltageWidth <= maxX) {
			tft.fillRect(voltageX, 20, voltageWidth, 50, ILI9341_BLACK);
		} else if(voltageX < maxX) {
			// Only clear up to the white strip
			tft.fillRect(voltageX, 20, maxX - voltageX, 50, ILI9341_BLACK);
		}
	}
}


// ------------------------
void displayAllChannelFrequency(bool clearStats) {
// ------------------------
	// Calculate statistics for all active channels
	Stats wStatsA1, wStatsA2, wStatsD1, wStatsD2;
	
	if(waves[0]) calculateStatsForChannel(0, wStatsA1);
	if(waves[1]) calculateStatsForChannel(1, wStatsA2);
	if(waves[2]) calculateStatsForDigitalChannel(2, wStatsD1);
	if(waves[3]) calculateStatsForDigitalChannel(3, wStatsD2);
	
	int xPos = 50; // Starting position for frequency values - MOVED 10px RIGHT (40->50)
	int yPos = 20;
	int channelSpacing = 40; // Perfect spacing
	
	// Clear the entire frequency values area first to remove any old data
	if(clearStats) {
		tft.fillRect(xPos, yPos, 160, 40, ILI9341_BLACK);
	}
	
	// Freq - display for all active channels
	int currentX = xPos;
	yPos = 20;
	
	if(waves[0]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		if(wStatsA1.pulseValid) {
			tft.print((int)wStatsA1.freq);
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[1]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		if(wStatsA2.pulseValid) {
			tft.print((int)wStatsA2.freq);
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[2]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL1, ILI9341_BLACK);
		if(wStatsD1.pulseValid) {
			tft.print((int)wStatsD1.freq);
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[3]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL2, ILI9341_BLACK);
		if(wStatsD2.pulseValid) {
			tft.print((int)wStatsD2.freq);
		} else {
			tft.print("--");
		}
	}
	
	// Cycle
	currentX = xPos;
	yPos = 30;
	
	if(waves[0]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		if(wStatsA1.pulseValid) {
			tft.print(wStatsA1.cycle, 1); tft.print("m");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[1]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		if(wStatsA2.pulseValid) {
			tft.print(wStatsA2.cycle, 1); tft.print("m");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[2]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL1, ILI9341_BLACK);
		if(wStatsD1.pulseValid) {
			tft.print(wStatsD1.cycle, 1); tft.print("m");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[3]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL2, ILI9341_BLACK);
		if(wStatsD2.pulseValid) {
			tft.print(wStatsD2.cycle, 1); tft.print("m");
		} else {
			tft.print("--");
		}
	}
	
	// PW
	currentX = xPos;
	yPos = 40;
	
	if(waves[0]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		if(wStatsA1.pulseValid) {
			tft.print(wStatsA1.avgPW/1000, 1); tft.print("m");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[1]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		if(wStatsA2.pulseValid) {
			tft.print(wStatsA2.avgPW/1000, 1); tft.print("m");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[2]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL1, ILI9341_BLACK);
		if(wStatsD1.pulseValid) {
			tft.print(wStatsD1.avgPW/1000, 1); tft.print("m");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[3]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL2, ILI9341_BLACK);
		if(wStatsD2.pulseValid) {
			tft.print(wStatsD2.avgPW/1000, 1); tft.print("m");
		} else {
			tft.print("--");
		}
	}
	
	// Duty
	currentX = xPos;
	yPos = 50;
	
	if(waves[0]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		if(wStatsA1.pulseValid) {
			tft.print(wStatsA1.duty, 0); tft.print("%");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[1]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		if(wStatsA2.pulseValid) {
			tft.print(wStatsA2.duty, 0); tft.print("%");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[2]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL1, ILI9341_BLACK);
		if(wStatsD1.pulseValid) {
			tft.print(wStatsD1.duty, 0); tft.print("%");
		} else {
			tft.print("--");
		}
		currentX += channelSpacing;
	}
	
	if(waves[3]) {
		tft.setCursor(currentX, yPos);
		tft.setTextColor(DG_SIGNAL2, ILI9341_BLACK);
		if(wStatsD2.pulseValid) {
			tft.print(wStatsD2.duty, 0); tft.print("%");
		} else {
			tft.print("--");
		}
	}
}

// ------------------------
void displayDualChannelVoltages(bool clearStats) {
// ------------------------
	// Calculate statistics for channel 2 if enabled
	Stats wStats2;
	if(waves[1]) {
		calculateStatsForChannel(1, wStats2);
	}
	
	int xPos = 240; // Position for values (unchanged)
	int yPos = 20;
	int yStep = 10;
	
	// Calculate maximum X position to stay within waveform area - allow closer to edge
	int maxX = hOffset + GRID_WIDTH - 1; // Only 1px margin from right white border
	
	// Clear the voltage values area only if needed
	if(clearStats) {
		// Calculate proper black box size based on active channels - use wider boxes
		int voltageWidth = (waves[0] && waves[1]) ? 74 : 39; // Increased from 70/35 to 74/39
		
		// Adjust width if it would go outside grid bounds
		if(xPos + voltageWidth > maxX) {
			voltageWidth = maxX - xPos;
		}
		
		if(voltageWidth > 0) {
			tft.fillRect(xPos, yPos, voltageWidth, 50, ILI9341_BLACK);
		}
	}
	
	// Vmax
	if(waves[0] && waves[1]) {
		// Both channels - A1 on left, A2 on right
		// Check if second value would go outside bounds
		if(xPos + 35 <= maxX) {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vmaxf, wStats.mvPos);
			tft.setCursor(xPos + 35, yPos);
			tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
			drawCompactVoltage(wStats2.Vmaxf, wStats.mvPos);
		} else {
			// Not enough space for both, show only first channel
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vmaxf, wStats.mvPos);
		}
	} else if(waves[0]) {
		// Only A1 - use left position
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		drawCompactVoltage(wStats.Vmaxf, wStats.mvPos);
	} else if(waves[1]) {
		// Only A2 - use left position
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		drawCompactVoltage(wStats2.Vmaxf, wStats.mvPos);
	}
	
	// Vmin
	yPos += yStep;
	if(waves[0] && waves[1]) {
		if(xPos + 35 <= maxX) {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vminf, wStats.mvPos);
			tft.setCursor(xPos + 35, yPos);
			tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
			drawCompactVoltage(wStats2.Vminf, wStats.mvPos);
		} else {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vminf, wStats.mvPos);
		}
	} else if(waves[0]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		drawCompactVoltage(wStats.Vminf, wStats.mvPos);
	} else if(waves[1]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		drawCompactVoltage(wStats2.Vminf, wStats.mvPos);
	}
	
	// Vavr
	yPos += yStep;
	if(waves[0] && waves[1]) {
		if(xPos + 35 <= maxX) {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vavrf, wStats.mvPos);
			tft.setCursor(xPos + 35, yPos);
			tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
			drawCompactVoltage(wStats2.Vavrf, wStats.mvPos);
		} else {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vavrf, wStats.mvPos);
		}
	} else if(waves[0]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		drawCompactVoltage(wStats.Vavrf, wStats.mvPos);
	} else if(waves[1]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		drawCompactVoltage(wStats2.Vavrf, wStats.mvPos);
	}
	
	// Vpp
	yPos += yStep;
	if(waves[0] && waves[1]) {
		if(xPos + 35 <= maxX) {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vmaxf - wStats.Vminf, wStats.mvPos);
			tft.setCursor(xPos + 35, yPos);
			tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
			drawCompactVoltage(wStats2.Vmaxf - wStats2.Vminf, wStats.mvPos);
		} else {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vmaxf - wStats.Vminf, wStats.mvPos);
		}
	} else if(waves[0]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		drawCompactVoltage(wStats.Vmaxf - wStats.Vminf, wStats.mvPos);
	} else if(waves[1]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		drawCompactVoltage(wStats2.Vmaxf - wStats2.Vminf, wStats.mvPos);
	}
	
	// Vrms
	yPos += yStep;
	if(waves[0] && waves[1]) {
		if(xPos + 35 <= maxX) {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vrmsf, wStats.mvPos);
			tft.setCursor(xPos + 35, yPos);
			tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
			drawCompactVoltage(wStats2.Vrmsf, wStats.mvPos);
		} else {
			tft.setCursor(xPos, yPos);
			tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
			drawCompactVoltage(wStats.Vrmsf, wStats.mvPos);
		}
	} else if(waves[0]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL1, ILI9341_BLACK);
		drawCompactVoltage(wStats.Vrmsf, wStats.mvPos);
	} else if(waves[1]) {
		tft.setCursor(xPos, yPos);
		tft.setTextColor(AN_SIGNAL2, ILI9341_BLACK);
		drawCompactVoltage(wStats2.Vrmsf, wStats.mvPos);
	}
}

// ------------------------
void drawCompactVoltage(float volt, bool mvRange) {
// ------------------------
	if(mvRange) {
		int iVolt = volt;
		tft.print(iVolt);
		tft.print("m");
	} else {
		// Show 1 decimal place for volts, but compact format
		if(volt < 10) {
			tft.print(volt, 1);
		} else {
			tft.print((int)volt);
		}
	}
}

// ------------------------
void calculateStatsForChannel(int channel, Stats &stats) {
// ------------------------
	// extract waveform stats for specified channel
	int16_t Vmax = -ADC_MAX_VAL, Vmin = ADC_MAX_VAL;
	int32_t sumSamples = 0;
	int64_t sumSquares = 0;
	int32_t freqSumSamples = 0;
	
	uint16_t* channelData = (channel == 0) ? ch1Capture : ch2Capture;
	int16_t zeroVoltage = (channel == 0) ? zeroVoltageA1 : zeroVoltageA2;

	for(uint16_t k = 0; k < currentBufferSize; k++) {
		int16_t val = channelData[k] - zeroVoltage;
		if(Vmax < val)
			Vmax = val;
		if(Vmin > val)
			Vmin = val;

		sumSamples += val;
		freqSumSamples += channelData[k];
		sumSquares += (val * val);
	}

	// Calculate frequency stats for this channel
	uint16_t fVavr = freqSumSamples/currentBufferSize;
	bool dnWave = (channelData[sIndex] < fVavr - 10);
	bool firstOne = true;
	uint16_t cHigh = 0;

	uint16_t sumCW = 0;
	uint16_t sumPW = 0;
	uint16_t numCycles = 0;
	uint16_t numHCycles = 0;

	for(uint16_t sCtr = 0, k = sIndex; sCtr < currentBufferSize; sCtr++, k++) {
		if(k == currentBufferSize)
			k = 0;

		if(dnWave && (channelData[k] > fVavr + 10)) {
			if(!firstOne) {
				sumCW += (sCtr - cHigh);
				numCycles++;
			} else {
				firstOne = false;
			}
			dnWave = false;
			cHigh = sCtr;
		}

		if(!dnWave && (channelData[k] < fVavr - 10)) {
			if(!firstOne) {
				sumPW += (sCtr - cHigh);
				numHCycles++;
			}
			dnWave = true;
		}
	}

	double tPerSample = ((double)samplingTime) / currentBufferSize;
	double avgCycleWidth = (numCycles > 0) ? (sumCW * tPerSample / numCycles) : 0;
	
	stats.avgPW = (numHCycles > 0) ? (sumPW * tPerSample / numHCycles) : 0;
	stats.duty = (avgCycleWidth > 0) ? (stats.avgPW * 100 / avgCycleWidth) : 0;
	stats.freq = (avgCycleWidth > 0) ? (1000000/avgCycleWidth) : 0;
	stats.cycle = avgCycleWidth/1000;
	stats.pulseValid = (avgCycleWidth != 0) && (stats.avgPW != 0) && ((Vmax - Vmin) > 20);
	
	stats.mvPos = (rangePos == RNG_50mV) || (rangePos == RNG_20mV) || (rangePos == RNG_10mV);
	stats.Vrmsf = sqrt(sumSquares/currentBufferSize) * adcMultiplier[rangePos];
	stats.Vavrf = sumSamples/currentBufferSize * adcMultiplier[rangePos];
	stats.Vmaxf = Vmax * adcMultiplier[rangePos];
	stats.Vminf = Vmin * adcMultiplier[rangePos];
}

// ------------------------
void calculateStatsForDigitalChannel(int channel, Stats &stats) {
// ------------------------
	// Extract digital waveform stats
	// Digital channels: 2 = D1, 3 = D2
	// Digital data is stored in bitStore: D1 = bit 13 (0x2000), D2 = bit 14 (0x4000)
	
	uint16_t mask = (channel == 2) ? 0x2000 : 0x4000;
	
	bool firstTransition = true;
	bool currentState = (bitStore[sIndex] & mask) != 0;
	bool lastState = currentState;
	uint16_t cHigh = 0;
	
	uint16_t sumCW = 0;
	uint16_t sumPW = 0;
	uint16_t numCycles = 0;
	uint16_t numHCycles = 0;

	for(uint16_t sCtr = 0, k = sIndex; sCtr < currentBufferSize; sCtr++, k++) {
		if(k == currentBufferSize)
			k = 0;

		currentState = (bitStore[k] & mask) != 0;
		
		// Detect rising edge
		if(!lastState && currentState) {
			if(!firstTransition) {
				sumCW += (sCtr - cHigh);
				numCycles++;
			} else {
				firstTransition = false;
			}
			cHigh = sCtr;
		}
		
		// Detect falling edge
		if(lastState && !currentState) {
			if(!firstTransition) {
				sumPW += (sCtr - cHigh);
				numHCycles++;
			}
		}
		
		lastState = currentState;
	}

	double tPerSample = ((double)samplingTime) / currentBufferSize;
	double avgCycleWidth = (numCycles > 0) ? (sumCW * tPerSample / numCycles) : 0;
	
	stats.avgPW = (numHCycles > 0) ? (sumPW * tPerSample / numHCycles) : 0;
	stats.duty = (avgCycleWidth > 0) ? (stats.avgPW * 100 / avgCycleWidth) : 0;
	stats.freq = (avgCycleWidth > 0) ? (1000000/avgCycleWidth) : 0;
	stats.cycle = avgCycleWidth/1000;
	stats.pulseValid = (avgCycleWidth != 0) && (stats.avgPW != 0);
	
	// Voltage stats are not meaningful for digital channels
	stats.mvPos = false;
	stats.Vrmsf = 0;
	stats.Vavrf = 0;
	stats.Vmaxf = 0;
	stats.Vminf = 0;
}

// ------------------------
void calculateStats(void)	{
// ------------------------
	// extract waveform stats
	int16_t Vmax = -ADC_MAX_VAL, Vmin = ADC_MAX_VAL;
	int32_t sumSamples = 0;
	int64_t sumSquares = 0;
	int32_t freqSumSamples = 0;

	for(uint16_t k = 0; k < NUM_SAMPLES; k++)	{
		int16_t val = ch1Capture[k] - zeroVoltageA1;
		if(Vmax < val)
			Vmax = val;
		if(Vmin > val)
			Vmin = val;

		sumSamples += val;
		freqSumSamples += ch1Capture[k];
		sumSquares += (val * val);
	}

	// find out frequency
	uint16_t fVavr = freqSumSamples/NUM_SAMPLES;
	bool dnWave = (ch1Capture[sIndex] < fVavr - 10);
	bool firstOne = true;
	uint16_t cHigh = 0;

	uint16_t sumCW = 0;
	uint16_t sumPW = 0;
	uint16_t numCycles = 0;
	uint16_t numHCycles = 0;

	// sampling stopped at sIndex - 1
	for(uint16_t sCtr = 0, k = sIndex; sCtr < NUM_SAMPLES; sCtr++, k++)	{
		if(k == NUM_SAMPLES)
			k = 0;

		// mark the points where wave transitions the average value
		if(dnWave && (ch1Capture[k] > fVavr + 10))	{
			if(!firstOne)	{
				sumCW += (sCtr - cHigh);
				numCycles++;
			}
			else
				firstOne = false;

			dnWave = false;
			cHigh = sCtr;
		}

		if(!dnWave && (ch1Capture[k] < fVavr - 10))	{
			if(!firstOne)	{
				sumPW += (sCtr - cHigh);
				numHCycles++;
			}

			dnWave = true;
		}
	}

	double tPerSample = ((double)samplingTime) / NUM_SAMPLES;
	//float timePerDiv = tPerSample * 25;
	double avgCycleWidth = sumCW * tPerSample / numCycles;
	
	wStats.avgPW = sumPW * tPerSample / numHCycles;
	wStats.duty = wStats.avgPW * 100 / avgCycleWidth;
	wStats.freq = 1000000/avgCycleWidth;
	wStats.cycle = avgCycleWidth/1000;
	wStats.pulseValid = (avgCycleWidth != 0) && (wStats.avgPW != 0) && ((Vmax - Vmin) > 20);
	
	wStats.mvPos = (rangePos == RNG_50mV) || (rangePos == RNG_20mV) || (rangePos == RNG_10mV);
	wStats.Vrmsf = sqrt(sumSquares/NUM_SAMPLES) * adcMultiplier[rangePos];
	wStats.Vavrf = sumSamples/NUM_SAMPLES * adcMultiplier[rangePos];
	wStats.Vmaxf = Vmax * adcMultiplier[rangePos];
	wStats.Vminf = Vmin * adcMultiplier[rangePos];
}




// ------------------------
void drawVoltage(float volt, int y, bool mvRange)	{
// ------------------------
	// text is standard 5 px wide
	int numDigits = 1;
	int lVolt = volt;
	
	// is there a negative sign at front
	if(volt < 0)	{
		numDigits++;
		lVolt = -lVolt;
	}
	
	// how many digits before 0
	if(lVolt > 999)
		numDigits++;
	if(lVolt > 99)
		numDigits++;
	if(lVolt > 9)
		numDigits++;
	
	// mv range has mV appended at back
	if(mvRange)	{
		numDigits += 1;
		int x = GRID_WIDTH + hOffset - 10 - numDigits * 5;
		tft.setCursor(x, y);
		int iVolt = volt;
		tft.print(iVolt);
		tft.print("m");
	}
	else	{
		// non mV range has two decimal pos and V appended at back
		numDigits += 3;
		int x = GRID_WIDTH + hOffset -10 - numDigits * 5;
		tft.setCursor(x, y);
		tft.print(volt);
	}

}





// ------------------------
void clearStats(void)	{
// ------------------------
	tft.fillRect(hOffset, vOffset, GRID_WIDTH, 80, ILI9341_BLACK);
}



// ------------------------
void banner(void)	{
// ------------------------
	tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
	tft.setTextSize(2);
	tft.setCursor(110, 30);
	tft.print("DSO138");
	tft.drawRect(100, 25, 88, 25, ILI9341_MAGENTA);

	tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
	tft.setTextSize(1);
	tft.setCursor(30, 70);
	tft.print("Dual Channel O-Scope with logic analyzer");

	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setCursor(30, 95);
	tft.print("Usage: ");
	tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
	tft.print(PROJECT_URL);

	tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
	tft.setCursor(30, 120);
	tft.print("DSO-138 hardware by JYE-Tech");

	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setCursor(30, 145);
	tft.print("Firmware version: ");
	tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
	tft.print(FIRMWARE_VERSION);

	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setTextSize(1);
	tft.setCursor(30, 200);
	tft.print("GNU GENERAL PUBLIC LICENSE Version 3");
}

// ------------------------
uint16_t getXYColorForPoint(uint16_t index, uint8_t baseBrightness) {
// ------------------------
	 if(index >= currentBufferSize) return AN_SIGNALX; // Safety check
	
	// Get digital intensity for D2 (color modulation)
	uint16_t d2Intensity = getDigitalIntensity(index, 1, 16); // Channel 1 (D2)
	
	// Full hue wheel based on D2 duty cycle
	uint8_t r, g, b;
	
	if(d2Intensity < 85) {
		// 0-33% duty: Cyan to Blue (decrease green)
		uint8_t mix = (d2Intensity * 3); // 0-255 scale
		r = 0;
		g = 255 - mix;
		b = 255;
	}
	else if(d2Intensity < 170) {
		// 33-66% duty: Blue to Magenta (increase red)
		uint8_t mix = (d2Intensity - 85) * 3;
		r = mix;
		g = 0;
		b = 255;
	}
	else {
		// 66-100% duty: Magenta to Red (decrease blue)
		uint8_t mix = (d2Intensity - 170) * 3;
		r = 255;
		g = 0;
		b = 255 - mix;
	}
	
	// Apply tail fading
	r = (r * baseBrightness) / 255;
	g = (g * baseBrightness) / 255;
	b = (b * baseBrightness) / 255;
	
	return tft.color565(r, g, b);
}

// ------------------------
uint16_t getDigitalIntensity(uint16_t currentIndex, uint8_t channel, uint8_t window) {
// ------------------------
	// channel: 0 for D1, 1 for D2
	uint16_t digitalMask = (channel == 0) ? DIGITAL_D1_MASK : DIGITAL_D2_MASK;
	
	// Calculate digital frequency/duty cycle intensity (0-255)
	uint16_t startIdx = (currentIndex >= window) ? currentIndex - window : 0;
	uint16_t highCount = 0;
	uint16_t samplesCounted = 0;
	
	// Count how many samples are HIGH in the window
	for(uint16_t i = startIdx; i <= currentIndex && i < currentBufferSize && samplesCounted < window; i++) {
		if(bitStore[i] & digitalMask) {
			highCount++;
		}
		samplesCounted++;
	}
	
	// Convert to intensity (0-255) based on duty cycle
	if(samplesCounted == 0) return 0;
	return (highCount * 255) / samplesCounted;
}

// ------------------------
uint8_t getPointSizeFromDigital(uint16_t index) {
// ------------------------
	if(index >= currentBufferSize) return 2; // Safety check
	
	// Get digital intensity for D1 (point size)
	uint16_t d1Intensity = getDigitalIntensity(index, 0, 16); // Channel 0 (D1)
	
	// Point size based on D1 duty cycle
	if(d1Intensity > 192) return 1;	  // 75-100% duty - small point
	if(d1Intensity > 128) return 2;	  // 50-75% duty - medium point  
	if(d1Intensity > 64) return 3;	   // 25-50% duty - large point
	return 4;							// 0-25% duty - extra large point
}

// ------------------------
void drawXYWaveform(uint16_t pointCount) {
// ------------------------
	static uint32_t lastXYDraw = 0;
	uint32_t drawInterval = (operationMode == MODE_XY) ? map(currentTimeBase, T20US, T50MS, 10, 50) : 20;
	
	if(millis() - lastXYDraw < drawInterval) return;
	lastXYDraw = millis();
	
	// Clear and draw grid
	tft.fillRect(hOffset, vOffset, GRID_WIDTH, GRID_HEIGHT, ILI9341_BLACK);
	drawGrid();
	
	if(pointCount == 0) return;
	
	// Safety check for tail length
	uint16_t safeTailLength = min(tailLength, pointCount);
	if(safeTailLength == 0) safeTailLength = 1;
	
	uint16_t tailStart = (pointCount > safeTailLength) ? pointCount - safeTailLength : 0;
	
	// Draw all points in tail
	for(uint16_t i = tailStart; i < pointCount; i++) {
		// Calculate fade for older points
		uint8_t fade = 255 - ((pointCount - i) * 255 / safeTailLength);
		
		// Get color based on D2 frequency
		uint16_t pointColor = getXYColorForPoint(i, fade);
		
		// Get point size based on D1 frequency  
		uint8_t pointSize = getPointSizeFromDigital(i);
		
		// Calculate position
		int16_t xPos = ((ch1Capture[i] - zeroVoltageA1) * GRID_WIDTH) / (ADC_2_GRID * 2);
		int16_t yPos = ((ch2Capture[i] - zeroVoltageA2) * GRID_HEIGHT) / ADC_2_GRID;
		int16_t screenX = constrain(GRID_WIDTH/2 + xPos + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
		int16_t screenY = constrain(GRID_HEIGHT/2 + vOffset - yPos, vOffset, vOffset + GRID_HEIGHT - 1);
		
		// Draw the point with size
		if(pointSize == 1) {
			tft.drawPixel(screenX, screenY, pointColor);
		} else {
			tft.fillCircle(screenX, screenY, pointSize, pointColor);
		}
	}
	
	// Draw connecting lines if enabled
	if(xylines) {
		for(uint16_t i = tailStart; i < pointCount - 1; i++) {
			uint8_t fade1 = 255 - ((pointCount - i) * 255 / safeTailLength);
			uint8_t fade2 = 255 - ((pointCount - (i + 1)) * 255 / safeTailLength);
			uint8_t lineFade = (fade1 > fade2) ? fade1 : fade2;
			
			uint16_t lineColor = getXYColorForPoint(i, lineFade);
			
			// Calculate positions for line endpoints
			int16_t x1 = ((ch1Capture[i] - zeroVoltageA1) * GRID_WIDTH) / (ADC_2_GRID * 2);
			int16_t y1 = ((ch2Capture[i] - zeroVoltageA2) * GRID_HEIGHT) / ADC_2_GRID;
			int16_t x2 = ((ch1Capture[i+1] - zeroVoltageA1) * GRID_WIDTH) / (ADC_2_GRID * 2);
			int16_t y2 = ((ch2Capture[i+1] - zeroVoltageA2) * GRID_HEIGHT) / ADC_2_GRID;
			
			int16_t screenX1 = constrain(GRID_WIDTH/2 + x1 + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
			int16_t screenY1 = constrain(GRID_HEIGHT/2 + vOffset - y1, vOffset, vOffset + GRID_HEIGHT - 1);
			int16_t screenX2 = constrain(GRID_WIDTH/2 + x2 + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
			int16_t screenY2 = constrain(GRID_HEIGHT/2 + vOffset - y2, vOffset, vOffset + GRID_HEIGHT - 1);
			
			tft.drawLine(screenX1, screenY1, screenX2, screenY2, lineColor);
		}
	}
	
	// Draw current point (brightest and largest)
	uint16_t currentIdx = pointCount - 1;
	uint16_t currentColor = getXYColorForPoint(currentIdx, 255);
	uint8_t currentSize = getPointSizeFromDigital(currentIdx);
	
	int16_t xPos = ((ch1Capture[currentIdx] - zeroVoltageA1) * GRID_WIDTH) / (ADC_2_GRID * 2);
	int16_t yPos = ((ch2Capture[currentIdx] - zeroVoltageA2) * GRID_HEIGHT) / ADC_2_GRID;
	int16_t screenX = constrain(GRID_WIDTH/2 + xPos + hOffset, hOffset, hOffset + GRID_WIDTH - 1);
	int16_t screenY = constrain(GRID_HEIGHT/2 + vOffset - yPos, vOffset, vOffset + GRID_HEIGHT - 1);
	
	if(currentSize == 1) {
		tft.drawPixel(screenX, screenY, currentColor);
	} else {
		tft.fillCircle(screenX, screenY, currentSize, currentColor);
	}
	// Highlight current point
	tft.drawCircle(screenX, screenY, currentSize + 1, ILI9341_WHITE);
}
