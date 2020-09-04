#include <Arduino.h>
#include "hal.h"

timer Timer2(TIM2);
timer Timer3(TIM3);
timer Timer4(TIM4);

//timer 

timer::timer(TIM_TypeDef *Instance) {
	Tim = new HardwareTimer(Instance);
}

timer::~timer() {
	delete Tim;
}

void timer::setChannel1Mode(TimerModes_t flags) {
	Tim->setMode(1, flags);
}

void timer::pause(void) {
	Tim->pause();
}

void timer::resume(void) {
	Tim->resume();
}

void timer::setCompare1(unsigned int val) {
	Tim->setCaptureCompare(1, val);
}

void timer::attachCompare1Interrupt(void (*fptr)()) {
	Tim->attachInterrupt(1, fptr);
}

void timer::setCount(unsigned int val) {
	Tim->setCount(val);
}

void timer::setPeriod(unsigned int val) {
	if (Tim == Timer2.Tim) {
		Tim->setOverflow(val, MICROSEC_FORMAT);
	}
}

//ADC

/*
 void adc_init(adc_dev *dev) {
 rcc_clk_enable(dev->clk_id);
 rcc_reset_dev(dev->clk_id);
 }

 */

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */
	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}
	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief ADC2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC2_Init(void) {

	/* USER CODE BEGIN ADC2_Init 0 */

	/* USER CODE END ADC2_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC2_Init 1 */

	/* USER CODE END ADC2_Init 1 */
	/** Common config
	 */
	hadc2.Instance = ADC2;
	hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc2.Init.ContinuousConvMode = ENABLE;
	hadc2.Init.DiscontinuousConvMode = DISABLE;
	hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc2.Init.NbrOfConversion = 1;
	if (HAL_ADC_Init(&hadc2) != HAL_OK) {
		Error_Handler();
	}
	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_5;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC2_Init 2 */

	/* USER CODE END ADC2_Init 2 */

}
void adc_calibrate(ADC_TypeDef *adc) {

	if (adc == ADC1) {
		MX_ADC1_Init();
	}
	if (adc == ADC2) {
		MX_ADC2_Init();
	}
}

void adc_set_sample_rate(ADC_TypeDef *adc, unsigned int smp_rate) {
	/*
	 uint32 adc_smpr1_val = 0, adc_smpr2_val = 0;
	 int i;

	 for (i = 0; i < 10; i++) {
	 if (i < 8) {
	 // ADC_SMPR1 determines sample time for channels [10,17]
	 adc_smpr1_val |= smp_rate << (i * 3);
	 }
	 // ADC_SMPR2 determines sample time for channels [0,9]
	 adc_smpr2_val |= smp_rate << (i * 3);
	 }

	 adc->SMPR1 = adc_smpr1_val;
	 adc->SMPR2 = adc_smpr2_val;
	 */
}

void adc_set_reg_seqlen(ADC_TypeDef *adc, unsigned int len) {
	/*
	 uint32 tmp = adc->SQR1;
	 tmp &= ~ADC_SQR1_L;
	 tmp |= (len - 1) << 20;
	 adc->SQR1 = tmp;
	 */
}

void adc_set_channel(ADC_TypeDef *adc, unsigned int channel) {

	ADC_ChannelConfTypeDef sConfig = { 0 };
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;

	if (adc == ADC1) {
		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
			Error_Handler();
		}
	} else {
		if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
			Error_Handler();
		}
	}

}

void adc_set_mode(ADC_TypeDef *adc, unsigned int CR1, unsigned int CR2) {
	if (adc == ADC1) {
		HAL_ADC_Start(&hadc1);
	} else {
		HAL_ADC_Start(&hadc2);
	}
}

unsigned int adc_read(ADC_TypeDef *adc) {
	if (adc == ADC1) {
		//while ((hadc1.Instance->SR & 0x02) == 0);
		return HAL_ADC_GetValue(&hadc1);
	} else {
		//while ((hadc2.Instance->SR & 0x02) == 0);
		return HAL_ADC_GetValue(&hadc2);
	}
}

void pwmWrite(unsigned int pin, unsigned int value) {
	if (pin == PA7) {
		Timer3.pause();
		Timer3.Tim->setPWM(2, PA7, 1000, value / 357);
	}
	if (pin == PB9) {
		Timer4.pause();
		Timer4.Tim->setPWM(4, PB9, 20000, value / 36);
	}
	if (pin == PB8) {
		Timer4.pause();
		Timer4.Tim->setPWM(3, PB8, 20000, value / 36);
	}
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
