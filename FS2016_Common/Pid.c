/**
 * \file
 * \brief This is the implementation of the PID Module
 * \author Erich Styger, erich.styger@hslu.ch
 */

#include "Platform.h"
#if PL_CONFIG_HAS_PID
#include "Pid.h"
#include "Motor.h"
#include "UTIL1.h"
#if PL_CONFIG_HAS_SHELL
  #include "CLS1.h"
#endif
#include "Reflectance.h"

typedef struct {
  int32_t pFactor100;
  int32_t iFactor100;
  int32_t dFactor100;
  uint8_t maxSpeedPercent; /* limitation of PID value */
  int32_t iAntiWindup;
  int32_t lastError;
  int32_t integral;
} PID_Config;

/*! \todo Add your own additional configurations as needed, at least with a position config */
static PID_Config lineFwConfig;
static PID_Config speedLeftConfig, speedRightConfig;
static PID_Config posLeftConfig, posRightConfig;

static int32_t PID(int32_t currVal, int32_t setVal, PID_Config *config) {
  int32_t error;
  int32_t pid;
  
  /* perform PID closed control loop calculation */
  error = setVal-currVal; /* calculate error */
  pid = (error*config->pFactor100)/100; /* P part */
  config->integral += error; /* integrate error */
  if (config->integral>config->iAntiWindup) {
    config->integral = config->iAntiWindup;
  } else if (config->integral<-config->iAntiWindup) {
    config->integral = -config->iAntiWindup;
  }
  pid += (config->integral*config->iFactor100)/100; /* add I part */
  pid += ((error-config->lastError)*config->dFactor100)/100; /* add D part */
  config->lastError = error; /* remember for next iteration of D part */
  return pid;
}

void PID_SpeedCfg(int32_t currSpeed, int32_t setSpeed, bool isLeft, PID_Config *config) {
  int32_t speed;
  MOT_Direction direction=MOT_DIR_FORWARD;
  MOT_MotorDevice *motHandle;
  
  speed = PID(currSpeed, setSpeed, config);
  if (speed>=0) {
    direction = MOT_DIR_FORWARD;
  } else { /* negative, make it positive */
    speed = -speed; /* make positive */
    direction = MOT_DIR_BACKWARD;
  }
  /* speed shall be positive here, make sure it is within 16bit PWM boundary */
  if (speed>0xFFFF) {
    speed = 0xFFFF;
  }
  /* send new speed values to motor */
  if (isLeft) {
    motHandle = MOT_GetMotorHandle(MOT_MOTOR_LEFT);
  } else {
    motHandle = MOT_GetMotorHandle(MOT_MOTOR_RIGHT);
  }
  MOT_SetVal(motHandle, 0xFFFF-speed); /* PWM is low active */
  MOT_SetDirection(motHandle, direction);
  MOT_UpdatePercent(motHandle, direction);
}

static int32_t Limit(int32_t val, int32_t minVal, int32_t maxVal) {
  if (val<minVal) {
    return minVal;
  } else if (val>maxVal) {
    return maxVal;
  }
  return val;
}

static MOT_Direction AbsSpeed(int32_t *speedP) {
  if (*speedP<0) {
    *speedP = -(*speedP);
    return MOT_DIR_BACKWARD;
  }
  return MOT_DIR_FORWARD;
}

/*! \brief returns error (always positive) percent */
static uint8_t errorWithinPercent(int32_t error) {
  if (error<0) {
    error = -error;
  }
  return error/(REF_MAX_LINE_VALUE/2/100);
}

#define PID_DEBUG 0

