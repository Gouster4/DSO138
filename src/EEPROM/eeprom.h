#ifndef __EEPROM_H
#define __EEPROM_H

#include <stdbool.h>
#include "stm32f1xx_hal.h"

//#define   _EEPROM_F1_LOW_DESTINY
#define   _EEPROM_F1_MEDIUM_DESTINY
//#define   _EEPROM_F1_HIGH_DESTINY

#define     _EEPROM_USE_FLASH_PAGE              127

//Support individual Address write. This requires one Page of Ram for temporary storage...
//#define INDIVIDUAL_ADDR_WRITE

//################################################################################################################
bool    EE_LL_Format(uint16_t startpage, uint16_t pages);
bool    EE_LL_Read(uint16_t startpage,uint16_t addr,uint16_t size, uint8_t* Data);
bool    EE_LL_Write(uint16_t startpage,uint16_t addr,uint16_t size, uint8_t* Data);

bool	EE_Format();
bool 	EE_Read(uint16_t VirtualAddress, uint8_t* Data);
#ifdef INDIVIDUAL_ADDR_WRITE
bool 	EE_Write(uint16_t VirtualAddress, uint8_t Data);
#endif

bool	EE_Reads(uint16_t VirtualAddress,uint16_t HowMuchToRead,uint32_t* Data);
bool 	EE_Writes(uint16_t VirtualAddress,uint16_t HowMuchToWrite,uint32_t* Data);

//################################################################################################################


#define EEPROM_OK 1
#define EESIZE 20   //size in word
class EEPROM_
{
uint16_t eedata[EESIZE];
uint16_t eeaddr;  //in bytes
public:
bool init(void){
	bool ret;
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
	__HAL_FLASH_SET_LATENCY(FLASH_LATENCY_2);

	eeaddr=0;

	do{
	ret = EE_Reads(eeaddr, EESIZE/2 ,(uint32_t*) &eedata);
	eeaddr+=EESIZE*2;
	if(eeaddr > 1024)ret=1;
	}while((eedata[0] != 0xFFFF)&&(ret)&&(eeaddr < 1024));
	eeaddr-=EESIZE*2;

	if(ret)
	{
		if(eeaddr >0)
		{
		ret = EE_Reads((eeaddr-(EESIZE*2)), EESIZE/2 ,(uint32_t*) &eedata);
		}
    }
	else
	{
		 format();
	}

	return ret;
};
uint16_t read(uint16_t addr)
  {
	return eedata[addr];
  };
bool format(void)
  {
	eeaddr=0;
	return EE_Format();
  };
bool write(uint16_t addr, uint16_t data)
  {
	 	 eedata[addr]=data;
	 	 if((eeaddr+(EESIZE*2)) >= 1024)
	     {
	 		 format();
	 	 }
		 bool ret = EE_Writes(eeaddr,EESIZE/2 ,(uint32_t*) &eedata);
		 eeaddr+=EESIZE*2;
		 return ret;
  }
};


#endif
