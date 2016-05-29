/*
 * LED.h
 *
 *  Created on: 29.02.2016
 *      Author: Erich Styger
 */

#ifndef SOURCES_INTRO_COMMON_MASTER_LED_H_
#define SOURCES_INTRO_COMMON_MASTER_LED_H_

#include "Platform.h"

#if PL_CONFIG_NOF_LEDS>=1
  #include "LEDPin1.h"
#endif
#if PL_CONFIG_NOF_LEDS>=2
  #include "LEDPin2.h"
#endif
#if PL_CONFIG_NOF_LEDS>=3
  #include "LEDPin3.h"
#endif

#if PL_CONFIG_NOF_LEDS>=1
  #define LED_On(nr)      LEDPin##nr##_ClrVal()
  #define LED_Off(nr)     LEDPin##nr##_SetVal()
  #define LED_Neg(nr)     LEDPin##nr##_NegVal()
  #define LED_Get(nr)     (!LEDPin##nr##_GetVal())
  #define LED_Put(nr,val) LEDPin##nr##_PutVal(!val)
  #define LED_Open(nr)    /* NYI */
  #define LED_Close(nr)   /* NYI */
#endif

#if PL_CONFIG_NOF_LEDS>=1
  #define LED1_On()       LED_On(1)
  #define LED1_Off()      LED_Off(1)
  #define LED1_Neg()      LED_Neg(1)
  #define LED1_Get()      LED_Get(1)
  #define LED1_Put(val)   LED_Put(1,val)
  #define LED1_Open()     LED_Open(1)
  #define LED1_Close()    LED_Close(1)
#else
  #define LED1_On()       /* empty */
  #define LED1_On()       /* empty */
  #define LED1_Neg()      /* empty */
  #define LED1_Get()      /* empty */
  #define LED1_Put(val)   FALSE
  #define LED1_Open()     /* NYI */
  #define LED1_Close()    /* NYI */
#endif

#if PL_CONFIG_NOF_LEDS>=2
  #define LED2_On()       LED_On(2)
  #define LED2_Off()      LED_Off(2)
  #define LED2_Neg()      LED_Neg(2)
  #define LED2_Get()      LED_Get(2)
  #define LED2_Put(val)   LED_Put(2,val)
  #define LED2_Open()     LED_Open(2)
  #define LED2_Close()    LED_Close(2)
#else
  #define LED2_On()       /* empty */
  #define LED2_On()       /* empty */
  #define LED2_Neg()      /* empty */
  #define LED2_Get()      /* empty */
  #define LED2_Put(val)   FALSE
  #define LED2_Open()     /* NYI */
  #define LED2_Close()    /* NYI */
#endif

#if PL_CONFIG_NOF_LEDS>=3
  #define LED3_On()       LED_On(3)
  #define LED3_Off()      LED_Off(3)
  #define LED3_Neg()      LED_Neg(3)
  #define LED3_Get()      LED_Get(3)
  #define LED3_Put(val)   LED_Put(3,val)
  #define LED3_Open()     LED_Open(3)
  #define LED3_Close()    LED_Close(3)
#else
  #define LED3_On()       /* empty */
  #define LED3_Off()      /* empty */
  #define LED3_Neg()      /* empty */
  #define LED3_Get()      /* empty */
  #define LED3_Put(val)   FALSE
  #define LED3_Open()     /* NYI */
  #define LED3_Close()    /* NYI */
#endif

void LED_Init(void); /* driver initialization */
void LED_Deinit(void); /* driver deinitialization */

#endif /* SOURCES_INTRO_COMMON_MASTER_LED_H_ */
