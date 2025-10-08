#ifndef BUTTONS_H
#define BUTTONS_H

#include "hal.h"

void initButtons(void);
void processButtons(void);
uint8_t getButtonEvent(void);

// Button event types - match what's used in processButtons()
#define BTN_NONE       0
#define BTN_ENCODER    1  // Encoder button short press  
#define BTN_HOLD       2  // Hold button short press
#define BTN_HOLD_LONG  3  // Hold button long press

#endif