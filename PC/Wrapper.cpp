/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   PC_Wrapper.h
 * Author: gamboa
 *
 * Created on 8 de Setembro de 2020, 15:20
 */

#include "Wrapper.h"
#include <unistd.h>
#include <sys/time.h>
#include "MicroDSO1.h"
#include "src/MicroDSO/hal.h"
#include "src/MicroDSO/variables.h"
#include "src/MicroDSO/display.h"
#include "src/MicroDSO/io.h"

//Control pins |RD |WR |RS |CS |RST|
#define TFT_RD         PB10
#define TFT_WR         PC15
#define TFT_RS         PC14
#define TFT_CS         PC13
#define TFT_RST        PB11


unsigned short adc[ADCMAX] = {0, 0, 0, 0, 0, 0};
static unsigned int adc_chn = 0;

void
Print::print (const char* str)
{
  for (unsigned int i = 0; i < strlen (str); i++)
    {
      write (str[i]);
    }
}

void
Print::print (const int i)
{
  char buff[100];
  sprintf (buff, "%-5i", i);
  print (buff);
}

void
Print::print (const double d)
{
  char buff[100];
  sprintf (buff, "%-5.2f", d);
  print (buff);
}


#define FLASH_LATENCY_2 2

void
__HAL_FLASH_PREFETCH_BUFFER_ENABLE (void) { };

void
__HAL_FLASH_SET_LATENCY (int latency) { };

unsigned int
millis (void)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);

  return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

unsigned int
micros (void)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  return ((tv.tv_sec % 600)*1000000) +tv.tv_usec;
}

void
delay (unsigned int t)
{
  Window1.DrawLCD ();
  for (unsigned int i = 0; i < t / 100; i++)
    {
      Application->ProcessEvents ();
      usleep (100000);
    }
}

void
delayMicroseconds (unsigned int t)
{
  Window1.DrawLCD ();
  Application->ProcessEvents ();
  usleep (t);
}

PORT_t PORTB;

typedef struct
{
  unsigned char value;
  unsigned char dir;
  unsigned char int_type;
  void (*intfcn)(void);
} pin_t;

#define PINMAX 48
static pin_t pins[PINMAX + 1];

void
pinMode (int pin, int dir)
{
  if (pin <= PINMAX)
    {
      pins[pin].dir = dir;
    }
}

void
digitalWrite (int pin, int value)
{
  if (pin <= PINMAX)
    {
      pins[pin].value = value;

      //printf("pin[%i]=%i\n",pin,value);

      switch (pin)
        {
        case TFT_RD:
        case TFT_CS:
        case TFT_RST:
        case TFT_WR:
        case TFT_RS:
          lcd_ili9341_8_io (&Window1.lcd, PORTB.BSRR & 0xFFFF, pins[TFT_WR].value,
                            pins[TFT_RD].value, pins[TFT_CS].value,
                            pins[TFT_RST].value, pins[TFT_RS].value);
          break;
        }
    }
}

void
ExternPinWrite (int pin, int value)
{
  if (pin <= PINMAX)
    {
      switch (pins[pin].int_type)
        {
        case FALLING:
          if ((pins[pin].value == 1) &&(value == 0))
            {
              pins[pin].value = value;
              pins[pin].intfcn ();
            }
          break;
        case RISING:
          if ((pins[pin].value == 0) &&(value == 1))
            {
              pins[pin].value = value;
              pins[pin].intfcn ();
            }
          break;
        case CHANGE:
          if (pins[pin].value != value)
            {
              pins[pin].value = value;
              pins[pin].intfcn ();
            }
          break;
        }
      pins[pin].value = value;
    }
}

void
ExternPinInit (int pin, int value)
{
  if (pin <= PINMAX)
    {
      pins[pin].value = value;
    }
}

unsigned char
digitalRead (int pin)
{
  if (pin <= PINMAX)
    {
      return pins[pin].value;
    }
  return 0;
}

ADC_TypeDef * ADC1;
ADC_TypeDef * ADC2;

