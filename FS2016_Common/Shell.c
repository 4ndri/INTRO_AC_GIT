/**
 * \file
 * \brief Shell and console interface implementation.
 * \author Erich Styger
 *
 * This module implements the front to the console/shell functionality.
 */

#include "Platform.h"
#if PL_CONFIG_HAS_SHELL
#include "Shell.h"
#include "CLS1.h"
#include "Application.h"
#include "FRTOS1.h"
#if PL_CONFIG_HAS_BLUETOOTH
  #include "BT1.h"
#endif
#if PL_CONFIG_HAS_BUZZER
  #include "Buzzer.h"
#endif
#if PL_CONFIG_HAS_SHELL_QUEUE
  #include "ShellQueue.h"
#endif
#if PL_CONFIG_HAS_REFLECTANCE
  #include "Reflectance.h"
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  #include "RTT1.h"
#endif
#if PL_CONFIG_HAS_MOTOR
  #include "Motor.h"
#endif
#if PL_CONFIG_HAS_MCP4728
  #include "MCP4728.h"
#endif
#if PL_CONFIG_HAS_QUADRATURE
  #include "Q4CLeft.h"
  #include "Q4CRight.h"
#endif
#if PL_CONFIG_HAS_QUAD_CALIBRATION
  #include "QuadCalib.h"
#endif
#if PL_CONFIG_HAS_MOTOR_TACHO
  #include "Tacho.h"
#endif
#if PL_CONFIG_HAS_ULTRASONIC
  #include "Ultrasonic.h"
#endif
#if PL_CONFIG_HAS_PID
  #include "PID.h"
#endif
#if PL_CONFIG_HAS_DRIVE
  #include "Drive.h"
#endif
#if PL_CONFIG_HAS_TURN
  #include "Turn.h"
#endif
#if PL_CONFIG_HAS_LINE_FOLLOW
  #include "LineFollow.h"
#endif
#if PL_CONFIG_HAS_RADIO
  #include "RApp.h"
  #include "RNet_App.h"
  #include "RNetConf.h"
#endif
#if RNET_CONFIG_REMOTE_STDIO
  #include "RStdIO.h"
#endif
#if PL_CONFIG_HAS_REMOTE
  #include "Remote.h"
#endif
#if PL_CONFIG_HAS_LINE_MAZE
  #include "Maze.h"
#endif
#include "KIN1.h"

/* forward declaration */
static uint8_t SHELL_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);

static const CLS1_ParseCommandCallback CmdParserTable[] =
{
  CLS1_ParseCommand, /* Processor Expert Shell component, is first in list */
  SHELL_ParseCommand, /* our own module parser */
#if FRTOS1_PARSE_COMMAND_ENABLED
  FRTOS1_ParseCommand, /* FreeRTOS shell parser */
#endif
//#if PL_CONFIG_HAS_BLUETOOTH
#if defined(BT1_PARSE_COMMAND_ENABLED) && BT1_PARSE_COMMAND_ENABLED
  BT1_ParseCommand,
#endif
//#endif
#if PL_CONFIG_HAS_BUZZER
  BUZ_ParseCommand,
#endif
#if PL_CONFIG_HAS_REFLECTANCE
  #if REF_PARSE_COMMAND_ENABLED
  REF_ParseCommand,
  #endif
#endif
#if PL_CONFIG_HAS_MOTOR
  MOT_ParseCommand,
#endif
#if PL_CONFIG_HAS_MCP4728
   MCP4728_ParseCommand,
#endif
#if PL_CONFIG_HAS_QUADRATURE
  Q4CLeft_ParseCommand,
  Q4CRight_ParseCommand,
#endif
#if PL_CONFIG_HAS_QUAD_CALIBRATION
   QUADCALIB_ParseCommand,
#endif
#if PL_CONFIG_HAS_MOTOR_TACHO
  TACHO_ParseCommand,
#endif
#if PL_CONFIG_HAS_ULTRASONIC
  US_ParseCommand,
#endif
#if PL_CONFIG_HAS_PID
  PID_ParseCommand,
#endif
#if KIN1_PARSE_COMMAND_ENABLED
  KIN1_ParseCommand,
#endif
#if PL_CONFIG_HAS_DRIVE
  DRV_ParseCommand,
#endif
#if PL_CONFIG_HAS_TURN
  TURN_ParseCommand,
#endif
#if PL_CONFIG_HAS_LINE_FOLLOW
  LF_ParseCommand,
#endif
#if PL_CONFIG_HAS_RADIO
#if RNET1_PARSE_COMMAND_ENABLED
  RNET1_ParseCommand,
#endif
  RNETA_ParseCommand,
#endif
#if PL_CONFIG_HAS_REMOTE
  REMOTE_ParseCommand,
#endif
#if PL_CONFIG_HAS_LINE_MAZE
  MAZE_ParseCommand,
#endif
  NULL /* Sentinel */
};

