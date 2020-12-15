
#ifndef HAL_H
#define HAL_H

#ifdef ARDUINO
#include"Arduino.h"
typedef unsigned long long uint32;
typedef unsigned int uint16;
typedef unsigned char uint8;
#else
#include"Wrapper.h"
#endif

#define TIMER_OUTPUTCOMPARE TIMER_OUTPUT_COMPARE

#define PWM 1
#define ADC_SMPR_1_5 0x01

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

void adc_calibrate(ADC_TypeDef * adc);
void adc_set_sample_rate(ADC_TypeDef * adc, unsigned int rate);
void adc_set_reg_seqlen(ADC_TypeDef *adc, unsigned int len);
void adc_set_channel(ADC_TypeDef *adc, unsigned int channel);
void adc_set_mode(ADC_TypeDef * , unsigned int CR1, unsigned int CR2);
unsigned int adc_read(ADC_TypeDef * adc);
  
void pwmWrite(unsigned int, unsigned int );

#endif
