/**
 * \file
 * \brief This is the interface to PID Module
 * \author Erich Styger, erich.styger@hslu.ch
 */

#ifndef PID_H_
#define PID_H_

#include "Platform.h"
#if PL_CONFIG_HAS_PID

#if PL_CONFIG_HAS_SHELL
#include "CLS1.h"
/*!
 * \brief Shell command line parser.
 * \param[in] cmd Pointer to command string
 * \param[out] handled If command is handled by the parser
 * \param[in] io Std I/O handler of shell
 */
uint8_t PID_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io);
#endif

/*!
 * \brief Performs PID closed loop calculation for the speed
 * \param currSpeed Current speed of motor
 * \param setSpeed desired speed of motor
 * \param isLeft TRUE if is for the left motor, otherwise for the right motor
 */
void PID_Speed(int32_t currSpeed, int32_t setSpeed, bool isLeft);

/*!
 * \brief Performs PID closed loop calculation for the line position
 * \param currPos Current position of wheel
 * \param setPos Desired wheel position
 * \param isLeft TRUE if is for the left wheel, otherwise for the right wheel
 */
void PID_Pos(int32_t currPos, int32_t setPos, bool isLeft);

void PID_Line(uint16_t currLine, uint16_t setLine);

/*! \brief Driver re-init and reset */
void PID_Start(void);

/*! \brief Driver initialization */
void PID_Init(void);

/*! \brief Driver de-initialization */
void PID_Deinit(void);

#endif /* PL_CONFIG_HAS_PID */

#endif /* PID_H_ */
