
#ifndef CAPTURE_H
#define CAPTURE_H

extern bool triggerRising;

void sampleWaves(bool wTimeout);
void dumpSamples(void);
void scanTimeoutISR(void); 
void setSamplingRate(uint8_t timeBase);
void setTriggerRising(bool rising);

#endif
