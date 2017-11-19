/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_BASE_DEVICES_HPP
#define FILE_BASE_DEVICES_HPP

#include "base/types2.hpp"

/****************************************************************************/

void sAddMidi(sBool logging=0, sBool onlyPhysical=0);


struct sMidiEvent
{
  uint8_t Device;
  uint8_t Status;
  uint8_t Value1;
  uint8_t Value2;
  uint32_t TimeStamp;
};


class sMidiHandler_
{
public:
  virtual ~sMidiHandler_() {}
  virtual sBool HasInput()=0;
  virtual sBool GetInput(sMidiEvent &e)=0;
  virtual void Output(uint8_t dev,uint8_t stat,uint8_t val1,uint8_t val2)=0;
  virtual const sChar *GetDeviceName(sBool out,int dev)=0;

  void Output(sMidiEvent &e) { Output(e.Device,e.Status,e.Value1,e.Value2); }
  sMessage InputMsg;
};

extern sMidiHandler_ *sMidiHandler;


/****************************************************************************/

#endif // FILE_BASE_DEVICES_HPP

