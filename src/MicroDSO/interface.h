
#ifndef INTERFACE_H
#define INTERFACE_H

const char* getTimebaseLabel(void);
void encoderChanged(int steps);
void readESwitchISR(void);
void btn4ISR(void);
void setTimeBase(uint8_t timeBase, bool save=true);

// Add these declarations
void incrementBufferSize(void);
void decrementBufferSize(void);
void changeBufferSize(uint8_t newBufferMode);
void adjustZoomForBufferSize(void);
uint16_t calculateMinZoomForBuffer(void);
bool isZoomCompatibleWithBuffer(uint16_t zoom);
void incrementZoom(void);
void decrementZoom(void);
float getZoomMultiplier(void);
void adjustXCursorForZoom(void);
void initializeBuffers(void);
void setOperationMode(uint8_t newMode);
void setTailLength(uint16_t length);
void clearXYBuffer();
void setDirectSampling(bool enable);
void setXYMode(bool enable);
void resetParam(void);
void updateDirectDisplay(void);
void updateSpectrumAnalysis(void);
void cleanupSpectrum(void);
#endif
