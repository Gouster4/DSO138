
#ifndef INTERFACE_H
#define INTERFACE_H

const char* getTimebaseLabel(void);
void encoderChanged(int steps);
void readESwitchISR(void);
void btn4ISR(void);
void setTimeBase(uint8_t timeBase, bool save=true);

// Add these declarations
void cycleBufferSize(void);
void adjustZoomForBufferSize(void);
uint16_t calculateMinZoomForBuffer(void);
bool isZoomCompatibleWithBuffer(uint16_t zoom);
void incrementZoom(void);
void decrementZoom(void);
float getZoomMultiplier(void);
void adjustXCursorForZoom(void);
void initializeBuffers(void);

#endif
