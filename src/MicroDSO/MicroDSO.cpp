
#include "MicroDSO.h"
#include "variables.h"
#include "io.h"
#include "display.h"
#include "capture.h"
#include "zconfig.h"
#include "interface.h"
#include "buttons.h"


uint8_t triggerType;


void captureDisplayCycle (bool wTimeOut);


// ------------------------

void
setTriggerType (uint8_t tType)
{
  // ------------------------
  triggerType = tType;
  // break any running capture loop
  keepSampling = false;
}

// ------------------------
void MicroDSO_Setup(void) {
// ------------------------
    DBG_INIT(SERIAL_BAUD_RATE);
    DBG_PRINT("Dual channel O Scope with two logic channels, ver: ");
    DBG_PRINTLN(FIRMWARE_VERSION);

    // Initialize systems in order
    initButtons();      // Initialize button handler FIRST
    initIO();           // Then IO (without interrupt attachments)
    
    // Load config and continue as before...
    loadConfig(digitalRead(BTN4) == LOW);
    
    if(!ch1Capture || !ch2Capture || !bitStore) {
        initializeBuffers();
    }
  // init the IL9341 display
  initDisplay ();
}


// ------------------------
static bool hold_ = true;

// ------------------------
void MicroDSO_Loop(void) {
// ------------------------
    static bool hold_ = true;
    static bool lastXYMode = false;
    
    // Process buttons EVERY loop - highest priority
    processButtons();
    
    // Handle button events
    uint8_t btnEvent = getButtonEvent();
    switch(btnEvent) {
        case BTN_ENCODER:
            focusNextLabel();
            repaintLabels();
            if(hold) {
                drawWaves();
            } else {
                // Stop any ongoing sampling when button pressed
                if(samplingActive) {
                    stopSampling();
                }
                if(triggerType != TRIGGER_AUTO) {
                    keepSampling = false;
                }
            }
            break;
            
        case BTN_HOLD:
            hold = !hold;
            if(hold && samplingActive) {
                stopSampling(); // Stop sampling when entering hold
            }
            repaintLabels();
            break;
            
        case BTN_HOLD_LONG:
            resetParam();
            break;
    }
    
    // Read analog switches
    readInpSwitches();
    
    // Handle mode transitions
    if(xyMode != lastXYMode) {
        clearWaves();
        lastXYMode = xyMode;
        if(samplingActive) {
            stopSampling();
        }
    }
    
    // Handle sampling modes - NON-BLOCKING
    if(!hold) {
        if(directSamplingMode) {
            // Real-time direct sampling (non-blocking)
            indicateCapturing();
            sampleSinglePoint();
            updateDirectDisplay();
            indicateCapturingDone();
        } 
        else if(triggerType == TRIGGER_AUTO) {
            if(!samplingActive) {
                startSampling(true); // Start with timeout
                indicateCapturing();
            }
            
            // Process sampling in small chunks
            processSampling();
            
            if(isSamplingComplete()) {
                indicateCapturingDone();
                drawWaves();
                if(triggered) blinkLED();
            }
        }
        else if(triggerType == TRIGGER_NORM) {
            if(!samplingActive) {
                startSampling(false); // Start without timeout
                indicateCapturing();
            }
            
            processSampling();
            
            if(isSamplingComplete()) {
                indicateCapturingDone();
                drawWaves();
                if(triggered) blinkLED();
            }
        }
        else if(triggerType == TRIGGER_SINGLE && !singleTriggerDone) {
            if(!samplingActive) {
                clearWaves();
                startSampling(false);
                indicateCapturing();
            }
            
            processSampling();
            
            if(isSamplingComplete()) {
                indicateCapturingDone();
                hold = true;
                singleTriggerDone = true;
                repaintLabels();
                drawWaves();
                blinkLED();
                dumpSamples();
            }
        }
    } else {
        // If we're in hold mode but sampling is active, stop it
        if(samplingActive) {
            stopSampling();
        }
    }
    
    // Update display state
    if(hold != hold_) {
        drawLabels();
        hold_ = hold;
        if(!hold) singleTriggerDone = false;
    }
    
    // Handle label repaint requests
    if(paintLabels) {
        drawLabels();
        paintLabels = false;
    }

  // freeze display if requested
  //while(hold);
}
