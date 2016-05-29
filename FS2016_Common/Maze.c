/*
 * Maze.c
 *
 *  Created on: Mar 11, 2013
 *      Author: Erich Styger
 */
#include "Platform.h"
#if PL_CONFIG_HAS_LINE_MAZE
#include "Maze.h"
#include "Turn.h"
#include "CLS1.h"
#include "LineFollow.h"
#include "Event.h"
#include "UTIL1.h"
#include "Shell.h"
#include "Reflectance.h"

#define MAZE_MIN_LINE_VAL      0x60   /* minimum value indicating a line */
static uint16_t SensorHistory[REF_NOF_SENSORS]; /* value of history while moving forward */

static void MAZE_SampleSensorHistory(void) {
  uint8_t i;
  uint16_t val[REF_NOF_SENSORS];

  REF_GetSensorValues(&val[0], REF_NOF_SENSORS);
  for(i=0; i<REF_NOF_SENSORS; i++) {
    if (val[i]>=(MAZE_MIN_LINE_VAL)) { /* only count line values */
      if (val[i]>SensorHistory[i]) {
        SensorHistory[i] = val[i];
      }
    }
  }
}

/*!
 * \brief Called during turning, will use it to sample sensor values.
 */
static bool MAZE_SampleTurnStopFunction(void) {
  MAZE_SampleSensorHistory();
  return FALSE; /* do not stop turning */
}

static REF_LineKind MAZE_HistoryLineKind(void) {
  int i, cnt, cntLeft, cntRight;

  cnt = cntLeft = cntRight = 0;
  for(i=0;i<REF_NOF_SENSORS;i++) {
    if (SensorHistory[i]>MAZE_MIN_LINE_VAL) { /* count only line values */
      cnt++;
      if (i<REF_NOF_SENSORS/2) {
        cntLeft++;
      } else {
        cntRight++;
      }
    }
  }
  if (cnt==0) {
    return REF_LINE_NONE;
  } else if (cnt==REF_NOF_SENSORS) {
    return REF_LINE_FULL;
  } else if (cntLeft>cntRight) {
    return REF_LINE_LEFT;
  } else { /* must be cntRight>cntLeft */
    return REF_LINE_RIGHT;
  }
}

void MAZE_ClearSensorHistory(void) {
  int i;

  for(i=0;i<REF_NOF_SENSORS;i++) {
    SensorHistory[i] = 0;
  }
}


#define MAZE_MAX_PATH        32 /* maximum number of turns in path */

static TURN_Kind path[MAZE_MAX_PATH]; /* recorded maze */
static uint8_t pathLength; /* number of entries in path[] */
static bool isSolved = FALSE; /* if we have solved the maze */

static TURN_Kind RevertTurn(TURN_Kind turn) {
  if (turn==TURN_LEFT90) {
    turn = TURN_RIGHT90;
  } else if (turn==TURN_RIGHT90) {
    turn = TURN_LEFT90;
  }
  return turn;
}

/**
 * \brief Reverts the path 
 */
static void MAZE_RevertPath(void) {
  int i, j;
  TURN_Kind tmp;
  
  if (pathLength==0) {
    return;
  }
  j = pathLength-1;
  i = 0;
  while(i<=j) {
    tmp = path[i];
    path[i] = RevertTurn(path[j]);
    path[j] = RevertTurn(tmp);
    i++; j--;
  }
}

TURN_Kind MAZE_SelectTurn(REF_LineKind prev, REF_LineKind curr) {
  if (prev==REF_LINE_NONE && curr==REF_LINE_NONE) { /* dead end */
    return TURN_RIGHT180; /* make U turn */
  }
  /*! \todo Implement all cases for turning */
  return TURN_STOP; /* error case */
}

void MAZE_SetSolved(void) {
  isSolved = TRUE;
  /*! \todo here the path could be reverted */
  MAZE_RevertPath();
  MAZE_AddPath(TURN_STOP); /* add an action to stop */
}

bool MAZE_IsSolved(void) {
  return isSolved;
}

void MAZE_AddPath(TURN_Kind kind) {
  if (pathLength<MAZE_MAX_PATH) {
    path[pathLength] = kind;
    pathLength++;
  } else {
    /* error! */
  }
}

/*!
 * \brief Performs path simplification.
 * The idea is that whenever we encounter x-TURN_RIGHT180-x or x-TURN_LEFT180-x, we simplify it by cutting the dead end.
 * For example if we have TURN_LEFT90-TURN_RIGHT180-TURN_LEFT90, this can be simplified with TURN_STRAIGHT.
 */
