#include "buttons.h"
#include "variables.h"
#include "interface.h"

static uint32_t buttonState = 0;
static uint32_t buttonPressTime[2] = {0, 0};
static uint8_t buttonEvent = BTN_NONE;

// ------------------------
void initButtons(void) {
// ------------------------
    pinMode(ENCODER_SW, INPUT_PULLUP);
    pinMode(BTN4, INPUT_PULLUP);
}

// ------------------------
void processButtons(void) {
// ------------------------
    static uint32_t lastButtonCheck = 0;
    uint32_t currentTime = millis();
    
    // Check buttons every 10ms
    if(currentTime - lastButtonCheck < 10) {
        return;
    }
    lastButtonCheck = currentTime;
    
    buttonEvent = BTN_NONE;
    
    // Check encoder switch (ENCODER_SW)
    if(digitalRead(ENCODER_SW) == LOW) {
        if((buttonState & 1) == 0) {
            // Encoder button just pressed
            buttonState |= 1;
            buttonPressTime[0] = currentTime;
        }
    } else {
        if((buttonState & 1) == 1) {
            // Encoder button released - always short press
            buttonState &= ~1;
            if(currentTime - buttonPressTime[0] < 1000) { // Debounce + short press
                buttonEvent = BTN_ENCODER;  // Use BTN_ENCODER constant
            }
        }
    }
    
    // Check hold button (BTN4) - supports short and long press
    if(digitalRead(BTN4) == LOW) {
        if((buttonState & 2) == 0) {
            // Hold button just pressed
            buttonState |= 2;
            buttonPressTime[1] = currentTime;
        } else {
            // Check for long press while held
            if(currentTime - buttonPressTime[1] > 1000) {
                buttonEvent = BTN_HOLD_LONG;  // Use BTN_HOLD_LONG constant
                buttonPressTime[1] = currentTime; // Prevent repeat events
            }
        }
    } else {
        if((buttonState & 2) == 2) {
            // Hold button released
            buttonState &= ~2;
            if(currentTime - buttonPressTime[1] < 1000) {
                // Short press (not long press)
                buttonEvent = BTN_HOLD;  // Use BTN_HOLD constant
            }
        }
    }
}

// ------------------------
uint8_t getButtonEvent(void) {
// ------------------------
    uint8_t event = buttonEvent;
    buttonEvent = BTN_NONE; // Clear after reading
    return event;
}