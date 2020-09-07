
#ifndef ZCONFIG_H
#define ZCONFIG_H

#define PREAMBLE_VALUE	2859

void loadDefaults(void);
void loadConfig(bool reset);
void saveParameter(uint16_t param, uint16_t data, bool flash_write= false);

#endif
