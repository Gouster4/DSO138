
#ifndef __EEPROM_H
#define FIRMWARE_VERSION  "2025-10"
#define PROJECT_URL "http://techax.sk/dso138/"
// coment out to disable debug
#define HWDEBUG 1

// comment out following line to use DSO push buttons instead of encoder
//#define USE_ENCODER

// serial print macros
#define DBG_INIT(...) 		{ Serial.begin(__VA_ARGS__); 	}
#define DBG_PRINT(...) 		{ Serial.print(__VA_ARGS__); 	}
#define DBG_PRINTLN(...) 	{ Serial.println(__VA_ARGS__); }

#define SERIAL_BAUD_RATE	115200

// analog and digital samples storage depth
#define NUM_SAMPLES       2048
#define NUM_SAMPLES_HALF  1024
#define NUM_SAMPLES_QUARTER 512
#define NUM_SAMPLES_EIGHTH  256
enum {BUF_FULL, BUF_HALF, BUF_QUARTER, BUF_EIGHTH};
enum {WAVES_A1A2, WAVES_A1, WAVES_A2, WAVES_XY, WAVES_NONE};
// display colours
#define AN_SIGNAL1 		ILI9341_BLUE
#define AN_SIGNAL2 		ILI9341_GREEN
#define AN_SIGNALX 		ILI9341_CYAN
#define DG_SIGNAL1 		ILI9341_MAGENTA
#define DG_SIGNAL2 		ILI9341_RED
// XY mode tail length
#define DEFAULT_TAIL_LENGTH 50 

// pin definitions (DSO138)
#define BOARD_LED 	PA15
#define TEST_WAVE_PIN 	PA7     // 1KHz square wave output
#define TRIGGER_IN	PA8
#define TRIGGER_LEVEL	PB8
#define VGEN		PB9		// used to generate negative voltage in DSO138

// captured inputs
#define AN_CH1 		PA0	// analog channel 1
#define AN_CH2 		PA4	// analog channel 2
#define AN_CH1_channel 	0	// analog channel 1
#define AN_CH2_channel	4	// analog channel 2
#define DG_CH1 		PA13	// digital channel 1 - 5V tolerant pin. Pin mask throughout code has to match digital pin
#define DG_CH2 		PA14	// digital channel 2 - 5V tolerant pin. Pin mask throughout code has to match digital pin

// misc analog inputs
#define VSENSSEL1 	PA2
#define VSENSSEL2	PA1
#define CPLSEL		PA3
#define VSENSSEL1_channel 	2
#define VSENSSEL2_channel	1
#define CPLSEL_channel		3

// switches
#define ENCODER_SW	PB12
#define ENCODER_A	PB13
#define ENCODER_B	PB14
#define BTN4 		PB15

// TFT pins are hard coded in Adafruit_TFTLCD_8bit_STM32.h file
// TFT_RD         PB10
// TFT_WR         PC15
// TFT_RS         PC14
// TFT_CS         PC13
// TFT_RST        PB11

#define LED_ON	digitalWrite(BOARD_LED, LOW)
#define LED_OFF	digitalWrite(BOARD_LED, HIGH)

//zoom

#define ZOOM_MIN 1       // 0.1x
#define ZOOM_MAX 100     // 10x
#define ZOOM_DEFAULT 10  // 1x

// number of pixels waveform moves left/right or up/down
#define XCURSOR_STEP	25
#define YCURSOR_STEP	5


#define BTN_DEBOUNCE_TIME	350

#define RGB_R(color16) (((color16) >> 11) & 0x1F)
#define RGB_G(color16) (((color16) >> 5) & 0x3F) 
#define RGB_B(color16) ((color16) & 0x1F)

// Convert 5-6-5 RGB to 8-8-8 (for calculations)
#define RGB16_TO_R8(color16) (RGB_R(color16) * 255 / 31)
#define RGB16_TO_G8(color16) (RGB_G(color16) * 255 / 63)
#define RGB16_TO_B8(color16) (RGB_B(color16) * 255 / 31)
// FLASH memory address defines
#define PARAM_PREAMBLE	0
#define PARAM_TIMEBASE	1
#define PARAM_TRIGTYPE	2
#define PARAM_TRIGDIR	3
#define PARAM_XCURSOR	4
#define PARAM_YCURSOR	5	// 5,6,7,8 - 4 params
#define PARAM_WAVES		9	// 9,10,11,12 - 4 params
#define PARAM_TLEVEL	13
#define PARAM_STATS		14
#define PARAM_ZERO1		15
#define PARAM_ZERO2		16
#define PARAM_BUFSIZE	17
#define PARAM_ZOOM		18
#define PARAM_OPERATION_MODE 	19
#define PARAM_TAILLENGTH 20
#define PARAM_XYLINES   21 
#else
#define EESIZE 32   //FLASH memory address lenght
#endif
