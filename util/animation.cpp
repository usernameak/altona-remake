/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "animation.hpp"

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   FadeData                                                           ***/
/***                                                                      ***/
/****************************************************************************/
#if 1
float sFadeData::GetFactor(uint32_t type)
{
  switch (type)
  {
  default:
  case LINEAR:
    return float(Time)/Length;
  case SMOOTH:
    {
    float x = float(Time)/Length;
    return -2*x*x*x+3*x*x;
    }
  case SMOOTH_IN:
    {
    float x = float(Time)/Length;
    return -1*x*x*x+2*x*x;
    }
  case SMOOTH_OUT:
    {
    float x = 1-float(Time)/Length;
    return 1-(-1*x*x*x+2*x*x);
    }
  case SWITCH:
    {
    return (float)((int)(float(Time)/Length));
    }
  }
}

/****************************************************************************/

void sFadeData::Fade (int delta, sFadeData::FadeDirection fade)
 {
   if      (fade==IN)  FadeIn(delta); 
   else if (fade==OUT) FadeOut(delta); 
   else if (Dir!=NONE) Fade(delta,Dir); 
 }
#endif
/****************************************************************************/