static uint32_t SHELL_val; /* used as demo value for shell */

void SHELL_SendString(unsigned char *msg) {
#if PL_CONFIG_HAS_SHELL_QUEUE
  SQUEUE_SendString(msg);
#elif CLS1_DEFAULT_SERIAL
  CLS1_SendStr(msg, CLS1_GetStdio()->stdOut);
#else
#endif
}

/*!
 * \brief Prints the help text to the console
 * \param io StdIO handler
 * \return ERR_OK or failure code
 */
static uint8_t SHELL_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr("Shell", "Shell commands\r\n", io->stdOut);
  CLS1_SendHelpStr("  help|status", "Print help or status information\r\n", io->stdOut);
  CLS1_SendHelpStr("  val <num>", "Assign number value\r\n", io->stdOut);
  return ERR_OK;
}

/*!
 * \brief Prints the status text to the console
 * \param io StdIO handler
 * \return ERR_OK or failure code
 */
static uint8_t SHELL_PrintStatus(const CLS1_StdIOType *io) {
  uint8_t buf[16];

  CLS1_SendStatusStr("Shell", "\r\n", io->stdOut);
  UTIL1_Num32sToStr(buf, sizeof(buf), SHELL_val);
  UTIL1_strcat(buf, sizeof(buf), "\r\n");
  CLS1_SendStatusStr("  val", buf, io->stdOut);
  return ERR_OK;
}

static uint8_t SHELL_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint32_t val;
  const unsigned char *p;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "Shell help")==0) {
    *handled = TRUE;
    return SHELL_PrintHelp(io);
  } else if (UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0 || UTIL1_strcmp((char*)cmd, "Shell status")==0) {
    *handled = TRUE;
    return SHELL_PrintStatus(io);
  } else if (UTIL1_strncmp(cmd, "Shell val ", sizeof("Shell val ")-1)==0) {
    p = cmd+sizeof("Shell val ")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK) {
      SHELL_val = val;
      *handled = TRUE;
    } else {
      return ERR_FAILED; /* wrong format of command? */
    }
  }
  return ERR_OK;
}

#if PL_CONFIG_HAS_BLUETOOTH
/* Bluetooth stdio */
static CLS1_ConstStdIOType BT_stdio = {
  (CLS1_StdIO_In_FctType)BT1_StdIOReadChar, /* stdin */
  (CLS1_StdIO_OutErr_FctType)BT1_StdIOSendChar, /* stdout */
  (CLS1_StdIO_OutErr_FctType)BT1_StdIOSendChar, /* stderr */
  BT1_StdIOKeyPressed /* if input is not empty */
};
#endif

#if PL_CONFIG_HAS_SEGGER_RTT
static CLS1_ConstStdIOType RTT_stdio = {
  (CLS1_StdIO_In_FctType)RTT1_StdIOReadChar, /* stdin */
  (CLS1_StdIO_OutErr_FctType)RTT1_StdIOSendChar, /* stdout */
  (CLS1_StdIO_OutErr_FctType)RTT1_StdIOSendChar, /* stderr */
  RTT1_StdIOKeyPressed /* if input is not empty */
};
#endif

#define SHELL_COMBINED_STDIO  0

