/*
 * Turn.c
 *
 *  Created on: Mar 11, 2013
 *      Author: Erich Styger
 */
#include "Platform.h"
#if PL_CONFIG_HAS_TURN
#include "Turn.h"
#include "WAIT1.h"
#include "Motor.h"
#include "UTIL1.h"
#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
  #include "Shell.h"
#endif
#include "Reflectance.h"
#if PL_CONFIG_HAS_QUADRATURE
  #include "Q4CLeft.h"
  #include "Q4CRight.h"
  #include "Pid.h"
#endif
#if PL_CONFIG_HAS_DRIVE
  #include "Drive.h"
#endif

#define TURN_STEPS_90         500
  /*!< number of steps for a 90 degree turn */
#define TURN_STEPS_LINE       150
  /*!< number of steps stepping over the line */
#define TURN_STEPS_POST_LINE  80
  /*!< number of steps after the line, before making a turn */
#define TURN_STEPS_90_TIMEOUT_MS        1000
#define TURN_STEPS_LINE_TIMEOUT_MS      200
#define TURN_STEPS_POST_LINE_TIMEOUT_MS 200
#define TURN_STEPS_STOP_TIMEOUT_MS      150

static int32_t TURN_Steps90 = TURN_STEPS_90;
static int32_t TURN_StepsLine = TURN_STEPS_LINE;
static int32_t TURN_StepsPostLine = TURN_STEPS_POST_LINE;

/*!
 * \brief Translate a turn kind into a string
 * \return Returns a descriptive string
 */
const unsigned char *TURN_TurnKindStr(TURN_Kind kind) {
  switch(kind) {
    case TURN_LEFT45:                 return (const unsigned char*)"LEFT45";
    case TURN_LEFT90:                 return (const unsigned char*)"LEFT90";
    case TURN_RIGHT45:                return (const unsigned char*)"RIGHT45";
    case TURN_RIGHT90:                return (const unsigned char*)"RIGHT90";
    case TURN_LEFT180:                return (const unsigned char*)"LEFT180";
    case TURN_RIGHT180:               return (const unsigned char*)"RIGHT180";
    case TURN_STRAIGHT:               return (const unsigned char*)"STRAIGHT";
    case TURN_STEP_LINE_FW:           return (const unsigned char*)"STEP_LINE_FW";
    case TURN_STEP_LINE_BW:           return (const unsigned char*)"STEP_LINE_BW";
    case TURN_STEP_POST_LINE_FW:      return (const unsigned char*)"STEP_POST_LINE_FW";
    case TURN_STEP_POST_LINE_BW:      return (const unsigned char*)"STEP_POST_LINE_BW";
    case TURN_STEP_LINE_FW_POST_LINE: return (const unsigned char*)"STEP_LINE_FW_POST_LINE";
    case TURN_STEP_LINE_BW_POST_LINE: return (const unsigned char*)"STEP_LINE_BW_POST_LINE";
    case TURN_STOP:                   return (const unsigned char*)"STOP";
    case TURN_FINISHED:               return (const unsigned char*)"FINISHED";
    default:                          return (const unsigned char*)"TURN_UNKNOWN!";
  }
}

void TURN_MoveToPos(int32_t targetLPos, int32_t targetRPos, bool wait, TURN_StopFct stopIt, int32_t timeoutMs) {
  (void)DRV_SetPos(targetLPos, targetRPos);
  (void)DRV_SetMode(DRV_MODE_POS);
  for(;;) { /* breaks */
    if (stopIt!=NULL) {
      if (stopIt()) { /* check stop condition */
        break;
      }
    }
    WAIT1_WaitOSms(1); /* wait some time, and give Drive module enough time to use new value */
    timeoutMs--;
    if (timeoutMs<=0) {
      break; /* timeout */
    }
    if (wait && DRV_HasTurned()) {
      break;
    }
    if (!wait) {
      break;
    }
  } /* for */
#if PL_CONFIG_HAS_SHELL
  if (timeoutMs<=0) {
    SHELL_SendString((unsigned char*)"MoveToPos Timeout.\r\n");
  }
#endif
}

static void StepsTurn(int32_t stepsL, int32_t stepsR, TURN_StopFct stopIt, int32_t timeOutMS) {
  int32_t currLPos, currRPos, targetLPos, targetRPos;
  /* stop before turn */
  int timeout = TURN_STEPS_STOP_TIMEOUT_MS;
  
  DRV_SetMode(DRV_MODE_STOP); /* stop it */
  WAIT1_WaitOSms(5);

  while (timeout>0 && !DRV_IsStopped()) {
    /* wait until stopped */
    timeout-=5;
    WAIT1_WaitOSms(5);
  }
#if PL_CONFIG_HAS_SHELL
  if (timeout<=0) {
    SHELL_SendString((unsigned char*)"StepsTurn Stopping Timeout.\r\n");
  }
#endif
  currLPos = Q4CLeft_GetPos();
  currRPos = Q4CRight_GetPos();
  targetLPos = currLPos+stepsL;
  targetRPos = currRPos+stepsR;
  TURN_MoveToPos(targetLPos, targetRPos, TRUE, stopIt, timeOutMS); /* go to final position */
}

