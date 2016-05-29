/**
 * \file
 * \brief Semaphore usage
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * Module using semaphores.
 */

/**
 * \file
 * \brief Semaphore usage
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * Module using semaphores.
 */

#include "Platform.h" /* interface to the platform */
#if PL_CONFIG_HAS_SEMAPHORE
#include "FRTOS1.h"
#include "Sem.h"
#include "LED.h"

#define USE_SEMAPHORES 0

#if USE_SEMAPHORES
static void vSlaveTask(void *pvParameters) {
  xSemaphoreHandle sem = (xSemaphoreHandle)pvParameters;

  if (sem==NULL) {
    for(;;) {
      /* should not be NULL? */
    }
  }
  for(;;) {
    /*! \todo Implement functionality */
    if (xSemaphoreTake(sem, portMAX_DELAY)==pdPASS) { /* block on semaphore */
      LED3_Neg();
    }
  }
}

static void vMasterTask(void *pvParameters) {
  /*! \todo Understand functionality */
  xSemaphoreHandle sem = NULL;

  (void)pvParameters; /* parameter not used */
  sem = xSemaphoreCreateBinary();
  if (sem==NULL) { /* semaphore creation failed */
    for(;;){} /* error */
  }
  vQueueAddToRegistry(sem, "IPC_Sem");
  /* create slave task */
  if (xTaskCreate(vSlaveTask, "Slave", configMINIMAL_STACK_SIZE, sem, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
    for(;;){} /* error */
  }
  for(;;) {
    (void)xSemaphoreGive(sem); /* give control to other task */
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}
#endif /* USE_SEMAPHORES */

void SEM_Deinit(void) {
}

/*! \brief Initializes module */
void SEM_Init(void) {
#if USE_SEMAPHORES
  if (xTaskCreate(vMasterTask, "Master", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
    for(;;){} /* error */
  }
#endif
}
#endif /* PL_CONFIG_HAS_SEMAPHORE */