#if SHELL_COMBINED_STDIO /* copy */
static void CombineStdOut(uint8_t ch) {
#if CLS1_DEFAULT_SERIAL
  CLS1_GetStdio()->stdOut(ch);
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  RTT_stdio.stdOut(ch);
#endif
#if PL_CONFIG_HAS_BLUETOOTH
  BT_stdio.stdOut(ch);
#endif
}

static void CombineStdErr(uint8_t ch) {
#if CLS1_DEFAULT_SERIAL
  CLS1_GetStdio()->stdErr(ch);
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  RTT_stdio.stdErr(ch);
#endif
#if PL_CONFIG_HAS_BLUETOOTH
  BT_stdio.stdErr(ch);
#endif
}

static void CombineStdIn(uint8_t *ch) {
#if CLS1_DEFAULT_SERIAL
  CLS1_GetStdio()->stdIn(ch);
  if (*ch!='\0') {
    return;
  }
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  RTT_stdio.stdIn(ch);
  if (*ch!='\0') {
    return;
  }
#endif
#if PL_CONFIG_HAS_BLUETOOTH
  BT_stdio.stdIn(ch);
  if (*ch!='\0') {
    return;
  }
#endif
  *ch = '\0'; /* nothing received */
}

static bool CombineKeyPressed(void) {
#if CLS1_DEFAULT_SERIAL
  if (CLS1_GetStdio()->keyPressed()) {
    return TRUE;
  }
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  if (RTT_stdio.keyPressed()) {
    return TRUE;
  }
#endif
#if PL_CONFIG_HAS_BLUETOOTH
  if (BT_stdio.keyPressed()) {
    return TRUE;
  }
#endif
  return FALSE;
}

static CLS1_ConstStdIOType CombineStdio = {
  (CLS1_StdIO_In_FctType)CombineStdIn, /* stdin */
  (CLS1_StdIO_OutErr_FctType)CombineStdOut, /* stdout */
  (CLS1_StdIO_OutErr_FctType)CombineStdErr, /* stderr */
  CombineKeyPressed /* if input is not empty */
};
#endif

void SHELL_ParseCmd(unsigned char *cmd) {
#if SHELL_COMBINED_STDIO
  (void)CLS1_ParseWithCommandTable(cmd, &CombineStdio, CmdParserTable);
#else
  #if CLS1_DEFAULT_SERIAL
    (void)CLS1_ParseWithCommandTable(cmd, CLS1_GetStdio(), CmdParserTable);
  #endif
  #if PL_CONFIG_HAS_BLUETOOTH
    (void)CLS1_ParseWithCommandTable(cmd, &BT_stdio, CmdParserTable);
  #endif
  #if PL_CONFIG_HAS_SEGGER_RTT
    (void)CLS1_ParseWithCommandTable(cmd, &RTT_stdio, CmdParserTable);
  #endif
#endif
}

