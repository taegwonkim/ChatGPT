#pragma once
#include "app_config.h"
#include <stdbool.h>

bool AppFlash_LoadConfig(AppWifiConfig *cfg);
bool AppFlash_SaveConfig(const AppWifiConfig *cfg);

/* Board SPI primitives supplied by app_port.c or the product BSP. */
bool Board_FlashRead(uint32_t address, void *data, uint32_t length);
bool Board_FlashProgram(uint32_t address, const void *data, uint32_t length);
bool Board_FlashEraseSector(uint32_t address);