void MAZE_SimplifyPath(void) {
  /*! \todo */
}

/*!
 * \brief Performs a turn.
 * \return Returns TRUE while turn is still in progress.
 */
uint8_t MAZE_EvaluteTurn(bool *finished) {
  REF_LineKind historyLineKind, currLineKind;
  TURN_Kind turn;

  *finished = FALSE;
  currLineKind = REF_GetLineKind();
  if (currLineKind==REF_LINE_NONE) { /* nothing, must be dead end */
    turn = TURN_LEFT180;
  } else {
    MAZE_ClearSensorHistory(); /* clear history values */
    MAZE_SampleSensorHistory(); /* store current values */
    TURN_Turn(TURN_STEP_LINE_FW_POST_LINE, MAZE_SampleTurnStopFunction); /* do the line and beyond in one step */
    historyLineKind = MAZE_HistoryLineKind(); /* new read new values */
    currLineKind = REF_GetLineKind();
    turn = MAZE_SelectTurn(historyLineKind, currLineKind);
  }
  if (turn==TURN_FINISHED) {
    *finished = TRUE;
    LF_StopFollowing();
    SHELL_SendString((unsigned char*)"MAZE: finished!\r\n");
    return ERR_OK;
  } else if (turn==TURN_STRAIGHT) {
    /*! \todo Extend if necessary */
    SHELL_SendString((unsigned char*)"going straight\r\n");
    return ERR_OK;
  } else if (turn==TURN_STOP) { /* should not happen here? */
    LF_StopFollowing();
    SHELL_SendString((unsigned char*)"Failure, stopped!!!\r\n");
    return ERR_FAILED; /* error case */
  } else { /* turn or do something */
    /*! \todo Extend if necessary */
   return ERR_OK; /* turn finished */
  }
}

static void MAZE_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr((unsigned char*)"maze", (unsigned char*)"Group of maze following commands\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  help|status", (unsigned char*)"Shows maze help or status\r\n", io->stdOut);
  CLS1_SendHelpStr((unsigned char*)"  clear", (unsigned char*)"Clear the maze solution\r\n", io->stdOut);
}

#if PL_CONFIG_HAS_SHELL
static void MAZE_PrintStatus(const CLS1_StdIOType *io) {
  int i;
  
  CLS1_SendStatusStr((unsigned char*)"maze", (unsigned char*)"\r\n", io->stdOut);
  CLS1_SendStatusStr((unsigned char*)"  solved", MAZE_IsSolved()?(unsigned char*)"yes\r\n":(unsigned char*)"no\r\n", io->stdOut);
  CLS1_SendStatusStr((unsigned char*)"  path", (unsigned char*)"(", io->stdOut);
  CLS1_SendNum8u(pathLength, io->stdOut);
  CLS1_SendStr((unsigned char*)") ", io->stdOut);
  for(i=0;i<pathLength;i++) {
    CLS1_SendStr(TURN_TurnKindStr(path[i]), io->stdOut);
    CLS1_SendStr((unsigned char*)" ", io->stdOut);
  }
  CLS1_SendStr((unsigned char*)"\r\n", io->stdOut);
}

uint8_t MAZE_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint8_t res = ERR_OK;

  if (UTIL1_strcmp((char*)cmd, (char*)CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, (char*)"maze help")==0) {
    MAZE_PrintHelp(io);
    *handled = TRUE;
  } else if (UTIL1_strcmp((char*)cmd, (char*)CLS1_CMD_STATUS)==0 || UTIL1_strcmp((char*)cmd, (char*)"maze status")==0) {
    MAZE_PrintStatus(io);
    *handled = TRUE;
  } else if (UTIL1_strcmp((char*)cmd, (char*)"maze clear")==0) {
    MAZE_ClearSolution();
    *handled = TRUE;
  }
  return res;
}
#endif

TURN_Kind MAZE_GetSolvedTurn(uint8_t *solvedIdx) {
  if (*solvedIdx < pathLength) {
    return path[(*solvedIdx)++];
  } else {
    return TURN_STOP; 
  }
}

void MAZE_ClearSolution(void) {
  isSolved = FALSE;
  pathLength = 0;
}

void MAZE_Deinit(void) {
}

void MAZE_Init(void) {
  MAZE_ClearSolution();
}
#endif /* PL_HAS_LINE_SENSOR */
