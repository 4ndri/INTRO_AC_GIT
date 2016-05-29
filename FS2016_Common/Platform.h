/*
 * Platform.h
 *
 *  Created on: 28.02.2016
 *      Author: Erich Styger
 */

#ifndef SOURCES_INTRO_COMMON_PLATFORM_H_
#define SOURCES_INTRO_COMMON_PLATFORM_H_

#include <stdint.h> /* types as uint8_t used in project */
#include <stdbool.h> /* for bool type */
#include "PE_Types.h" /* common Processor Expert types: bool, NULL, ... */
#include "Platform_Local.h" /* local project configuration, present in each project (FRDM, Robot) */
#include "Cpu.h" /* for PEcfg_RoboV2/PEcfg_RoboV1 */

/* check local platform configuration */
#if PL_LOCAL_CONFIG_BOARD_IS_ROBO
  #define PL_CONFIG_BOARD_IS_FRDM   (0)
  #define PL_CONFIG_BOARD_IS_ROBO   (1)
  #if defined(PEcfg_RoboV2)
    #define PL_CONFIG_BOARD_IS_ROBO_V1          (0)
    #define PL_CONFIG_BOARD_IS_ROBO_V2          (1)
  #else
    #define PL_CONFIG_BOARD_IS_ROBO_V1          (1)
    #define PL_CONFIG_BOARD_IS_ROBO_V2          (0)
  #endif
#elif PL_LOCAL_CONFIG_BOARD_IS_FRDM
  #define PL_CONFIG_BOARD_IS_FRDM   (1)
  #define PL_CONFIG_BOARD_IS_ROBO   (0)
#else
  #error "One board type has to be defined in Platform_Local.h!"
#endif

#define PL_CONFIG_TEST_DRIVERS  (1) /* if enabled, will perform driver tests */

/* configuration from local config */
#define PL_CONFIG_NOF_LEDS      PL_LOCAL_CONFIG_NOF_LEDS /* number of LEDs */
#define PL_CONFIG_NOF_KEYS      PL_LOCAL_CONFIG_NOF_KEYS /* number of keys */
#define PL_CONFIG_KEY_1_ISR     PL_LOCAL_CONFIG_KEY_1_ISR /* if key is using interrupt */
#define PL_CONFIG_KEY_2_ISR     PL_LOCAL_CONFIG_KEY_2_ISR /* if key is using interrupt */
#define PL_CONFIG_KEY_3_ISR     PL_LOCAL_CONFIG_KEY_3_ISR /* if key is using interrupt */
#define PL_CONFIG_KEY_4_ISR     PL_LOCAL_CONFIG_KEY_4_ISR /* if key is using interrupt */
#define PL_CONFIG_KEY_5_ISR     PL_LOCAL_CONFIG_KEY_5_ISR /* if key is using interrupt */
#define PL_CONFIG_KEY_6_ISR     PL_LOCAL_CONFIG_KEY_6_ISR /* if key is using interrupt */
#define PL_CONFIG_KEY_7_ISR     PL_LOCAL_CONFIG_KEY_7_ISR /* if key is using interrupt */
#define PL_CONFIG_HAS_KBI       (PL_CONFIG_KEY_1_ISR||PL_CONFIG_KEY_2_ISR||PL_CONFIG_KEY_3_ISR||PL_CONFIG_KEY_4_ISR||PL_CONFIG_KEY_5_ISR||PL_CONFIG_KEY_6_ISR||PL_CONFIG_KEY_7_ISR)

/* driver configuration */
#define PL_CONFIG_HAS_LEDS      (1 && !defined(PL_LOCAL_CONFIG_HAS_LEDS_DISABLED)) /* LED driver */
#define PL_CONFIG_HAS_EVENTS    (1) /* event driver */
#define PL_CONFIG_HAS_TIMER     (1) /* timer interrupts */
#define PL_CONFIG_HAS_KEYS      (1) /* support for keys */
#define PL_CONFIG_HAS_TRIGGER   (1) /* support for triggers */
#define PL_CONFIG_HAS_BUZZER    (1 && PL_CONFIG_BOARD_IS_ROBO) /* support for buzzer */
#define PL_CONFIG_HAS_DEBOUNCE  (1) /* support for debouncing */
#define PL_CONFIG_HAS_RTOS      (1) /* RTOS support */

#define PL_CONFIG_HAS_SHELL             (1) /* shell support disabled for now */
#define PL_CONFIG_HAS_BLUETOOTH         (0 && PL_CONFIG_BOARD_IS_ROBO)
#define PL_CONFIG_HAS_SEGGER_RTT        (0 && PL_CONFIG_HAS_SHELL) /* using RTT with shell */
#define PL_CONFIG_HAS_SHELL_QUEUE       (1 && PL_CONFIG_HAS_SHELL) /* enable shell queueing */
#define PL_CONFIG_SQUEUE_SINGLE_CHAR    (1 && PL_CONFIG_HAS_SHELL_QUEUE) /* using single character shell queue */
#define PL_CONFIG_HAS_SEMAPHORE         (1) /* semaphore tests */
#define PL_CONFIG_HAS_REFLECTANCE       (1 && PL_CONFIG_BOARD_IS_ROBO)
#define PL_CONFIG_HAS_MOTOR             (1 && PL_CONFIG_BOARD_IS_ROBO)
#define PL_CONFIG_HAS_QUADRATURE        (1 && PL_CONFIG_HAS_MOTOR)
#define PL_CONFIG_HAS_MOTOR_TACHO       (1 && PL_CONFIG_HAS_QUADRATURE)

#define PL_CONFIG_HAS_MCP4728           (0 && PL_CONFIG_BOARD_IS_ROBO_V1) /* only for V1 robot */
#define PL_CONFIG_HAS_QUAD_CALIBRATION  (0 && PL_CONFIG_HAS_MCP4728)
#define PL_CONFIG_HAS_PID               (1 && PL_CONFIG_HAS_QUADRATURE)
#define PL_CONFIG_HAS_DRIVE             (1 && PL_CONFIG_HAS_PID)
#define PL_CONFIG_HAS_LINE_FOLLOW       (1 && PL_CONFIG_HAS_DRIVE)
#define PL_CONFIG_HAS_RADIO             (1)
//#define RNET_CONFIG_REMOTE_STDIO        (0) /* temporary only, to be remove when RNET gets added */
#define PL_CONFIG_HAS_REMOTE            (1)
#define PL_CONFIG_CONTROL_SENDER        (1 && PL_CONFIG_BOARD_IS_FRDM)
#define PL_CONFIG_HAS_JOYSTICK          (1 && PL_CONFIG_BOARD_IS_FRDM)
#define PL_CONFIG_HAS_TURN              (1 && PL_CONFIG_HAS_QUADRATURE)
#define PL_CONFIG_HAS_LINE_MAZE         (1 && PL_CONFIG_HAS_LINE_FOLLOW)
#define PL_CONFIG_HAS_CONFIG_NVM        (1)

/* interface */
void PL_Init(void); /* driver initialization */
void PL_Deinit(void); /* driver deinitialization */

#endif /* SOURCES_INTRO_COMMON_PLATFORM_H_ */
