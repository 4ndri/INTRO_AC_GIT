/**
 * \file
 * \brief Non-Volatile memory configuration handling.
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * This module implements the infrastructure to store configuration data
 * into non-volatile FLASH memory on the microcontroller.
 */

#include "Platform.h"
#if PL_CONFIG_HAS_CONFIG_NVM
#include "NVM_Config.h"
#include "IFsh1.h"

static bool isErased(uint8_t *ptr, int nofBytes) {
  while (nofBytes>0) {
    if (*ptr!=0xFF) {
      return FALSE; /* byte not erased */
    }
    ptr++;
    nofBytes--;
  }
  return TRUE;
}

uint8_t NVMC_SaveReflectanceData(void *data, uint16_t dataSize) {
  if (dataSize>NVMC_REFLECTANCE_DATA_SIZE) {
    return ERR_OVERFLOW;
  }
  return IFsh1_SetBlockFlash(data, (IFsh1_TAddress)(NVMC_REFLECTANCE_DATA_START_ADDR), dataSize);  
}

void *NVMC_GetReflectanceData(void) {
  if (isErased((uint8_t*)NVMC_REFLECTANCE_DATA_START_ADDR, NVMC_REFLECTANCE_DATA_SIZE)) {
    return NULL;
  }
  return (void*)NVMC_REFLECTANCE_DATA_START_ADDR;
}

void NVMC_Init(void) {
  /* nothing needed */
}

void NVMC_Deinit(void) {
  /* nothing needed */
}

#endif /* PL_CONFIG_HAS_CONFIG_NVM */


