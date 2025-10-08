#ifndef IO_H
#define IO_H

extern int16_t trigLevel;

void readInpSwitches(void);
void blinkLED(void);
void initIO(void);
int16_t getTriggerLevel(void);
void setTriggerLevel(int16_t tLvl);

#endif