void TURN_Turn(TURN_Kind kind, TURN_StopFct stopIt) {
  switch(kind) {
    case TURN_LEFT45:
      StepsTurn(-TURN_Steps90/2, TURN_Steps90/2, stopIt, TURN_STEPS_90_TIMEOUT_MS/2);
      break;
    case TURN_RIGHT45:
      StepsTurn(TURN_Steps90/2, -TURN_Steps90/2, stopIt, TURN_STEPS_90_TIMEOUT_MS/2);
      break;
    case TURN_LEFT90:
      StepsTurn(-TURN_Steps90, TURN_Steps90, stopIt, TURN_STEPS_90_TIMEOUT_MS);
      break;
    case TURN_RIGHT90:
      StepsTurn(TURN_Steps90, -TURN_Steps90, stopIt, TURN_STEPS_90_TIMEOUT_MS);
      break;
    case TURN_LEFT180:
      StepsTurn(-(2*TURN_Steps90), 2*TURN_Steps90, stopIt, TURN_STEPS_90_TIMEOUT_MS*2);
     break;
    case TURN_RIGHT180:
      StepsTurn(2*TURN_Steps90, -(2*TURN_Steps90), stopIt, TURN_STEPS_90_TIMEOUT_MS*2);
     break;
    case TURN_STEP_BORDER_BW:
      StepsTurn(-(3*TURN_StepsLine), -(3*TURN_StepsLine), stopIt, TURN_STEPS_LINE_TIMEOUT_MS);
      break;
    case TURN_STEP_LINE_FW:
      StepsTurn(TURN_StepsLine, TURN_StepsLine, stopIt, TURN_STEPS_LINE_TIMEOUT_MS);
      break;
    case TURN_STEP_LINE_BW:
      StepsTurn(-TURN_StepsLine, -TURN_StepsLine, stopIt, TURN_STEPS_LINE_TIMEOUT_MS);
      break;
    case TURN_STEP_POST_LINE_FW:
      StepsTurn(TURN_StepsPostLine, TURN_StepsPostLine, stopIt, TURN_STEPS_POST_LINE_TIMEOUT_MS);
      break;
    case TURN_STEP_POST_LINE_BW:
      StepsTurn(-TURN_StepsPostLine, -TURN_StepsPostLine, stopIt, TURN_STEPS_POST_LINE_TIMEOUT_MS);
      break;
    case TURN_STEP_LINE_FW_POST_LINE: /* combination of TURN_STEP_LINE_FW and TURN_STEP_POST_LINE_FW */
      StepsTurn(TURN_StepsLine+TURN_StepsPostLine, TURN_StepsLine+TURN_StepsPostLine, stopIt, TURN_STEPS_LINE_TIMEOUT_MS+TURN_STEPS_POST_LINE_TIMEOUT_MS);
      break;
    case TURN_STEP_LINE_BW_POST_LINE: /* combination of TURN_STEP_LINE_BW and TURN_STEP_POST_LINE_BW */
      StepsTurn(-TURN_StepsLine-TURN_StepsPostLine, -TURN_StepsLine-TURN_StepsPostLine, stopIt, TURN_STEPS_LINE_TIMEOUT_MS+TURN_STEPS_POST_LINE_TIMEOUT_MS);
      break;
    case TURN_STOP_LEFT:
      MOT_SetSpeedPercent(MOT_GetMotorHandle(MOT_MOTOR_LEFT), 0);
      break;
    case TURN_STOP_RIGHT:
      MOT_SetSpeedPercent(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), 0);
      break;
    case TURN_STOP:
      MOT_SetSpeedPercent(MOT_GetMotorHandle(MOT_MOTOR_LEFT), 0);
      MOT_SetSpeedPercent(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), 0);
      break;
  default:
    break;
  }
}

void TURN_TurnAngle(int16_t angle, TURN_StopFct stopIt) {
  bool isLeft = angle<0;
  uint32_t steps;
  
  if (isLeft) {
    angle = -angle; /* make it positive */
  }
  angle %= 360; /* keep it inside 360° */
  steps = (angle*TURN_Steps90)/90;
  if (isLeft) {
    StepsTurn(-steps, steps, stopIt, ((angle/90)+1)*TURN_STEPS_90_TIMEOUT_MS);
  } else { /* right */
    StepsTurn(steps, -steps, stopIt, ((angle/90)+1)*TURN_STEPS_90_TIMEOUT_MS);
  }
}

#if PL_CONFIG_HAS_SHELL
static void TURN_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr((unsigned char*)"turn", (unsigned char*)"Group of turning commands\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  help|status", (unsigned char*)"Shows turn help or status\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  <angle>", (unsigned char*)"Turn the robot by angle, negative is counter-clockwise, e.g. 'turn -90'\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  forward", (unsigned char*)"Move one step forward\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  forward postline", (unsigned char*)"Move one step forward post the line\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  backward", (unsigned char*)"Move one step backward\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  steps90 <steps>", (unsigned char*)"Number of steps for a 90 degree turn\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  stepsline <steps>", (unsigned char*)"Number of steps for stepping over line\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  stepspostline <steps>", (unsigned char*)"Number of steps for a step post the line\r\n", io->stdOut);
}

