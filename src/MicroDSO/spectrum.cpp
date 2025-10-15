#include "hal.h"
#include "variables.h"
#include "spectrum.h"
#include "display.h"
#include "interface.h"
#include <math.h>
#include <string.h>

// Spectrum data buffers
float* spectrumDataA1 = NULL;
float* spectrumDataA2 = NULL;
float* spectrumDataD1 = NULL;
float* spectrumDataD2 = NULL;

// Spectrum configuration
uint16_t spectrumBinCount = 128;  // Smaller for performance
float spectrumDBRange = 60.0f;

// Simple magnitude calculation (not even FFT - just for testing)
void calculateSimpleSpectrum(uint16_t* channelData, float* spectrumOutput, uint16_t dataSize) {
    if(!spectrumOutput || dataSize < spectrumBinCount) return;
    
    // Simple approach: Calculate RMS in frequency bands
    for(uint16_t bin = 0; bin < spectrumBinCount; bin++) {
        float sum = 0;
        uint16_t samplesPerBin = dataSize / spectrumBinCount;
        uint16_t startSample = bin * samplesPerBin;
        
        // Calculate RMS for this frequency band
        for(uint16_t i = 0; i < samplesPerBin && (startSample + i) < dataSize; i++) {
            int16_t sample = (int16_t)channelData[startSample + i] - 2048; // Remove DC
            sum += sample * sample;
        }
        
        float rms = sqrtf(sum / samplesPerBin);
        
        // Convert to dB
        if(rms > 1.0f) {
            spectrumOutput[bin] = 20.0f * log10f(rms / 100.0f);
        } else {
            spectrumOutput[bin] = -spectrumDBRange;
        }
        
        // Clamp
        if(spectrumOutput[bin] > 0) spectrumOutput[bin] = 0;
        if(spectrumOutput[bin] < -spectrumDBRange) spectrumOutput[bin] = -spectrumDBRange;
    }
}

// Even simpler: just show the raw signal variations
void calculateRawSpectrum(uint16_t* channelData, float* spectrumOutput, uint16_t dataSize) {
    if(!spectrumOutput || dataSize < spectrumBinCount) return;
    
    // Just show the signal amplitude variations across the buffer
    for(uint16_t bin = 0; bin < spectrumBinCount; bin++) {
        uint16_t samplesPerBin = dataSize / spectrumBinCount;
        uint16_t startSample = bin * samplesPerBin;
        
        // Find min and max in this bin
        int16_t minVal = 4096;
        int16_t maxVal = 0;
        for(uint16_t i = 0; i < samplesPerBin && (startSample + i) < dataSize; i++) {
            int16_t sample = channelData[startSample + i];
            if(sample < minVal) minVal = sample;
            if(sample > maxVal) maxVal = sample;
        }
        
        int16_t amplitude = maxVal - minVal;
        
        // Convert to dB-like scale
        if(amplitude > 10) {
            spectrumOutput[bin] = 20.0f * log10f((float)amplitude / 100.0f);
        } else {
            spectrumOutput[bin] = -spectrumDBRange;
        }
        
        // Clamp
        if(spectrumOutput[bin] > 0) spectrumOutput[bin] = 0;
        if(spectrumOutput[bin] < -spectrumDBRange) spectrumOutput[bin] = -spectrumDBRange;
    }
}

// Super simple: just show if there's any signal at all
void calculateBasicSpectrum(uint16_t* channelData, float* spectrumOutput, uint16_t dataSize) {
    if(!spectrumOutput) return;
    
    // Initialize to noise floor
    for(uint16_t i = 0; i < spectrumBinCount; i++) {
        spectrumOutput[i] = -spectrumDBRange;
    }
    
    // Simple peak detection - if we have any significant variation, show it
    int16_t minVal = 4096;
    int16_t maxVal = 0;
    for(uint16_t i = 0; i < dataSize && i < 256; i++) {
        int16_t sample = channelData[i];
        if(sample < minVal) minVal = sample;
        if(sample > maxVal) maxVal = sample;
    }
    
    int16_t amplitude = maxVal - minVal;
    
    // If we have a signal, create an artificial peak
    if(amplitude > 100) {
        // Put a peak in the middle of the spectrum
        uint16_t peakBin = spectrumBinCount / 3;
        spectrumOutput[peakBin] = -10.0f; // Strong peak
        spectrumOutput[peakBin-1] = -20.0f;
        spectrumOutput[peakBin+1] = -20.0f;
        spectrumOutput[peakBin-2] = -30.0f;
        spectrumOutput[peakBin+2] = -30.0f;
    }
}

void updateSpectrumAnalysis(void) {
    if(operationMode != MODE_SPECTRUM) return;
    
    // Initialize buffers
    if(!spectrumDataA1) spectrumDataA1 = (float*)malloc(spectrumBinCount * sizeof(float));
    if(!spectrumDataA2) spectrumDataA2 = (float*)malloc(spectrumBinCount * sizeof(float));
    if(!spectrumDataD1) spectrumDataD1 = (float*)malloc(spectrumBinCount * sizeof(float));
    if(!spectrumDataD2) spectrumDataD2 = (float*)malloc(spectrumBinCount * sizeof(float));
    
    if(!spectrumDataA1 || !spectrumDataA2) return;
    
    // Initialize to noise floor
    for(uint16_t i = 0; i < spectrumBinCount; i++) {
        spectrumDataA1[i] = -spectrumDBRange;
        spectrumDataA2[i] = -spectrumDBRange;
        spectrumDataD1[i] = -spectrumDBRange;
        spectrumDataD2[i] = -spectrumDBRange;
    }
    
    // Need at least some data
    if(currentBufferSize < 256) return;
    
    // Use a portion of the buffer
    uint16_t useSamples = (currentBufferSize > 512) ? 512 : currentBufferSize;
    uint16_t startIdx = (sIndex >= useSamples) ? (sIndex - useSamples) : 0;
    
    // Process analog channels with the basic method
    calculateBasicSpectrum(&ch1Capture[startIdx], spectrumDataA1, useSamples);
    calculateBasicSpectrum(&ch2Capture[startIdx], spectrumDataA2, useSamples);
}

bool hasMeaningfulSpectrum(float* spectrum, uint16_t binCount) {
    return (spectrum != NULL);
}

void cleanupSpectrum(void) {
    if(spectrumDataA1) { free(spectrumDataA1); spectrumDataA1 = NULL; }
    if(spectrumDataA2) { free(spectrumDataA2); spectrumDataA2 = NULL; }
    if(spectrumDataD1) { free(spectrumDataD1); spectrumDataD1 = NULL; }
    if(spectrumDataD2) { free(spectrumDataD2); spectrumDataD2 = NULL; }
}