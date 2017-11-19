/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2017 Anton Korzhuk, all rights reserved                        ***/
/***                                                                      ***/
/****************************************************************************/

#include "base/types.hpp"

#if sCONFIG_SYSTEM_LINUX

#include "base/sound.hpp"
#include "base/system.hpp"

void sClearSoundHandler() {}
sBool sSetSoundHandler(int freq,sSoundHandler,int latency,int flags) {
    return false;
}
int sGetCurrentSample()
{
  return 0;
}


#endif