void PID_LineCfg(uint16_t currLine, uint16_t setLine, PID_Config *config) {
  int32_t pid, speed, speedL, speedR;
#if PID_DEBUG
  unsigned char buf[16];
  static uint8_t cnt = 0;
#endif
  uint8_t errorPercent;
  MOT_Direction directionL=MOT_DIR_FORWARD, directionR=MOT_DIR_FORWARD;

  pid = PID(currLine, setLine, config);
  errorPercent = errorWithinPercent(currLine-setLine);

  /* transform into different speed for motors. The PID is used as difference value to the motor PWM */
  if (errorPercent <= 20) { /* pretty on center: move forward both motors with base speed */
    speed = ((int32_t)config->maxSpeedPercent)*(0xffff/100); /* 100% */
    pid = Limit(pid, -speed, speed);
    if (pid<0) { /* turn right */
      speedR = speed;
      speedL = speed-pid;
    } else { /* turn left */
      speedR = speed+pid;
      speedL = speed;
    }
  } else if (errorPercent <= 40) {
    /* outside left/right halve position from center, slow down one motor and speed up the other */
    speed = ((int32_t)config->maxSpeedPercent)*(0xffff/100)*8/10; /* 80% */
    pid = Limit(pid, -speed, speed);
    if (pid<0) { /* turn right */
      speedR = speed+pid; /* decrease speed */
      speedL = speed-pid; /* increase speed */
    } else { /* turn left */
      speedR = speed+pid; /* increase speed */
      speedL = speed-pid; /* decrease speed */
    }
  } else if (errorPercent <= 70) {
    speed = ((int32_t)config->maxSpeedPercent)*(0xffff/100)*6/10; /* %60 */
    pid = Limit(pid, -speed, speed);
    if (pid<0) { /* turn right */
      speedR = 0 /*maxSpeed+pid*/; /* decrease speed */
      speedL = speed-pid; /* increase speed */
    } else { /* turn left */
      speedR = speed+pid; /* increase speed */
      speedL = 0 /*maxSpeed-pid*/; /* decrease speed */
    }
  } else  {
    /* line is far to the left or right: use backward motor motion */
    speed = ((int32_t)config->maxSpeedPercent)*(0xffff/100)*10/10; /* %80 */
    if (pid<0) { /* turn right */
      speedR = -speed+pid; /* decrease speed */
      speedL = speed-pid; /* increase speed */
    } else { /* turn left */
      speedR = speed+pid; /* increase speed */
      speedL = -speed-pid; /* decrease speed */
    }
    speedL = Limit(speedL, -speed, speed);
    speedR = Limit(speedR, -speed, speed);
    directionL = AbsSpeed(&speedL);
    directionR = AbsSpeed(&speedR);
  }
  /* speed is now always positive, make sure it is within 16bit PWM boundary */
  if (speedL>0xFFFF) {
    speedL = 0xFFFF;
  } else if (speedL<0) {
    speedL = 0;
  }
  if (speedR>0xFFFF) {
    speedR = 0xFFFF;
  } else if (speedR<0) {
    speedR = 0;
  }
  /* send new speed values to motor */
  MOT_SetVal(MOT_GetMotorHandle(MOT_MOTOR_LEFT), 0xFFFF-speedL); /* PWM is low active */
  MOT_SetDirection(MOT_GetMotorHandle(MOT_MOTOR_LEFT), directionL);
  MOT_SetVal(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), 0xFFFF-speedR); /* PWM is low active */
  MOT_SetDirection(MOT_GetMotorHandle(MOT_MOTOR_RIGHT), directionR);
#if PID_DEBUG /* debug diagnostic */
  {
    cnt++;
    if (cnt>10) { /* limit number of messages to the console */
      CLS1_StdIO_OutErr_FctType ioOut = CLS1_GetStdio()->stdOut;
      cnt = 0;

      CLS1_SendStr((unsigned char*)"line:", ioOut);
      buf[0] = '\0';
      UTIL1_strcatNum16u(buf, sizeof(buf), currLine);
      CLS1_SendStr(buf, ioOut);

      CLS1_SendStr((unsigned char*)" sum:", ioOut);
      buf[0] = '\0';
      UTIL1_strcatNum32Hex(buf, sizeof(buf), integral);
      CLS1_SendStr(buf, ioOut);

      CLS1_SendStr((unsigned char*)" left:", ioOut);
      CLS1_SendStr(directionL==MOT_DIR_FORWARD?(unsigned char*)"fw ":(unsigned char*)"bw ", ioOut);
      buf[0] = '\0';
      UTIL1_strcatNum16Hex(buf, sizeof(buf), speedL);
      CLS1_SendStr(buf, ioOut);

      CLS1_SendStr((unsigned char*)" right:", ioOut);
      CLS1_SendStr(directionR==MOT_DIR_FORWARD?(unsigned char*)"fw ":(unsigned char*)"bw ", ioOut);
      buf[0] = '\0';
      UTIL1_strcatNum16Hex(buf, sizeof(buf), speedR);
      CLS1_SendStr(buf, ioOut);

      CLS1_SendStr((unsigned char*)"\r\n", ioOut);
    }
  }
#endif
}

void PID_Line(uint16_t currLine, uint16_t setLine) {
  PID_LineCfg(currLine, setLine, &lineFwConfig);
}