static void TURN_PrintStatus(const CLS1_StdIOType *io) {
  unsigned char buf[32];

  CLS1_SendStatusStr((unsigned char*)"turn", (unsigned char*)"\r\n", io->stdOut);
  UTIL1_Num32sToStr(buf, sizeof(buf), TURN_Steps90);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" steps\r\n");
  CLS1_SendStatusStr((unsigned char*)"  90°", buf, io->stdOut);

  UTIL1_Num32sToStr(buf, sizeof(buf), TURN_StepsLine);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" steps\r\n");
  CLS1_SendStatusStr((unsigned char*)"  line", buf, io->stdOut);

  UTIL1_Num32sToStr(buf, sizeof(buf), TURN_StepsPostLine);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" steps\r\n");
  CLS1_SendStatusStr((unsigned char*)"  postline", buf, io->stdOut);

  UTIL1_Num32sToStr(buf, sizeof(buf), Q4CLeft_GetPos());
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)", ");
  UTIL1_strcatNum16u(buf, sizeof(buf), Q4CLeft_NofErrors());
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" errors\r\n");
  CLS1_SendStatusStr((unsigned char*)"  left pos", buf, io->stdOut);
  
  UTIL1_Num32sToStr(buf, sizeof(buf), Q4CRight_GetPos());
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)", ");
  UTIL1_strcatNum16u(buf, sizeof(buf), Q4CRight_NofErrors());
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" errors\r\n");
  CLS1_SendStatusStr((unsigned char*)"  right pos", buf, io->stdOut);
}

static bool isNumberStart(uint8_t ch) {
  if (ch=='-') { /* negative number start */
    return TRUE;
  } else if (ch>='0' && ch<='9') {
    return TRUE;
  }
  return FALSE;
}

uint8_t TURN_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint8_t res = ERR_OK;
  const unsigned char *p;
  uint16_t val16u;

  if (UTIL1_strcmp((char*)cmd, (char*)CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, (char*)"turn help")==0) {
    TURN_PrintHelp(io);
    *handled = TRUE;
  } else if (UTIL1_strcmp((char*)cmd, (char*)CLS1_CMD_STATUS)==0 || UTIL1_strcmp((char*)cmd, (char*)"turn status")==0) {
    TURN_PrintStatus(io);
    *handled = TRUE;
  } else if (UTIL1_strncmp((char*)cmd, (char*)"turn ", sizeof("turn ")-1)==0 && isNumberStart(cmd[sizeof("turn ")-1])) {
    int32_t angle;

    p = cmd+sizeof("turn ")-1;
    if (UTIL1_xatoi(&p, &angle)==ERR_OK) {
      TURN_TurnAngle((int16_t)angle, NULL);
      TURN_Turn(TURN_STOP, NULL);
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strcmp((char*)cmd, (char*)"turn forward postline")==0) {
    TURN_Turn(TURN_STEP_LINE_FW_POST_LINE, NULL);
    TURN_Turn(TURN_STOP, NULL);
    *handled = TRUE;
  } else if (UTIL1_strcmp((char*)cmd, (char*)"turn forward")==0) {
    TURN_Turn(TURN_STEP_LINE_FW, NULL);
    TURN_Turn(TURN_STOP, NULL);
    *handled = TRUE;
  } else if (UTIL1_strcmp((char*)cmd, (char*)"turn backward")==0) {
    TURN_Turn(TURN_STEP_LINE_BW, NULL);
    TURN_Turn(TURN_STOP, NULL);
    *handled = TRUE;
  } else if (UTIL1_strncmp((char*)cmd, (char*)"turn steps90 ", sizeof("turn steps90 ")-1)==0) {
    p = cmd+sizeof("turn steps90");
    if (UTIL1_ScanDecimal16uNumber(&p, &val16u)==ERR_OK) {
      TURN_Steps90 = val16u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strncmp((char*)cmd, (char*)"turn stepspostline ", sizeof("turn stepspostline ")-1)==0) {
    p = cmd+sizeof("turn stepspostline");
    if (UTIL1_ScanDecimal16uNumber(&p, &val16u)==ERR_OK) {
      TURN_StepsPostLine = val16u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strncmp((char*)cmd, (char*)"turn stepsline ", sizeof("turn stepsline ")-1)==0) {
    p = cmd+sizeof("turn stepsline");
    if (UTIL1_ScanDecimal16uNumber(&p, &val16u)==ERR_OK) {
      TURN_StepsLine = val16u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  }
  return res;
}
#endif /* PL_CONFIG_HAS_SHELL */

void TURN_Deinit(void) {
}

void TURN_Init(void) {
  TURN_Steps90 = TURN_STEPS_90;
  TURN_StepsPostLine = TURN_STEPS_POST_LINE;
  TURN_StepsLine = TURN_STEPS_LINE;
}
#endif /* PL_CONFIG_HAS_TURN */
