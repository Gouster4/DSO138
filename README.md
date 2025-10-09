# MicroDSO

port of DLO-138 to compile in Arduino 1.8.10 with new STM32 support. Converted to C++ to be portable.

Use the [Sloeber plugin](https://eclipse.baeyens.it/) in eclipse to debug the code. More info in [stm32duino how to debug](https://github.com/stm32duino/wiki/wiki/How-to-debug)

To avoid unnecessary wear on the flash memory used as an emulated eeprom, the parameters are only saved now by the user's action.

In the previous normal mode whenever the + or - keys are pressed, the settings were saved. The settings are 20 bytes in size, the flash page 1024, so the configuration can be saved 51 times before the page can be erased. As each page can be erased 10000, we have a total of 510000 presses of the + and - keys before the flash loses its useful life.
Considering 100 clicks per use (probably more if the encoder is used), the flash memory of the configuration should last 5100 uses. Saving only when the user wishes, there is no concern with the duration of the flash. In the newer models DSO138 mini and DSO150 there is an external EEPROM memory to avoid this problem.


[Online demonstration](https://lcgamboa.github.io/MicroDSO/MicroDSO.html)

![Photo](https://github.com/lcgamboa/MicroDSO/blob/master/pics/demojs.png)

# DLO-138
An open source firmware for DSO-138 Oscilloscope. 
![Photo](https://github.com/ardyesp/DLO-138/blob/master/pics/pic4.png)

DSO-138 is an excellent piece of hardware based on ARM Cortex M3 core STM32F103 processor and sufficient for most beginner users. The stock firmware, while quite responsive, can use a few improvements. The main shortcoming which prompted the development of DLO-138 firmware is the inability to get waveform data into a computer for further analysis and the lack of a second channel. Engineers troubleshooting hardware issues need to mark reference points on waveform so having another analog or digital channel can greatly improve analysis. This firmware hopes to improve on these issues.

## Features
- Two analog channels
- Two digital logic channels (SWDIO and SWDIO pins (PA13 and PA14) on board)
- Serial port interface for captured waveform data
- Trigger source selectable from Analog Channel 1 or Digital Channel
- Option to use rotary encoder instead of + - and SEL switches
- 2K sample depth

This firmware can be used on stock DSO-138 hardware as well. Select one of the pre-compiled binaries to suit the board. Follow the firmware upgrade instructions for DSO-138. At any time, you can reflash DSO-138 with JYE Tech provided firmware.

# Cost
Extra features come at an additional cost. In the case of DLO-138, it is the loss of lowest timebase. Maximum sampling rate in DLO-138 is 20 µs/div instead of 10 µs/div. In the 20 µs/div range, firmware under-samples ADC channels, often reading same data twice. To use the second analog channel, analog front end has to be duplicated on a daughter board. On a stock hardware, this firmware can be used to provide two digital logic channels.

# Build

[Use ArduinoIDE](https://www.arduino.cc/en/software/) (for exammple 2.3.6)

In Preferences add this url to Aditional boards manager URLs: [https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json](https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json)

Install "[DEPRECATED - Please use new package index] STM32 Cores" version 1.9.0 using Board Manager

Select Board: [DEPRECATED - Please use new package index] STM32 Cores > "Generic STM32F1 Series"

Select Board part number: "Generic F103C8"

Select U(S)ART support: "Enabled (generic Serial)"

Rest of settings are optional:

   -USB Support: "None"
	
   -Optimalize: "Smallest (-Os)"
	
   -C Runtime Library: "Newlib Nano"
	
# Hardware
Following changes can be applied selectively, to get maximum functionality from board. The firmware can be run on unmodified hardware as well.
![Mod Schematic](https://github.com/ardyesp/DLO-138/blob/master/pics/HardwareMod.png)

# Usage:
	Push button in encoder (SEL if using switches) moves focus to next parameter
	Left/Right turn in encoder (+/- if using switches) changes the parameter which is in focus
	Short press OK to HOLD the waveform and output it on serial port
	Long press OK button:
	
		Focus				Action
		Buffer Size		Save configuration to EEPROM (FLASH emulated)
		Trigger Level		Zero the trigger level to Analog channel 1
		Wave X scrollbar	Center waveform on screen (at trigger point)
		Wave Y cursor		Zero the cursor. If Analog CH1 coupling is GND, waveform reference base is set 
		Other				Toggle on screen Analog CH1 statistics display

	Press and hold OK button at power up to reset settings to default
	
# Flash binaries directly via serial interface

When using Windows you can follow the guide from jyetech:
https://jyetech.com/wp-content/uploads/2018/07/dso138-firmware-upgrade.pdf

The guide uses the graphical programming tool provided by ST:
https://www.st.com/en/development-tools/flasher-stm32.html

---

When using Linux, you can use the open source command line tool stm32flash:

Install stm32flash:
```
sudo apt-get install stm32flash
```

Connect your TTL-UART-to-USB converter to the DSO138 and bridge jumpers J1 and J2 on the back of the PCB just like in the above manual.

Or solder 10 kΩ resistor across JP2, and button on JP1 to have programming button which needs to be pressed while connecting DSO138 to power for programming mode.

Unlock the flash of the STM32:
```
sudo stm32flash /dev/ttyUSB0 -k -b 115200

sudo stm32flash /dev/ttyUSB0 -u -b 115200
```

Flash new firmware:
```
sudo stm32flash /dev/ttyUSB0 -w binaries/DLO-138_switches_1.0.bin -b 115200
```

Remove the solder bridges on J1 and J2 and enjoy the alternative firmware on your DSO138.
 		

# TODO:
Add Spectrum Analyzer mode

Add support to PC protocols over UART (oscilloscope software on PC)

# References
DSO-138 - http://www.jyetech.com/Products/LcdScope/e138.php

STM32Duino - http://www.stm32duino.com

STM32F103 - http://www.st.com/en/microcontrollers/stm32f103.html

Adafruint Graphics Library - https://github.com/adafruit/Adafruit-GFX-Library

Parallel 8 bit ILI9341 library - https://github.com/stevstrong/Adafruit_TFTLCD_8bit_STM32


