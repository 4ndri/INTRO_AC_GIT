/*
 * LineFollow.h
 *
 *  Created on: Feb 15, 2013
 *      Author: Erich Styger
 */

#ifndef LINEFOLLOW_H_
#define LINEFOLLOW_H_

#include "Platform.h"
#if PL_CONFIG_HAS_LINE_FOLLOW

#if PL_CONFIG_HAS_SHELL
#include "CLS1.h"

uint8_t LF_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
#endif

void LF_StartFollowing(void);
void LF_StopFollowing(void);
void LF_StartStopFollowing(void);
bool LF_IsFollowing(void);

void LF_Init(void);
void LF_Deinit(void);

void LF_SetIsLeftHandRule(bool newRule);

#endif /* PL_CONFIG_HAS_LINE_FOLLOW */

#endif /* LINEFOLLOW_H_ */
