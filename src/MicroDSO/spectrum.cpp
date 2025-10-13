#include "hal.h"
#include "variables.h"
#include "spectrum.h"
#include "display.h"
#include "interface.h"
#include <math.h>
#include <string.h>

void applyWindow(float* data, uint16_t n, uint8_t windowType) {
    switch(windowType) {
        case WINDOW_HAMMING:
            for(uint16_t i = 0; i < n; i++) {
                data[i] *= 0.54f - 0.46f * cosf(2 * M_PI * i / (n-1));
            }
            break;
        case WINDOW_HANNING:
            for(uint16_t i = 0; i < n; i++) {
                data[i] *= 0.5f * (1 - cosf(2 * M_PI * i / (n-1)));
            }
            break;
        case WINDOW_RECTANGULAR:
        default:
            // No window applied - rectangular window
            break;
    }
}

// Add the missing calculateMagnitude function:
void calculateMagnitude(float* real, float* imag, float* magnitude, uint16_t n) {
    for(uint16_t i = 0; i < n/2; i++) {
        float mag = sqrtf(real[i]*real[i] + imag[i]*imag[i]);
        magnitude[i] = (mag > 0) ? 20.0f * log10f(mag) : -spectrumDBRange;
    }
}

// Simple FFT implementation
void fft(float* real, float* imag, uint16_t n) {
    uint16_t i, j, k, m;
    float theta, w_real, w_imag, tmp_real, tmp_imag;
    
    // Bit reversal
    j = 0;
    for(i = 0; i < n-1; i++) {
        if(i < j) {
            tmp_real = real[i];
            real[i] = real[j];
            real[j] = tmp_real;
            tmp_imag = imag[i];
            imag[i] = imag[j];
            imag[j] = tmp_imag;
        }
        k = n >> 1;
        while(k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    
    // FFT computation - iterative version
    uint16_t step = 1;
    while(step < n) {
        m = step << 1;
        theta = -M_PI / step;
        w_real = cos(theta);
        w_imag = sin(theta);
        
        for(k = 0; k < step; k++) {
            float wr = cos(k * theta);
            float wi = sin(k * theta);
            
            for(i = k; i < n; i += m) {
                j = i + step;
                tmp_real = wr * real[j] - wi * imag[j];
                tmp_imag = wr * imag[j] + wi * real[j];
                real[j] = real[i] - tmp_real;
                imag[j] = imag[i] - tmp_imag;
                real[i] += tmp_real;
                imag[i] += tmp_imag;
            }
        }
        step = m;
    }
}

void calculateChannelSpectrum(uint16_t* channelData, float* spectrumOutput, uint16_t dataSize) {
    if(!spectrumOutput || dataSize < spectrumBinCount) return;
    
    float* real = (float*)malloc(spectrumBinCount * sizeof(float));
    float* imag = (float*)malloc(spectrumBinCount * sizeof(float));
    
    if(!real || !imag) return;
    
    // Copy data and remove DC offset
    int32_t sum = 0;
    for(uint16_t i = 0; i < dataSize && i < spectrumBinCount; i++) {
        sum += channelData[i];
    }
    float dcOffset = (float)sum / dataSize;
    
    for(uint16_t i = 0; i < spectrumBinCount && i < dataSize; i++) {
        real[i] = (float)channelData[i] - dcOffset;
        imag[i] = 0.0f;
    }
    
    // Zero pad
    for(uint16_t i = dataSize; i < spectrumBinCount; i++) {
        real[i] = 0.0f;
        imag[i] = 0.0f;
    }
    
    applyWindow(real, spectrumBinCount, spectrumWindow);
    
    // Perform FFT
    fft(real, imag, spectrumBinCount);
    
    // Calculate magnitude spectrum
    for(uint16_t i = 0; i < spectrumBinCount/2; i++) {
        float mag = sqrtf(real[i]*real[i] + imag[i]*imag[i]);
        
        // Normalize by number of points
        mag = mag / (spectrumBinCount / 4);
        
        // Convert to dB with a reasonable reference level
        // This ensures most real-world signals fall within 0 to -60dB
        float db = 0.0f;
        if(mag > 1e-6f) {
            db = 20.0f * log10f(mag);
            // Apply offset to get values in our display range
            db += 40.0f; // Adjust this to move the entire spectrum up/down
        } else {
            db = -spectrumDBRange; // Minimum value
        }
        
        // Clamp to display range
        if(db > 0) db = 0;
        if(db < -spectrumDBRange) db = -spectrumDBRange;
        
        spectrumOutput[i] = db;
    }
    
    free(real);
    free(imag);
}

void updateSpectrumAnalysis(void) {
    if(operationMode != MODE_SPECTRUM) return;
    
    // Extract the most recent contiguous block for FFT
    uint16_t fftSize = min(spectrumBinCount, currentBufferSize);
    uint16_t* tempBuffer1 = (uint16_t*)malloc(fftSize * sizeof(uint16_t));
    uint16_t* tempBuffer2 = (uint16_t*)malloc(fftSize * sizeof(uint16_t));
    
    if(!tempBuffer1 || !tempBuffer2) return;
    
    // Get the most recent 'fftSize' samples (handles circular buffer)
    uint16_t startIdx;
    if(sIndex >= fftSize) {
        // We have enough recent samples in linear order
        startIdx = sIndex - fftSize;
        memcpy(tempBuffer1, &ch1Capture[startIdx], fftSize * sizeof(uint16_t));
        memcpy(tempBuffer2, &ch2Capture[startIdx], fftSize * sizeof(uint16_t));
    } else {
        // Need to wrap around the circular buffer
        uint16_t part1 = fftSize - sIndex;
        uint16_t part2 = sIndex;
        
        // Copy end of buffer (older data)
        memcpy(tempBuffer1, &ch1Capture[currentBufferSize - part1], part1 * sizeof(uint16_t));
        memcpy(tempBuffer2, &ch2Capture[currentBufferSize - part1], part1 * sizeof(uint16_t));
        
        // Copy beginning of buffer (newer data)  
        memcpy(&tempBuffer1[part1], ch1Capture, part2 * sizeof(uint16_t));
        memcpy(&tempBuffer2[part1], ch2Capture, part2 * sizeof(uint16_t));
    }
    
    // Process the extracted contiguous block
    calculateChannelSpectrum(tempBuffer1, spectrumDataA1, fftSize);
    calculateChannelSpectrum(tempBuffer2, spectrumDataA2, fftSize);
    
    // Process digital channels similarly
    uint16_t* digitalTemp1 = (uint16_t*)malloc(fftSize * sizeof(uint16_t));
    uint16_t* digitalTemp2 = (uint16_t*)malloc(fftSize * sizeof(uint16_t));
    
    if(digitalTemp1 && digitalTemp2) {
        // Extract digital data using same logic
        if(sIndex >= fftSize) {
            startIdx = sIndex - fftSize;
            for(uint16_t i = 0; i < fftSize; i++) {
                digitalTemp1[i] = (bitStore[startIdx + i] & DIGITAL_D1_MASK) ? 4095 : 0;
                digitalTemp2[i] = (bitStore[startIdx + i] & DIGITAL_D2_MASK) ? 4095 : 0;
            }
        } else {
            uint16_t part1 = fftSize - sIndex;
            for(uint16_t i = 0; i < part1; i++) {
                digitalTemp1[i] = (bitStore[currentBufferSize - part1 + i] & DIGITAL_D1_MASK) ? 4095 : 0;
                digitalTemp2[i] = (bitStore[currentBufferSize - part1 + i] & DIGITAL_D2_MASK) ? 4095 : 0;
            }
            for(uint16_t i = 0; i < sIndex; i++) {
                digitalTemp1[part1 + i] = (bitStore[i] & DIGITAL_D1_MASK) ? 4095 : 0;
                digitalTemp2[part1 + i] = (bitStore[i] & DIGITAL_D2_MASK) ? 4095 : 0;
            }
        }
        
        calculateChannelSpectrum(digitalTemp1, spectrumDataD1, fftSize);
        calculateChannelSpectrum(digitalTemp2, spectrumDataD2, fftSize);
        
        free(digitalTemp1);
        free(digitalTemp2);
    }
    
    free(tempBuffer1);
    free(tempBuffer2);
}

bool hasMeaningfulSpectrum(float* spectrum, uint16_t binCount) {
    if(!spectrum) return false;
    
    uint16_t significantBins = 0;
    float noiseFloor = -spectrumDBRange + 6.0f;
    
    for(uint16_t i = 1; i < binCount/4 && i < 128; i++) {
        if(spectrum[i] > noiseFloor) {
            significantBins++;
        }
    }
    
    return (significantBins >= 3);
}

void cleanupSpectrum(void) {
    if(spectrumDataA1) { free(spectrumDataA1); spectrumDataA1 = NULL; }
    if(spectrumDataA2) { free(spectrumDataA2); spectrumDataA2 = NULL; }
    if(spectrumDataD1) { free(spectrumDataD1); spectrumDataD1 = NULL; }
    if(spectrumDataD2) { free(spectrumDataD2); spectrumDataD2 = NULL; }
}