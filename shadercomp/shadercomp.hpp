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

#ifndef HEADER_ALTONA_UTIL_SHADERS
#define HEADER_ALTONA_UTIL_SHADERS

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   The individual real compilers                                      ***/
/***                                                                      ***/
/****************************************************************************/

sBool sShaderCompileDX     (const sChar *source,const sChar *profile,const sChar *main,uint8_t *&data,int &size,int flags=0,sTextBuffer *errors=0);
sBool sShaderCompileCG     (const sChar *source,const sChar *profile,const sChar *main,uint8_t *&data,int &size,int flags=0,sTextBuffer *errors=0);

/****************************************************************************/
/***                                                                      ***/
/***   Shader compiler                                                    ***/
/***                                                                      ***/
/****************************************************************************/

// external compiler stuff.

struct sExternCompileBuffer
{
  sExternCompileBuffer(int size0, int size1) { Buffer = new uint8_t[size0]; BufferSize = size0; ResultSize = 0; Message = new sChar8[size1]; Message[0]=0; MessageSize = size1; }
  ~sExternCompileBuffer()            { sDeleteArray(Buffer); sDeleteArray(Message); }

  uint8_t *Buffer;
  int BufferSize;
  int ResultSize;
  sChar8 *Message;
  int MessageSize;

  void SetShader(const uint8_t *data, int size)  { for(int i=0;i<sMin(size,BufferSize);i++) Buffer[i]=data[i]; ResultSize = size; }
  void SetMessage(const sChar8 *msg)          { int i=0; for(;i<MessageSize-1&&*msg;i++) Message[i] = *msg++; Message[i] = 0; }
};

typedef sBool (*sCompileCallback)(sExternCompileBuffer *buffer, int stype, int dtype, int flags, const sChar8 *source, int len, const sChar8 *name);
//sBool sCompileExtern(sCompileCallback cb, sCompileResult &result, int stype, int dtype, int flags, const sChar8 *source, int len, const sChar8 *name);

/****************************************************************************/

#endif
