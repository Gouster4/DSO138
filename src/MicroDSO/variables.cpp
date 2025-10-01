#include "hal.h"
#include "variables.h"

// global capture variables
uint16_t ch1Capture[NUM_SAMPLES] = {0};
uint16_t ch2Capture[NUM_SAMPLES] = {0};
uint16_t bitStore[NUM_SAMPLES] = {0};
uint16_t sIndex = 0;
uint16_t tIndex = 0;
volatile bool triggered = false;

volatile bool keepSampling = true;
long samplingTime;
volatile bool hold = false;
// waveform calculated statistics
Stats wStats;

const char* cplNames[] = {"GND", "AC", "DC"}; 
const char* rngNames[] = {"5V", "2V", "1V", "0.5V", "0.2V", "0.1V", "50mV", "20mV", "10mV"}; 
const float adcMultiplier[] = {0.05085, 0.02034, 0.01017, 0.005085, 0.002034, 0.001017, 0.5085, 0.2034, 0.1017}; 
// analog switch enumerated values
uint8_t couplingPos, rangePos;

// this represents the offset voltage at ADC input (1.66V), when Analog input is zero
int16_t zeroVoltageA1, zeroVoltageA2;

// timebase enumerations and store
const char* tbNames[] = {"20 uS", "30 uS", "50 uS", "0.1 mS", "0.2 mS", "0.5 mS", "1 mS", "2 mS", "5 mS", "10 mS", "20 mS", "50 mS"}; 
uint8_t currentTimeBase;
bool halfBufferMode = false;
uint16_t currentBufferSize = NUM_SAMPLES;