static void ShellTask(void *pvParameters) {
#define DEFAULT_BUF_SIZE 48
#if SHELL_COMBINED_STDIO
  static unsigned char combined_buf[DEFAULT_BUF_SIZE];
#else
#if CLS1_DEFAULT_SERIAL
  static unsigned char localConsole_buf[DEFAULT_BUF_SIZE];
#endif
#if PL_CONFIG_HAS_BLUETOOTH
  static unsigned char bluetooth_buf[DEFAULT_BUF_SIZE];
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  static unsigned char rtt_buf[DEFAULT_BUF_SIZE];
#endif
#if CLS1_DEFAULT_SERIAL
  CLS1_ConstStdIOTypePtr ioLocal = CLS1_GetStdio();  
#endif
#if PL_CONFIG_HAS_RADIO && RNET_CONFIG_REMOTE_STDIO
  static unsigned char radio_cmd_buf[48];
  CLS1_ConstStdIOType *ioRemote = RSTDIO_GetStdioRx();
#endif
#endif

  (void)pvParameters; /* not used */
#if SHELL_COMBINED_STDIO
  combined_buf[0] = '\0';
#else
#if PL_CONFIG_HAS_BLUETOOTH
  bluetooth_buf[0] = '\0';
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
  rtt_buf[0] = '\0';
#endif
#if CLS1_DEFAULT_SERIAL
  localConsole_buf[0] = '\0';
#endif
#if PL_CONFIG_HAS_RADIO && RNET_CONFIG_REMOTE_STDIO
  radio_cmd_buf[0] = '\0';
#endif
#endif
#if SHELL_COMBINED_STDIO
  (void)CLS1_ParseWithCommandTable((unsigned char*)CLS1_CMD_HELP, &CombineStdio, CmdParserTable);
#else
#if CLS1_DEFAULT_SERIAL
  (void)CLS1_ParseWithCommandTable((unsigned char*)CLS1_CMD_HELP, ioLocal, CmdParserTable);
#endif
#endif
  for(;;) {
#if SHELL_COMBINED_STDIO
    (void)CLS1_ReadAndParseWithCommandTable(combined_buf, sizeof(combined_buf), &CombineStdio, CmdParserTable);
#else
#if CLS1_DEFAULT_SERIAL
    (void)CLS1_ReadAndParseWithCommandTable(localConsole_buf, sizeof(localConsole_buf), ioLocal, CmdParserTable);
#endif
#if PL_CONFIG_HAS_BLUETOOTH
    (void)CLS1_ReadAndParseWithCommandTable(bluetooth_buf, sizeof(bluetooth_buf), &BT_stdio, CmdParserTable);
#endif
#if PL_CONFIG_HAS_SEGGER_RTT
    (void)CLS1_ReadAndParseWithCommandTable(rtt_buf, sizeof(rtt_buf), &RTT_stdio, CmdParserTable);
#endif
#if PL_CONFIG_HAS_RADIO && RNET_CONFIG_REMOTE_STDIO
    RSTDIO_Print(ioLocal); /* dispatch incoming messages */
    (void)CLS1_ReadAndParseWithCommandTable(radio_cmd_buf, sizeof(radio_cmd_buf), ioRemote, CmdParserTable);
#endif
#endif

#if PL_CONFIG_HAS_SHELL_QUEUE
#if PL_CONFIG_SQUEUE_SINGLE_CHAR
    {
        /*! \todo Handle shell queue */
      unsigned char ch;

      while((ch=SQUEUE_ReceiveChar()) && ch!='\0') {
    #if CLS1_DEFAULT_SERIAL
       ioLocal->stdOut(ch);
    #endif
    #if PL_CONFIG_HAS_BLUETOOTH
        BT_stdio.stdOut(ch); /* copy on Bluetooth */
    #endif
    #if PL_CONFIG_HAS_SEGGER_RTT
        CDC_stdio.stdOut(ch); /* copy on USB CDC */
    #endif
      }
    }
#else /* PL_CONFIG_SQUEUE_SINGLE_CHAR */
    {
      const unsigned char *msg;

      msg = SQUEUE_ReceiveMessage();
      if (msg!=NULL) {
        CLS1_SendStr(msg, CLS1_GetStdio()->stdOut);
        FRTOS1_vPortFree((void*)msg);
      }
    }
#endif /* PL_CONFIG_SQUEUE_SINGLE_CHAR */
#endif /* PL_CONFIG_HAS_SHELL_QUEUE */
    FRTOS1_vTaskDelay(10/portTICK_PERIOD_MS);
  } /* for */
}

void SHELL_Init(void) {
  SHELL_val = 0;
#if !CLS1_DEFAULT_SERIAL && PL_CONFIG_CONFIG_HAS_BLUETOOTH
  (void)CLS1_SetStdio(&BT_stdio); /* use the Bluetooth stdio as default */
#endif
#if PL_CONFIG_HAS_RTOS
  if (FRTOS1_xTaskCreate(ShellTask, "Shell", configMINIMAL_STACK_SIZE+100, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
    for(;;){} /* error */
  }
#endif
}

void SHELL_Deinit(void) {
}

#endif /* PL_CONFIG_HAS_SHELL */
