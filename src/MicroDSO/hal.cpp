#include <Arduino.h>
#include "hal.h"


timer Timer2(TIM2);
timer Timer3(TIM3);
timer Timer4(TIM4);

//timer 

timer::timer(TIM_TypeDef *Instance)
{
  Tim = new HardwareTimer(Instance);
}

timer::~timer()
{
  delete Tim;
}

void timer::setChannel1Mode(TimerModes_t flags)
{
  Tim->setMode(1, flags);
}

void timer::pause(void)
{
  Tim->pause();
}

void timer::resume(void)
{
  Tim->resume();
}

void timer::setCompare1(unsigned int val)
{
  Tim->setCaptureCompare(1, val);
}

void timer::attachCompare1Interrupt( void (*fptr)() )
{
  Tim->attachInterrupt(1, fptr);
}
 
void timer::setCount(unsigned int val)
{
 Tim->setCount(val);
}

void timer::setPeriod(unsigned int val)
{
 Tim->setOverflow(val);
}
  

//ADC

/*
void adc_init(adc_dev *dev) {
    rcc_clk_enable(dev->clk_id);
    rcc_reset_dev(dev->clk_id);
}

*/

void adc_calibrate(ADC_TypeDef * adc)
{
}

void adc_set_sample_rate(ADC_TypeDef *adc, unsigned int smp_rate)
{
    uint32 adc_smpr1_val = 0, adc_smpr2_val = 0;
    int i;

    for (i = 0; i < 10; i++) {
        if (i < 8) {
            /* ADC_SMPR1 determines sample time for channels [10,17] */
            adc_smpr1_val |= smp_rate << (i * 3);
        }
        /* ADC_SMPR2 determines sample time for channels [0,9] */
        adc_smpr2_val |= smp_rate << (i * 3);
    }

    adc->SMPR1 = adc_smpr1_val;
    adc->SMPR2 = adc_smpr2_val;
}

void adc_set_reg_seqlen(ADC_TypeDef *adc, unsigned int len)
{
    uint32 tmp = adc->SQR1;
    tmp &= ~ADC_SQR1_L;
    tmp |= (len - 1) << 20;
    adc->SQR1 = tmp;
}
  
void adc_set_channel(ADC_TypeDef * adc , unsigned int channel)
{
  adc->SQR3 = channel;
}
  
void adc_set_mode(ADC_TypeDef * adc, unsigned int CR1, unsigned int CR2)
{
  adc->CR1 |= CR1;
  adc->CR2 |= CR2;
}

unsigned int adc_read(ADC_TypeDef * adc)
{
  return  adc->DR & ADC_DR_DATA;
}
  
void pwmWrite(unsigned int V1, unsigned int V2)
{
}

#ifndef ARDUINO

//Arduino Function Wrapper...

void delayMS(uint32_t ms)
{
  HAL_Delay(ms);
}


#define STM32_DELAY_US_MULT (SystemCoreClock / 6000000U)
/*inline*/ void delayUS(uint32_t us)
{
    us *= STM32_DELAY_US_MULT;

    /* fudge for function call overhead  */
    us--;
    asm volatile("   mov r0, %[us]          \n\t"
                 "1: subs r0, #1            \n\t"
                 "   bhi 1b                 \n\t"
                 :
                 : [us] "r" (us)
                 : "r0");
}

uint32_t millis(void)
{
  return HAL_GetTick();
}

#define SYSTICK_RELOAD_VAL (SystemCoreClock/1000)
#define CYCLES_PER_MICROSECOND (SystemCoreClock / 1000000L)

uint32_t micros(void)
{
  uint32_t ms;
    uint32_t cycle_cnt;

    do {
        ms = millis();
        cycle_cnt = SysTick->VAL;
        asm volatile("nop"); //allow interrupt to fire
        asm volatile("nop");
    } while (ms != millis());

#define US_PER_MS               1000
    /* SYSTICK_RELOAD_VAL is 1 less than the number of cycles it
     * actually takes to complete a SysTick reload */
    return ((ms * US_PER_MS) + (SYSTICK_RELOAD_VAL + 1 - cycle_cnt) / CYCLES_PER_MICROSECOND);
#undef US_PER_MS
}

void setPinMode(GPIO_TypeDef  *GPIOx,uint32_t Pin,uint32_t Mode, uint32_t Pull,uint32_t Speed)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = Pin;
  GPIO_InitStruct.Mode = Mode;
  GPIO_InitStruct.Pull = Pull;
  GPIO_InitStruct.Speed = Speed;
  HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

/*
void timerPause(TIM_HandleTypeDef *htim)
{
  // Check the TIM handle allocation
  if(htim == NULL)
  {
  return;
  }

  htim->Instance->CR1 = htim->Instance->CR1 & ~TIM_CR1_CEN;
}

void timerResume(TIM_HandleTypeDef *htim)
{
  // Check the TIM handle allocation
  if(htim == NULL)
  {
  return;
  }

  htim->Instance->CR1 = htim->Instance->CR1 | TIM_CR1_CEN;
}



static inline void timerSetOverflow(TIM_HandleTypeDef *htim, uint16_t arr)
{
  htim->Instance->ARR = arr;
}

static inline void timerSetPrescaler(TIM_HandleTypeDef *htim, uint16_t psc)
{
  htim->Instance->PSC = psc-1;
}

#define MAX_RELOAD ((1 << 16) - 1)
void timerSetPeriod(TIM_HandleTypeDef *htim,uint32_t ms_period)
{
  // Check the TIM handle allocation
  if(htim == NULL)
  {
  return;
  }

  uint32_t period_cyc = ms_period * CYCLES_PER_MICROSECOND;
  uint16_t prescaler = (uint16_t)(period_cyc / MAX_RELOAD + 1);
  uint16_t overflow = (uint16_t)((period_cyc + (prescaler / 2)) / prescaler);
  timerSetPrescaler(htim,prescaler);
  timerSetOverflow(htim,overflow);

}

void timerSetCount(TIM_HandleTypeDef *htim,uint32_t count)
{
  // Check the TIM handle allocation
  if(htim == NULL)
  {
  return;
  }
  htim->Instance->CNT = count;
}


void timerSetPWM(TIM_HandleTypeDef *htim,uint32_t Channel,uint16_t perc)
{
  // Check the TIM handle allocation
  if(htim == NULL)
  {
  return;
  }

  timerSetCompare(htim,Channel,perc);

}


void timerSetCompare(TIM_HandleTypeDef *htim,uint32_t Channel,uint32_t val)
{
  // Check the TIM handle allocation
  if(htim == NULL)
  {
  return;
  }
  volatile uint32_t *ccr = &htim->Instance->CCR1 + (Channel>>2);
  *ccr = val;
}
*/
#endif
