
#ifndef HAL_H
#define HAL_H

#include"Arduino.h"

#define TIMER_OUTPUTCOMPARE TIMER_OUTPUT_COMPARE

#define PWM 1
#define ADC_SMPR_1_5 0x01

typedef unsigned long long uint32;
typedef unsigned int uint16;
typedef unsigned char uint8;

class timer
{
public:
  HardwareTimer *Tim;
  timer(TIM_TypeDef *Instance);
  ~timer();
  void setChannel1Mode(TimerModes_t flags);
  void pause(void);
  void resume(void);
  void setCompare1(unsigned int);
  void attachCompare1Interrupt( void (*fptr)() );  
  void setCount(unsigned int);
  void setPeriod(unsigned int);
  
};

extern timer Timer2;
extern timer Timer3;
extern timer Timer4;

typedef struct 
{
int adc_channel;
}pins;


void adc_calibrate(ADC_TypeDef * adc);
void adc_set_sample_rate(ADC_TypeDef * adc, unsigned int rate);
void adc_set_reg_seqlen(ADC_TypeDef *adc, unsigned int len);
void adc_set_channel(ADC_TypeDef *adc, unsigned int channel);
void adc_set_mode(ADC_TypeDef * , unsigned int CR1, unsigned int CR2);
unsigned int adc_read(ADC_TypeDef * adc);
  
void pwmWrite(unsigned int, unsigned int );

#ifndef ARDUINO
#include "stm32f1xx_hal.h"

#define DBG_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__);

extern void delayMS(uint32_t ms);
extern void delayUS(uint32_t us);
void setPinMode(GPIO_TypeDef  *GPIOx,uint32_t Pin,uint32_t Mode, uint32_t Pull,uint32_t Speed);
extern uint32_t millis(void);
extern uint32_t micros(void);
/*
extern void timerPause(TIM_HandleTypeDef *htim);
extern void timerResume(TIM_HandleTypeDef *htim);
extern void timerSetPeriod(TIM_HandleTypeDef *htim,uint32_t ms_period);
extern void  timerSetPWM(TIM_HandleTypeDef *htim,uint32_t Channel,uint16_t perc);
extern void timerSetCompare(TIM_HandleTypeDef *htim,uint32_t Channel,uint32_t val);
extern void timerSetCount(TIM_HandleTypeDef *htim,uint32_t count);
*/
#endif
#endif
