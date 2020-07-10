

#ifndef CONTROL
#define CONTROL

#include "hal.h"

enum { TRIGGER_AUTO, TRIGGER_NORM, TRIGGER_SINGLE };

extern uint8_t triggerType;


void setTriggerType(uint8_t tType);

void DSO_Setup(void);
	
void DSO_Loop(void);  

#endif
