/**
 * \file
 * \brief Real Time Operating System (RTOS) main program.
 * \author Erich Styger, erich.styger@hslu.ch
 */

#include "Platform.h"
#if PL_CONFIG_HAS_RTOS
#include "RTOS.h"
#include "FRTOS1.h"
#include "LED.h"
#include "Event.h"
#include "Keys.h"
#include "Application.h"

static void AppTask(void* param) {
  const int *whichLED = (int*)param;

  //(void)param; /* avoid compiler warning */
  for(;;) {
    if (*whichLED==1) {
      LED1_Neg();
    } else if (*whichLED==2) {
      LED2_Neg();
    }
#if PL_CONFIG_HAS_KEYS
    KEY_Scan();
#endif
#if PL_CONFIG_HAS_EVENTS
    EVNT_HandleEvent(APP_EventHandler, TRUE);
#endif
    //FRTOS1_vTaskDelay(500/portTICK_PERIOD_MS);
    FRTOS1_vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void KillMe(void* param) {
  LED1_On(); /* LED of death! */
  vTaskDelete(NULL); /* suicide! */
}

void RTOS_Init(void) {
  static const int led1 = 1;
  static const int led2 = 2;

  EVNT_SetEvent(EVNT_STARTUP); /* set startup event */
  /*! \todo Create tasks here */
  if (FRTOS1_xTaskCreate(AppTask, (signed portCHAR *)"App1", configMINIMAL_STACK_SIZE, (void*)&led1, tskIDLE_PRIORITY, NULL) != pdPASS) {
    for(;;){} /* error case only, stay here! */
  }
#if 0
  if (FRTOS1_xTaskCreate(AppTask, (signed portCHAR *)"App2", configMINIMAL_STACK_SIZE, (void*)&led2, tskIDLE_PRIORITY, NULL) != pdPASS) {
    for(;;){} /* error case only, stay here! */
  }
  if (FRTOS1_xTaskCreate(KillMe, (signed portCHAR *)"KillMe", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL) != pdPASS) {
    for(;;){} /* error case only, stay here! */
  }
#endif
}

void RTOS_Deinit(void) {
  /* nothing needed for now */
}

#endif /* PL_CONFIG_HAS_RTOS */
