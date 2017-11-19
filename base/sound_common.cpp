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

#include "base/types.hpp"

#include "base/sound.hpp"
#include "base/system.hpp"

/****************************************************************************/

void sSoundHandlerNull(int16_t *data,int count)
{
  for(int i=0;i<count;i++)
  {
    data[0] = 0;
    data[1] = 0;
    data+=2;
  }
}

static int phase0,phase1;
static int mod=0x4000;

void sSoundHandlerTest(int16_t *data,int count)
{
  for(int i=0;i<count;i++)
  {
    data[0] = int(sFSin(((phase0)&0xffff)/65536.0f*sPI2F)*12000);
    data[1] = int(sFSin(((phase1)&0xffff)/65536.0f*sPI2F)*12000);

    phase0 += (mod+(mod/50))/16;
    phase1 += (mod+(mod/50))/16;
//    mod = mod+4;
    if(mod>0x40000) mod = 0x1000;

    data+=2;
  }
}

/****************************************************************************/