void PID_Speed(int32_t currSpeed, int32_t setSpeed, bool isLeft) {
  if (isLeft) {
    PID_SpeedCfg(currSpeed, setSpeed, isLeft, &speedLeftConfig);
  } else {
    PID_SpeedCfg(currSpeed, setSpeed, isLeft, &speedRightConfig);
  }
}

void PID_PosCfg(int32_t currPos, int32_t setPos, bool isLeft, PID_Config *config) {
  int32_t speed, val;
  MOT_Direction direction=MOT_DIR_FORWARD;
  MOT_MotorDevice *motHandle;
  int error;
  #define POS_FILTER 5

  error = setPos-currPos;
  if (error>-POS_FILTER && error<POS_FILTER) { /* avoid jitter around zero */
    setPos = currPos;
  }
  speed = PID(currPos, setPos, config);
  /* transform into motor speed */
  speed *= 1000; /* scale PID, otherwise we need high PID constants */
  if (speed>=0) {
    direction = MOT_DIR_FORWARD;
  } else { /* negative, make it positive */
    speed = -speed; /* make positive */
    direction = MOT_DIR_BACKWARD;
  }
  /* speed is now always positive, make sure it is within 16bit PWM boundary */
  if (speed>0xFFFF) {
    speed = 0xFFFF;
  }
#if 1
  /* limit speed to maximum value */
  val = ((int32_t)config->maxSpeedPercent)*(0xffff/100); /* 100% */
  speed = Limit(speed, -val, val);
#else
  /* limit speed to maximum value */
  speed = (speed*config->maxSpeedPercent)/100;
#endif
  /* send new speed values to motor */
  if (isLeft) {
    motHandle = MOT_GetMotorHandle(MOT_MOTOR_LEFT);
  } else {
    motHandle = MOT_GetMotorHandle(MOT_MOTOR_RIGHT);
  }
  MOT_SetVal(motHandle, 0xFFFF-speed); /* PWM is low active */
  MOT_SetDirection(motHandle, direction);
  MOT_UpdatePercent(motHandle, direction);
}

void PID_Pos(int32_t currPos, int32_t setPos, bool isLeft) {
  if (isLeft) {
    PID_PosCfg(currPos, setPos, isLeft, &posLeftConfig);
  } else {
    PID_PosCfg(currPos, setPos, isLeft, &posRightConfig);
  }
}

#if PL_CONFIG_HAS_SHELL
static void PID_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr((unsigned char*)"pid", (unsigned char*)"Group of PID commands\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  help|status", (unsigned char*)"Shows PID help or status\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  speed (L|R) (p|d|i|w) <val>", (unsigned char*)"Sets P, D, I or anti-windup position value\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  speed (L|R) speed <value>", (unsigned char*)"Maximum speed % value\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  pos (L|R) (p|d|i|w) <val>", (unsigned char*)"Sets P, D, I or anti-windup position value\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  pos speed <value>", (unsigned char*)"Maximum speed % value\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  fw (p|i|d|w) <value>", (unsigned char*)"Sets P, I, D or anti-Windup line value\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  fw speed <value>", (unsigned char*)"Maximum speed % value\r\n", io->stdOut);
}

static void PrintPIDstatus(PID_Config *config, const unsigned char *kindStr, const CLS1_StdIOType *io) {
  unsigned char buf[48];
  unsigned char kindBuf[16];

  UTIL1_strcpy(kindBuf, sizeof(buf), (unsigned char*)"  ");
  UTIL1_strcat(kindBuf, sizeof(buf), kindStr);
  UTIL1_strcat(kindBuf, sizeof(buf), (unsigned char*)" PID");
  UTIL1_strcpy(buf, sizeof(buf), (unsigned char*)"p: ");
  UTIL1_strcatNum32s(buf, sizeof(buf), config->pFactor100);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" i: ");
  UTIL1_strcatNum32s(buf, sizeof(buf), config->iFactor100);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)" d: ");
  UTIL1_strcatNum32s(buf, sizeof(buf), config->dFactor100);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  CLS1_SendStatusStr(kindBuf, buf, io->stdOut);

  UTIL1_strcpy(kindBuf, sizeof(buf), (unsigned char*)"  ");
  UTIL1_strcat(kindBuf, sizeof(buf), kindStr);
  UTIL1_strcat(kindBuf, sizeof(buf), (unsigned char*)" windup");
  UTIL1_Num32sToStr(buf, sizeof(buf), config->iAntiWindup);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  CLS1_SendStatusStr(kindBuf, buf, io->stdOut);

  UTIL1_strcpy(kindBuf, sizeof(buf), (unsigned char*)"  ");
  UTIL1_strcat(kindBuf, sizeof(buf), kindStr);
  UTIL1_strcat(kindBuf, sizeof(buf), (unsigned char*)" error");
  UTIL1_Num32sToStr(buf, sizeof(buf), config->lastError);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  CLS1_SendStatusStr(kindBuf, buf, io->stdOut);

  UTIL1_strcpy(kindBuf, sizeof(buf), (unsigned char*)"  ");
  UTIL1_strcat(kindBuf, sizeof(buf), kindStr);
  UTIL1_strcat(kindBuf, sizeof(buf), (unsigned char*)" integral");
  UTIL1_Num32sToStr(buf, sizeof(buf), config->integral);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"\r\n");
  CLS1_SendStatusStr(kindBuf, buf, io->stdOut);

  UTIL1_strcpy(kindBuf, sizeof(buf), (unsigned char*)"  ");
  UTIL1_strcat(kindBuf, sizeof(buf), kindStr);
  UTIL1_strcat(kindBuf, sizeof(buf), (unsigned char*)" speed");
  UTIL1_Num8uToStr(buf, sizeof(buf), config->maxSpeedPercent);
  UTIL1_strcat(buf, sizeof(buf), (unsigned char*)"%\r\n");
  CLS1_SendStatusStr(kindBuf, buf, io->stdOut);
}

