
#ifndef INTERFACE_H
#define INTERFACE_H

const char* getTimebaseLabel(void);
void encoderChanged(int steps);
void readESwitchISR(void);
void btn4ISR(void);
void setTimeBase(uint8_t timeBase, bool save=true);

#endif
