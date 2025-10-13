#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "hal.h"
#include "variables.h"

// Spectrum analysis functions
void updateSpectrumAnalysis(void);
void cleanupSpectrum(void);
bool hasMeaningfulSpectrum(float* spectrum, uint16_t binCount);

// Spectrum display functions  
void drawSpectrum(void);
void drawSpectrumAxisLabels(void);
void drawChannelSpectrum(float* spectrum, uint16_t color, const char* label);
void drawSpectrumLabels(void);
void drawSimpleSpectrum(float* spectrum, uint16_t color);


#endif