
#ifndef CAPTURE_H
#define CAPTURE_H

extern bool triggerRising;
extern bool samplingActive;
extern uint8_t samplingState;

// Non-blocking sampling functions
void startSampling(bool wTimeout);
void stopSampling(void);
bool isSamplingComplete(void);
void processSampling(void);
void sampleWavesChunk(void);

// Existing functions
void sampleWaves(bool wTimeout);
void dumpSamples(void);
void scanTimeoutISR(void); 
void setSamplingRate(uint8_t timeBase);
void setTriggerRising(bool rising);
void sampleSinglePoint(void);
void updateDirectDisplay(void);

#endif