static void PID_PrintStatus(const CLS1_StdIOType *io) {
  CLS1_SendStatusStr((unsigned char*)"pid", (unsigned char*)"\r\n", io->stdOut);
  PrintPIDstatus(&lineFwConfig, (unsigned char*)"fw", io);
  PrintPIDstatus(&speedLeftConfig, (unsigned char*)"speed L", io);
  PrintPIDstatus(&speedRightConfig, (unsigned char*)"speed R", io);
  PrintPIDstatus(&posLeftConfig, (unsigned char*)"pos L", io);
  PrintPIDstatus(&posRightConfig, (unsigned char*)"pos R", io);
}

static uint8_t ParsePidParameter(PID_Config *config, const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  const unsigned char *p;
  uint8_t val8u;
  uint32_t val32u;
  uint8_t res = ERR_OK;

  if (UTIL1_strncmp((char*)cmd, (char*)"p ", sizeof("p ")-1)==0) {
    p = cmd+sizeof("p");
    if (UTIL1_ScanDecimal32uNumber(&p, &val32u)==ERR_OK) {
      config->pFactor100 = val32u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strncmp((char*)cmd, (char*)"i ", sizeof("i ")-1)==0) {
    p = cmd+sizeof("i");
    if (UTIL1_ScanDecimal32uNumber(&p, &val32u)==ERR_OK) {
      config->iFactor100 = val32u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strncmp((char*)cmd, (char*)"d ", sizeof("d ")-1)==0) {
    p = cmd+sizeof("d");
    if (UTIL1_ScanDecimal32uNumber(&p, &val32u)==ERR_OK) {
      config->dFactor100 = val32u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strncmp((char*)cmd, (char*)"w ", sizeof("w ")-1)==0) {
    p = cmd+sizeof("w");
    if (UTIL1_ScanDecimal32uNumber(&p, &val32u)==ERR_OK) {
      config->iAntiWindup = val32u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  } else if (UTIL1_strncmp((char*)cmd, (char*)"speed ", sizeof("speed ")-1)==0) {
    p = cmd+sizeof("speed");
    if (UTIL1_ScanDecimal8uNumber(&p, &val8u)==ERR_OK && val8u<=100) {
      config->maxSpeedPercent = val8u;
      *handled = TRUE;
    } else {
      CLS1_SendStr((unsigned char*)"Wrong argument\r\n", io->stdErr);
      res = ERR_FAILED;
    }
  }
  return res;
}

uint8_t PID_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint8_t res = ERR_OK;

  if (UTIL1_strcmp((char*)cmd, (char*)CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, (char*)"pid help")==0) {
    PID_PrintHelp(io);
    *handled = TRUE;
  } else if (UTIL1_strcmp((char*)cmd, (char*)CLS1_CMD_STATUS)==0 || UTIL1_strcmp((char*)cmd, (char*)"pid status")==0) {
    PID_PrintStatus(io);
    *handled = TRUE;
  } else if (UTIL1_strncmp((char*)cmd, (char*)"pid speed L ", sizeof("pid speed L ")-1)==0) {
    res = ParsePidParameter(&speedLeftConfig, cmd+sizeof("pid speed L ")-1, handled, io);
  } else if (UTIL1_strncmp((char*)cmd, (char*)"pid speed R ", sizeof("pid speed R ")-1)==0) {
    res = ParsePidParameter(&speedRightConfig, cmd+sizeof("pid speed R ")-1, handled, io);
  } else if (UTIL1_strncmp((char*)cmd, (char*)"pid pos L ", sizeof("pid pos L ")-1)==0) {
    res = ParsePidParameter(&speedLeftConfig, cmd+sizeof("pid pos L ")-1, handled, io);
  } else if (UTIL1_strncmp((char*)cmd, (char*)"pid pos R ", sizeof("pid pos R ")-1)==0) {
    res = ParsePidParameter(&speedRightConfig, cmd+sizeof("pid pos R ")-1, handled, io);
  } else if (UTIL1_strncmp((char*)cmd, (char*)"pid fw ", sizeof("pid fw ")-1)==0) {
    res = ParsePidParameter(&lineFwConfig, cmd+sizeof("pid fw ")-1, handled, io);
  }
  return res;
}
#endif /* PL_HAS_SHELL */

void PID_Start(void) {
  /* reset the 'memory' values of the structure back to zero */
  lineFwConfig.lastError = 0;
  lineFwConfig.integral = 0;

  speedLeftConfig.lastError = 0;
  speedLeftConfig.integral = 0;
  speedRightConfig.lastError = 0;
  speedRightConfig.integral = 0;
  posLeftConfig.lastError = 0;
  posLeftConfig.integral = 0;
  posRightConfig.lastError = 0;
  posRightConfig.integral = 0;
}

void PID_Deinit(void) {
  /* nothing needed */
}

void PID_Init(void) {
  /*! \todo determine your PID values */
#if 0
  speedLeftConfig.pFactor100 = 0;
  speedLeftConfig.iFactor100 = 0;
  speedLeftConfig.dFactor100 = 0;
  speedLeftConfig.iAntiWindup = 0;
  speedLeftConfig.lastError = 0;
  speedLeftConfig.integral = 0;

  speedRightConfig.pFactor100 = 0;
  speedRightConfig.iFactor100 = 0;
  speedRightConfig.dFactor100 = 0;
  speedRightConfig.iAntiWindup = 0;
  speedRightConfig.lastError = 0;
  speedRightConfig.integral = 0;
#else
  speedLeftConfig.pFactor100 = 1000;
  speedLeftConfig.iFactor100 = 45;
  speedLeftConfig.dFactor100 = 0;
  speedLeftConfig.iAntiWindup = 120000;
  speedLeftConfig.lastError = 0;
  speedLeftConfig.integral = 0;
  speedLeftConfig.maxSpeedPercent = 50;

  speedRightConfig.pFactor100 = speedLeftConfig.pFactor100;
  speedRightConfig.iFactor100 = speedLeftConfig.iFactor100;
  speedRightConfig.dFactor100 = speedLeftConfig.dFactor100;
  speedRightConfig.iAntiWindup = speedLeftConfig.iAntiWindup;
  speedRightConfig.lastError = 0;
  speedRightConfig.integral = 0;
  speedRightConfig.maxSpeedPercent = speedLeftConfig.maxSpeedPercent;
#endif
  lineFwConfig.pFactor100 = 4000;
  lineFwConfig.iFactor100 = 10;
  lineFwConfig.dFactor100 = 100;
  lineFwConfig.iAntiWindup = 100000;
  lineFwConfig.maxSpeedPercent = 15;
  lineFwConfig.lastError = 0;
  lineFwConfig.integral = 0;

  posLeftConfig.pFactor100 = 1000;
  posLeftConfig.iFactor100 = 2;
  posLeftConfig.dFactor100 = 50;
  posLeftConfig.iAntiWindup = 200;
  posLeftConfig.maxSpeedPercent = 40;
  posLeftConfig.lastError = 0;
  posLeftConfig.integral = 0;
  posRightConfig.pFactor100 = posLeftConfig.pFactor100;
  posRightConfig.iFactor100 = posLeftConfig.iFactor100;
  posRightConfig.dFactor100 = posLeftConfig.dFactor100;
  posRightConfig.iAntiWindup = posLeftConfig.iAntiWindup;
  posRightConfig.lastError = posLeftConfig.lastError;
  posRightConfig.integral = posLeftConfig.integral;
  posRightConfig.maxSpeedPercent = posLeftConfig.maxSpeedPercent;
}
#endif /* PL_CONFIG_HAS_PID */
