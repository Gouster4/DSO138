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

#ifndef PC_WRAPPER_H

#define PC_WRAPPER_H

#include<stdio.h>
#include<math.h>
#include<string.h>

//typedef char int8_t;
typedef short int16_t;
typedef int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned char byte;

#define __FlashStringHelper char*

typedef struct {
} HardwareTimer;

typedef struct {
} TIM_TypeDef;

typedef int ADC_TypeDef;

typedef struct {
} GPIO_TypeDef;

typedef int TimerModes_t;


class Print {
public:

    void print(const char* str);
    void print(const int i);
    void print(const double d);
    
    virtual size_t write(uint8_t) = 0;
};

#define FLASH_LATENCY_2 2
void __HAL_FLASH_PREFETCH_BUFFER_ENABLE(void);
void __HAL_FLASH_SET_LATENCY(int latency);


unsigned int micros(void);
unsigned int millis(void);
void delay(unsigned int t);
void delayMicroseconds(unsigned int t);

// pins  
#define PA15   15  //Board LED
#define PA7    7 // 1KHz square wave output
#define PA8    8 //TRIGGER_IN
#define PB8    (8+16) //TRIGGER_LEVEL
#define PB9    (9+16) //VGEN		// used to generate negative voltage in DSO138
#define PA0    0 //AN_CH1 			// analog channel 1
#define PA4    4 //AN_CH2 			// analog channel 2
#define PA13   13 //DG_CH1 			// digital channel 1 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define PA14   14 //DG_CH2 			// digital channel 2 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define PA2    2 //VSENSSEL1 	
#define PA1    1 //VSENSSEL2	
#define PA3    3 //CPLSEL		
#define PB12   (12+16) //ENCODER_SW	
#define PB13   (13+16) //ENCODER_A	
#define PB14   (14+16) //ENCODER_B	
#define PB15   (15+16) //BTN4 		
#define PB10   (10+16)//TFT_RD         
#define PC15   (15+32)//TFT_WR         
#define PC14   (14+32)//TFT_RS         
#define PC13   (13+32)//TFT_CS         
#define PB11   (11+16)//TFT_RST        

typedef struct{
unsigned int BSRR;
unsigned int CRL;
unsigned int CRH;
unsigned int IDR;
}PORT_t;

extern PORT_t PORTB;
//display PB0 to PB7 
#define GPIOB (&PORTB)

#define LOW 0
#define HIGH 1

#define INPUT 0
#define OUTPUT 1
#define INPUT_ANALOG 2
#define INPUT_PULLUP 3

void pinMode(int pin, int dir);
void digitalWrite(int pin, int value);
unsigned char digitalRead(int pin);

extern ADC_TypeDef * ADC1;
extern ADC_TypeDef * ADC2;

#define NONE 0
#define FALLING 1
#define RISING 2
#define CHANGE  3

#define ADC_CR2_SWSTART 1
#define ADC_CR2_CONT 2

#define TIMER_OUTPUT_COMPARE 3 

class serial
{
public:
    void begin(int ); 	
    void print(const char *); 
    void print(int); 
    void print(long int); 	
    void print(double); 	
    
    void println(const char *); 
    void println(int); 
    void println( long int); 	
    void println(double); 
    void println(void); 
};

extern serial Serial;



void attachInterrupt(int pin, void (*fn)(void), int type);
void detachInterrupt(int pin);


void     wrapper_sampling(int16_t lDelay); 

void ExternPinWrite(int pin, int value);
void ExternPinInit (int pin, int value);


#define ADCMAX 6
extern unsigned short adc[ADCMAX];

#endif /* PC_WRAPPER_H */

