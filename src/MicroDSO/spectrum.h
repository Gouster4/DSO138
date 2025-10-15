#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "hal.h"
#include "variables.h"

// Spectrum configuration
#define SPECTRUM_BIN_COUNT 256
#define SPECTRUM_DB_RANGE 80.0f
#define SPECTRUM_WINDOW_HAMMING 0
#define SPECTRUM_WINDOW_HANNING 1

// Function declarations
void updateSpectrumAnalysis(void);
void cleanupSpectrum(void);
void calculateChannelSpectrum(uint16_t* channelData, float* spectrumOutput, uint16_t dataSize, bool isDigital);
bool hasMeaningfulSpectrum(float* spectrum, uint16_t binCount);

// FFT functions
void fft(float* x, float* y, uint16_t n);
void calculateMagnitude(float* real, float* imag, float* magnitude, uint16_t n);
void applyWindow(float* data, uint16_t n, uint8_t windowType);

// External variables
extern float* spectrumDataA1;
extern float* spectrumDataA2;
extern float* spectrumDataD1;
extern float* spectrumDataD2;
extern uint16_t spectrumBinCount;
extern float spectrumDBRange;
extern uint8_t spectrumWindow;

#endif