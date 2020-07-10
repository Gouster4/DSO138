#include "global.h"

// global capture variables
extern uint16_t ch1Capture[NUM_SAMPLES];
extern uint16_t ch2Capture[NUM_SAMPLES];
extern uint16_t bitStore[NUM_SAMPLES];
extern uint16_t sIndex;
extern uint16_t tIndex;
extern volatile bool triggered;

extern volatile bool keepSampling;
extern long samplingTime;
extern volatile bool hold ;
// waveform calculated statistics
struct Stats {
	bool pulseValid;
	double avgPW;
	float duty;
	float freq;
	float cycle;
	
	bool mvPos;
	float Vrmsf;
	float Vavrf;
	float Vmaxf;
	float Vminf;
};

extern Stats wStats;

enum {CPL_GND, CPL_AC, CPL_DC};
extern  const char* cplNames[]; 
enum {RNG_5V, RNG_2V, RNG_1V, RNG_0_5V, RNG_0_2V, RNG_0_1V, RNG_50mV, RNG_20mV, RNG_10mV};
extern const char* rngNames[]; 
extern const float adcMultiplier[]; 
// analog switch enumerated values
extern uint8_t couplingPos, rangePos;

// this represents the offset voltage at ADC input (1.66V), when Analog input is zero
extern int16_t zeroVoltageA1, zeroVoltageA2;

// timebase enumerations and store
enum {T20US, T30US, T50US, T0_1MS, T0_2MS, T0_5MS, T1MS, T2MS, T5MS, T10MS, T20MS, T50MS};
extern const char* tbNames[]; 
extern uint8_t currentTimeBase;
