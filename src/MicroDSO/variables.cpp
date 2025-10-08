#include "hal.h"
#include "variables.h"

// global capture variables
uint16_t *ch1Capture = NULL;
uint16_t *ch2Capture = NULL;
uint16_t *bitStore = NULL;
uint16_t sIndex = 0;
uint16_t tIndex = 0;
volatile bool triggered = false;

volatile bool keepSampling = true;
long samplingTime;
volatile bool hold = false;
bool xyMode = false;
// waveform calculated statistics
Stats wStats;

const char* cplNames[] = {"GND", "AC", "DC"}; 
const char* rngNames[] = {"5V", "2V", "1V", "0.5V", "0.2V", "0.1V", "50mV", "20mV", "10mV"}; 
const float adcMultiplier[] = {0.05085, 0.02034, 0.01017, 0.005085, 0.002034, 0.001017, 0.5085, 0.2034, 0.1017}; 
const char* bufferModeNames[] = {"FUL", "1/2", "1/4", "1/8"};
// analog switch enumerated values
uint8_t couplingPos, rangePos;

// this represents the offset voltage at ADC input (1.66V), when Analog input is zero
int16_t zeroVoltageA1, zeroVoltageA2;

// timebase enumerations and store
const char* tbNames[] = {"20 uS", "30 uS", "50 uS", "0.1 mS", "0.2 mS", "0.5 mS", "1 mS", "2 mS", "5 mS", "10 mS", "20 mS", "50 mS"}; 
uint8_t currentTimeBase;
uint8_t bufferMode = BUF_FULL;
uint16_t currentBufferSize = NUM_SAMPLES;
const uint16_t bufferSizes[] = {NUM_SAMPLES, NUM_SAMPLES_HALF, NUM_SAMPLES_QUARTER, NUM_SAMPLES_EIGHTH};

// zoom control
uint16_t zoomFactor = ZOOM_DEFAULT;

//XY mode
// Add to variables.cpp
uint16_t tailLength = DEFAULT_TAIL_LENGTH;  // Default tail length
bool tailEnabled = true;   // Tail enabled by default
bool directSamplingMode = false;
uint16_t directSampleCount = 0;
unsigned long lastDirectDrawTime = 0;
bool singleTriggerDone = false;
bool xylines = false;  // Default: dots only in XY mode
