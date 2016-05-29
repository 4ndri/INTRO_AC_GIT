/*
 * LED.c
 *
 *  Created on: 29.02.2016
 *      Author: Erich Styger
 */


#include "Platform.h"
#if PL_CONFIG_HAS_LEDS
#include "LED.h"

static void LED_Test(void) {
  LED1_On();
  LED1_Off();
  LED1_Neg();
  LED1_Put(!LED1_Get()); /* toggle */

  LED2_On();
  LED2_Off();
  LED2_Neg();
  LED2_Put(!LED2_Get()); /* toggle */

  LED3_On();
  LED3_Off();
  LED3_Neg();
  LED3_Put(!LED3_Get()); /* toggle */
}


void LED_Init(void) {
  LED1_Off();
  LED2_Off();
  LED3_Off();
#if PL_CONFIG_TEST_DRIVERS
  LED_Test();
#endif
}

void LED_Deinit(void) {
#if PL_CONFIG_NOF_LEDS>=1
  LED1_Off();
#endif
#if PL_CONFIG_NOF_LEDS>=2
  LED2_Off();
#endif
#if PL_CONFIG_NOF_LEDS>=3
  LED3_Off();
#endif
#if PL_CONFIG_NOF_LEDS>=4
  #error "only 3 LEDs supported"
#endif
}

#endif /* PL_CONFIG_HAS_LEDS */