void
serial::begin (int) { }

void
serial::print (const char * str)
{
  printf ("%s",str);
}

void
serial::print (int i)
{
  printf ("%i", i);
}

void
serial::print (long int l)
{
  printf ("%li", l);
}

void
serial::print (double d)
{
  printf ("%f", d);
}

void
serial::println (const char * str)
{
  printf ("%s\n", str);
}

void
serial::println (int i)
{
  printf ("%i\n", i);
}

void
serial::println (long int l)
{
  printf ("%li\n", l);
}

void
serial::println (double d)
{
  printf ("%f\n", d);
}

void
serial::println (void)
{
  printf ("\n");
}

serial Serial;

void
attachInterrupt (int pin, void (*fn)(void), int type)
{
  if (pin <= PINMAX)
    {
      pins[pin].int_type = type;
      pins[pin].intfcn = fn;
    }
}

void
detachInterrupt (int pin)
{
  if (pin <= PINMAX)
    {
      pins[pin].int_type = NONE;
      pins[pin].intfcn = NULL;
    }
}


extern int16_t sDly, tDly;
extern bool minSamplesAcquired;
extern bool triggerRising;
extern long prevTime;

// hold pointer references for updating variables in memory
extern uint16_t *sIndexPtr;
extern volatile bool *keepSamplingPtr;
extern volatile bool *triggeredPtr;

int xZoom = 1;


// ------------------------

static float t = 0;

inline void
snapMicros (void)
{
  // ------------------------
  samplingTime = t * 1e6 - prevTime;
  prevTime = t * 1e6;
  minSamplesAcquired = true;
}

unsigned short
getval (float gain, unsigned int sampletime)
{
  float freq = 1000;
  float value;
  //ch1Capture[sIndex] = 248.0 * sin (2 * M_PI * freq * t) + zeroVoltageA1+ (10.0 * rand () / RAND_MAX);

  switch (couplingPos)
    {
    case CPL_GND:
      ch1Capture[sIndex] = zeroVoltageA1 + (10.0 * rand () / RAND_MAX);
      break;
    case CPL_AC:
      value = (sin (2 * M_PI * freq * t) > 0) ? 1.66 / gain : -1.66 / gain;
      ch1Capture[sIndex] = (2048 / 22) * value + zeroVoltageA1 + (10.0 * rand () / RAND_MAX);
      break;
    case CPL_DC:
      value = (sin (2 * M_PI * freq * t) > 0) ? 3.3 / gain : 0;
      ch1Capture[sIndex] = (2048 / 22) * value + zeroVoltageA1 + (10.0 * rand () / RAND_MAX);
      break;
    }

  ch2Capture[sIndex] = 248.0 * cos (2 * M_PI * freq * t) + zeroVoltageA2 + (10.0 * rand () / RAND_MAX);

  if (ch1Capture[sIndex] > zeroVoltageA1 + 100)
    bitStore[sIndex] |= 0x2000;
  else
    bitStore[sIndex] &= ~0x2000;

  if (ch2Capture[sIndex] > zeroVoltageA2)
    bitStore[sIndex] |= 0x4000;
  else
    bitStore[sIndex] &= ~0x4000;

  if (samplingTime <= 0.0)
    {
      samplingTime = 20;
    }
  
  t += (1e-6 * samplingTime) / 24.0;
  
  if(t > 16.0) t-=16.0;

  //timeout
  if ((t * 1e6) > (tDly * 1000) + prevTime)
    {
      keepSampling = false;
    }

  ExternPinWrite (TRIGGER_IN, ch1Capture[sIndex] > (getTriggerLevel () + zeroVoltageA1));

  return ch1Capture[sIndex];
}

