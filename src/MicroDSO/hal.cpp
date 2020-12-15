#ifdef ARDUINO

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

#endif
