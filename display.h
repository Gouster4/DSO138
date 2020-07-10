
#ifndef DISPLAY_H
#define DISPLAY_H

// TFT display constants
#define PORTRAIT     0
#define LANDSCAPE     1

#define TFT_WIDTH   320
#define TFT_HEIGHT    240
#define GRID_WIDTH    300
#define GRID_HEIGHT   210

#define GRID_COLOR    0x4208
#define ADC_MAX_VAL   4096
#define ADC_2_GRID    800

enum {L_timebase, L_triggerType, L_triggerEdge, L_triggerLevel, L_waves, L_window, L_vPos1, L_vPos2, L_vPos3, L_vPos4};

extern uint8_t currentFocus;
extern bool printStats;
extern int16_t xCursor;
extern int16_t yCursors[4];
extern bool waves[4];

void clearWaves(void);
void indicateCapturing(void);
void indicateCapturingDone(void);
void repaintLabels(void);
void drawWaves(void);
void drawLabels(void);
void focusNextLabel(void);
void calculateStats(void);

#endif
