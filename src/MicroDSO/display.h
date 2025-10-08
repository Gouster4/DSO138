
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

enum {L_timebase, L_triggerType, L_triggerEdge, L_bufferSize, L_triggerLevel, L_waves, L_window, L_zoom, L_vPos1, L_vPos2, L_vPos3, L_vPos4};

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
void initDisplay(void);

void displayDualChannelVoltages(bool clearStats);
void displayVoltageValue(const char* label, float value1, float value2, bool show1, bool show2, bool mvRange);
void displayAllChannelFrequency(bool clearStats);
void displayDualChannelVoltages(bool clearStats);
void calculateStatsForChannel(int channel, Stats &stats);
void calculateStatsForDigitalChannel(int channel, Stats &stats);
void drawCompactVoltage(float volt, bool mvRange);
void displayAllChannelFrequency(bool clearStats);
void displayDualChannelVoltages(bool clearStats);
void calculateStatsForChannel(int channel, Stats &stats);
void calculateStatsForDigitalChannel(int channel, Stats &stats);
void drawCompactVoltage(float volt, bool mvRange);
void drawXYWaveform(uint16_t pointCount);
void setDirectSampling(bool enable);
void setXYMode(bool enable);
uint8_t getDigitalIntensity(uint16_t currentIndex, uint8_t channel, uint8_t window);
uint8_t getPointSizeFromDigital(uint16_t index);
uint16_t getXYColorForPoint(uint16_t index, uint8_t baseBrightness);
#endif
