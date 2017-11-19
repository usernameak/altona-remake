/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_BASE_SOUND_HPP
#define FILE_BASE_SOUND_HPP

#include "base/types.hpp"

/****************************************************************************/

typedef void (*sSoundHandler)(int16_t *data,int count);      // 1 count = left+right = 4 bytes

// sound out

sBool sSetSoundHandler(int freq,sSoundHandler,int latency,int flags=0);
void sClearSoundHandler();
int sGetCurrentSample();
void sClearSoundBuffer();

void sSoundHandlerNull(int16_t *data,int count);
void sSoundHandlerTest(int16_t *data,int count);

enum sSoundOutFlags
{
  sSOF_GLOBALFOCUS = 1,
  sSOF_5POINT1 = 2,     // 6-channel 5.1 output instead of stereo
};

// sound in

sBool sSetSoundInHandler(int freq,sSoundHandler,int latency);
void sSoundInput();
int sGetCurrentInSample();

void sLockSound();
void sUnlockSound();

/****************************************************************************/

#endif