void
wrapper_sampling (int16_t lDelay)
{

  float gain = 1.0;

  switch (currentTimeBase)
    {
    case T20US:
      samplingTime = 20;
      break;
    case T30US:
      samplingTime = 30;
      break;
    case T50US:
      samplingTime = 50;
      break;
    case T0_1MS:
      samplingTime = 100;
      break;
    case T0_2MS:
      samplingTime = 200;
      break;
    case T0_5MS:
      samplingTime = 500;
      break;
    case T1MS:
      samplingTime = 1000;
      break;
    case T2MS:
      samplingTime = 2000;
      break;
    case T5MS:
      samplingTime = 5000;
      break;
    case T10MS:
      samplingTime = 10000;
      break;
    case T20MS:
      samplingTime = 20000;
      break;
    case T50MS:
      samplingTime = 50000;
      break;
    }


  switch (rangePos)
    {
    case RNG_5V:
      gain = 5;
      break;
    case RNG_2V:
      gain = 2;
      break;
    case RNG_1V:
      gain = 1;
      break;
    case RNG_0_5V:
      gain = 0.5;
      break;
    case RNG_0_2V:
      gain = 0.2;
      break;
    case RNG_0_1V:
      gain = 0.1;
      break;
    case RNG_50mV:
      gain = 0.05;
      break;
    case RNG_20mV:
      gain = 0.02;
      break;
    case RNG_10mV:
      gain = 0.01;
      break;
    }


  uint16_t lCtr = 0;

  prevTime = t * 1e6;

  while (keepSampling)
    {
      ch1Capture[sIndex] = getval (gain, samplingTime); //hadc1.Instance->DR | (GPIOB->IDR & 0xE000);
      sIndex++;
      if (sIndex == NUM_SAMPLES)
        {
          sIndex = 0;
          snapMicros ();
        }
      if (triggered)
        {
          lCtr++;
          if (lCtr == (NUM_SAMPLES / 2)) //Why do we only sample half the numbers of samples?
            return;
        }

      //lDelay 
    }

}

void
adc_set_sample_rate (int*, unsigned int) { }

void
adc_set_reg_seqlen (int*, unsigned int) { }

void
adc_set_channel (int* adc, unsigned int chn)
{
  if (chn < ADCMAX)
    {
      adc_chn = chn;
    }
}

void
adc_set_mode (int*, unsigned int, unsigned int) { }

void
adc_calibrate (int*) { }

unsigned int
adc_read (ADC_TypeDef * adcv)
{
  return adc[adc_chn];
}

timer Timer2 (NULL);
timer Timer3 (NULL);
timer Timer4 (NULL);

void
pwmWrite (unsigned int, unsigned int) { }

timer::timer (TIM_TypeDef*) { }

timer::~timer () { }

void
timer::setPeriod (unsigned int) { }

void
timer::setChannel1Mode (int) { }

void
timer::pause (void) { }

void
timer::setCompare1 (unsigned int) { }

void
timer::attachCompare1Interrupt (void (*fptr)()) { }

void
timer::setCount (unsigned int) { }

void
timer::resume (void) { }

bool
EE_Format (void)
{
  FILE * fee;
  fee = fopen ("assets/eeprom.bin", "w");
  char buff[1024];
  if (fee)
    {
      memset (buff, 0xFF, 1024);
      fwrite (buff, 1024, 1, fee);
      fclose (fee);
      return true;
    }
  return false;
}

bool
EE_Writes (uint16_t VirtualAddress, uint16_t HowMuchToWrite, uint32_t* Data)
{
  FILE * fee;
  fee = fopen ("assets/eeprom.bin", "r+");
  if (fee)
    {
      fseek (fee, VirtualAddress, SEEK_SET);
      fwrite (Data, 4, HowMuchToWrite, fee);
      fclose (fee);
      return true;
    }
  return false;
}

bool
EE_Reads (uint16_t VirtualAddress, uint16_t HowMuchToRead, uint32_t* Data)
{
  FILE * fee;
  fee = fopen ("assets/eeprom.bin", "r");
  if (fee)
    {
      fseek (fee, VirtualAddress, SEEK_SET);
      fread (Data, 4, HowMuchToRead, fee);
      fclose (fee);
      return true;
    }
  return false;
}
