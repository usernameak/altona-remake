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

#ifndef HEADER_ALTONA_UTIL_PAINTER
#define HEADER_ALTONA_UTIL_PAINTER

#ifndef __GNUC__
#pragma once
#endif


#include "base/types.hpp"
#include "base/system.hpp"

/****************************************************************************/

class sBasicPainter
{
#if !sCONFIG_QUADRICS
  int Alloc;
  int Used;
  struct sVertexSingle *Vertices;
#endif

  class sGeometry *Geo;
  class sGeometry *GeoX;
  class sGeometry *GeoXStandard;
  class sSimpleMaterial *Mtrl;
  class sCubeMaterial *CubeMtrl;
  class sMaterialEnv *Env;
  class sTexture2D *Tex;
  class sViewport *View;
  float UVOffset;
  float XYOffset;

public:
  // management
  sBasicPainter(int vertexmax = 0);
  ~sBasicPainter();
  void SetTarget();
  void SetTarget(const sRect &target);
  void Begin();
  void End();
  void Clip(const sFRect &r);
  void ClipOff();

  // shapes
  void Box(const sFRect &r,const uint32_t col);
  void Box(const sFRect &r,const uint32_t *colors);
  void Line(float x0,float y0,float x1,float y1,uint32_t c0,uint32_t c1,sBool skiplastpixel=0);

  // fonts. select unique fontid as you like
  void RegisterFont(int fontid,const sChar *name,int height,int style);
  void Print(int fontid,float x,float y,uint32_t col,const sChar *text,int len=-1,float zoom=1.0f);
  float GetWidth(int fontid,const sChar *text,int len=-1);
  float GetHeight(int fontid);

  // debbuging textures
  void PaintTexture(sTexture2D* tex, uint32_t col=0xffffffff, int xs=-1, int ys=-1, int xo=0, int yo=0, sBool noalpha=sTRUE);
  void PaintTexture(class sTextureCube *tex, int xs=-1, int ys=-1, int xo=0, int yo=0, sBool noalpha=sTRUE);
};

/****************************************************************************/

class sPainter : public sBasicPainter
{
  int FontId;
  int Advance;
  uint32_t TextColor;
  uint32_t AltColor;
  float Zoom;
  sString<1024> classbuf;
  void Print0(float x,float y,const sChar *txt);
public:
  sPainter(int vertexmax = 0);
  void Box(const sFRect &r,uint32_t col);
  void Box(float x0,float y0,float x1,float y1,uint32_t col);
  void Print(float x,float y,const sChar *txt);
  void Print(int fontid,float x,float y,uint32_t col,const sChar *text,int len=-1,float zoom=1.0f) { sBasicPainter::Print(fontid,x,y,col,text,len,zoom); }
  void SetPrint(int fontid,uint32_t col,float zoom,uint32_t altcol=0xffffffff,int advance = 10);
  sPRINTING2(PrintF,sFormatStringBuffer buf=sFormatStringBase(classbuf,format);buf,Print0(arg1,arg2,classbuf);,float,float);

	using sBasicPainter::Box;
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_PAINTER
#endif
