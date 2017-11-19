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

#ifndef HEADER_ALTONA_UTIL_WINDOWS
#define HEADER_ALTONA_UTIL_WINDOWS

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/serialize.hpp" // for clipboard serialize
class sImage2D;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Windowing stuff                                                    ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void sSetMousePointer(int code);

enum sMousePointerCodes
{
  sMP_OFF = 0,                    // Hide cursor
  sMP_ARROW,                      // varius images
  sMP_WAIT,
  sMP_CROSSHAIR,
  sMP_HAND,
  sMP_TEXT,
  sMP_NO,
  sMP_SIZEALL,
  sMP_SIZENS,
  sMP_SIZEWE,
  sMP_SIZENWSE,
  sMP_SIZENESW,
  sMP_MAX,                        // mark end of list
};

/****************************************************************************/

void sSetWindowName(const sChar *name);
const sChar *sGetWindowName();

/****************************************************************************/

void sSetWindowMode(int mode);
int sGetWindowMode();
sBool sHasWindowFocus();            // does our application window have the os-focus?
void sSetWindowSize(int x,int y);
void sSetWindowPos(int x,int y);
void sGetWindowPos(int &x,int &y);
void sGetWindowSize(int &sx,int &sy);

enum sWindowModeCodes
{
  sWM_NORMAL = 1,
  sWM_MAXIMIZED = 2,
  sWM_MINIMIZED = 3,
};

/****************************************************************************/

void sUpdateWindow();
void sUpdateWindow(const sRect &);

/****************************************************************************/

void sSetClipboard(const sChar *,int len=-1);
sChar *sGetClipboard();

void sSetClipboard(const uint8_t *data,ptrdiff_t size,uint32_t serid,int sermode);
uint8_t *sGetClipboard(ptrdiff_t &size,uint32_t serid,int sermode);

void sEnableFileDrop(sBool enable);
const sChar *sGetDragDropFile();

/****************************************************************************/

template <class Type> sBool sSetClipboardObject(Type *obj,uint32_t serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  sFile *file = sCreateGrowMemFile();
  sWriter stream; stream.Begin(file); obj->Serialize(stream); stream.End();
  if(!stream.IsOk()) return 0;
  ptrdiff_t size = file->GetSize();
  uint8_t *data = file->Map(0,size);
  sSetClipboard(data,size,serid,1);
  delete file;
  return 1;
#else
  return 0;
#endif
}

template <class Type> sBool sGetClipboardObject(Type *&obj,uint32_t serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  ptrdiff_t size;
  uint8_t *data = sGetClipboard(size,serid,1);
  if(!data) return 0;
  sFile *file = sCreateMemFile(data,size);
  obj = new Type;
  sReader stream; stream.Begin(file); obj->Serialize(stream); stream.End();
  delete file;
  if(stream.IsOk())
    return 1;
  delete obj;
  obj = 0;
#endif
  return 0;
}

template <class Type> sBool sSetClipboardArray(sStaticArray<Type *>&arr,uint32_t serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  Type *e;
  sFile *file = sCreateGrowMemFile();
  sWriter stream; stream.Begin(file); 
  stream.Header(sSerId::sStaticArray,1);
  stream.ArrayNew(arr);
  sFORALL(arr,e)
    e->Serialize(stream);
  stream.Footer();
  stream.End();
  if(!stream.IsOk()) return 0;
  ptrdiff_t size = file->GetSize();
  uint8_t *data = file->Map(0,size);
  sSetClipboard(data,size,serid,2);
  delete file;
  return 1;
#else
  return 0;
#endif
}


template <class Type> sBool sGetClipboardArray(sStaticArray<Type *>&arr,uint32_t serid)
{
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
  Type *e;
  ptrdiff_t size;
  uint8_t *data = sGetClipboard(size,serid,2);
  if(!data) return 0;
  sFile *file = sCreateMemFile(data,size);
  sReader stream; stream.Begin(file); 
  stream.Header(sSerId::sStaticArray,1);
  stream.ArrayNew(arr);
  sFORALL(arr,e)
    e->Serialize(stream);
  stream.Footer();
  stream.End();
  delete file;
  return stream.IsOk();
#else
  return 0;
#endif
}

/****************************************************************************/

enum sSystemFileOpenDialogFlags
{
  sSOF_LOAD = 1,
  sSOF_SAVE = 2,
  sSOF_DIR = 3,

  sSOF_MULTISELECT = 4,
};

sBool sSystemOpenFileDialog(const sChar *label,const sChar *extensions,int flags,const sStringDesc &buffer);

enum sSystemMessageFlags
{
  sSMF_ERROR = 1,
  sSMF_OK = 2,
  sSMF_OKCANCEL = 3,
  sSMF_YESNO = 4,
};

sBool sSystemMessageDialog(const sChar *label,int flags);
sBool sSystemMessageDialog(const sChar *label,int flags,const sChar *title);

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Graph2D - interface to standard windows system                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

// convention: use colid 0 as short-time color!
// drawing and clipping only allowed during OnPaint2D or 
// between sRender2DBegin/sRender2DEnd

// drawing

void sSetColor2D(int colid,uint32_t color);
uint32_t sGetColor2D(int colid);
void sRect2D(int x0,int y0,int x1,int y1,int colid);
void sRect2D(const sRect &,int colid);
void sRectInvert2D(int x0,int y0,int x1,int y1);
void sRectInvert2D(const sRect &);
void sRectFrame2D(int x0,int y0,int x1,int y1,int colid);
void sRectFrame2D(const sRect &,int colid);
void sRectHole2D(const sRect &out,const sRect &hole,int colid);
void sLine2D(int x0,int y0,int x1,int y1,int colid);
void sLine2D(int *list,int count,int colid);
void sLineList2D(int *list,int count,int colid);
void sBlit2D(const uint32_t *data,int width,const sRect &dest);      // good for constantly changing data, for constant images use sImage2D!
void sStretch2D(const uint32_t *data,int width,const sRect &source,const sRect &dest);

// clipping

void sClipFlush();
void sClipPush();
void sClipPop();
void sClipExclude(const sRect &);
void sClipRect(const sRect &);

// render control. you can call it during OnPaint2D(), but do not nest any further.

void sRender2DBegin();                // this renders on screen, outside messageloop
void sRender2DBegin(int xs,int ys); // render to throwaway-offscreen surface
void sRender2DBegin(sImage2D *);      // render to preallocated offscreen surface
void sRender2DEnd();                                  
void sRender2DSet(uint32_t *data);        // set pixels (data -> bitmap)
void sRender2DGet(uint32_t *data);        // get pixels (bitmap -> data)


/****************************************************************************/

struct sPrintInfo
{
  void Init();
  int Mode;                      // sPIM_???         

  int CursorPos;                 // print cursor here. -1 to disable
  int Overwrite;                 // print a fat overwrite cursor
  int SelectStart;               // start selection mark here, -1 to disable
  int SelectEnd;                 // end selection mark here, must be greater than start
  uint32_t SelectBackColor;           // second back color for selection. -1 to swap front and back instead
  int HintLine;                  // fine vertical line that hints the right border of the page
  uint32_t HintLineColor;             // sGC_???

  sRect CullRect;                 // multiline prints profit from culling

  int QueryX;
  int QueryY;
  const sChar *QueryPos;
  sBool HasSelection() { return SelectStart>=0 && SelectEnd>=0 && SelectStart<SelectEnd; }
};

enum sPrintInfoMode
{
  sPIM_PRINT = 0,                 // just print the text
  sPIM_GETHEIGHT,                 // ??
  sPIM_POINT2POS,                 // set QueryPos on letter index for a QueryX/QueryY Click.
  sPIM_POS2POINT,                 // set QueryX/QueryY for letter of index QueryPos.
  sPIM_QUERYDONE,                 // this is set after successfull sPIM_POINT2POS
};

class sFont2D
{
  struct sFont2DPrivate *prv;
public:
  sFont2D(const sChar *name,int size,int flags,int width=0);
  ~sFont2D();

  int GetHeight();
  int GetBaseline();
  int GetCharHeight();
  int GetWidth(const sChar *text,int len=-1);
  int GetAdvance(const sChar *text,int len=-1);
  int GetCharCountFromWidth(int width,const sChar *text,int len=-1);

  struct sLetterDimensions
  {
    int16_t Pre;                       ///< kerning before character in bitmap-pixels
    int16_t Cell;                      ///< size of bitmap-cell in bitmap-pixels
    int16_t Post;                      ///< kerning after character in bitmap-pixels
    int16_t Advance;                   ///< total advance in bitmap-pixels
    int16_t OriginY;                   // glyph origin relative to baseline
    int16_t Height;                    // height of the glyphs black box
  };
  sLetterDimensions sGetLetterDimensions(const sChar letter);
  sBool LetterExists(sChar letter);

  static void AddResource(const sChar *filename);
  void Init(const sChar *name,int size,int flags,int width=0);
  void Exit();
  void SetColor(int text,int back);
  void PrintMarked(int flags,const sRect *r,int x,int y,const sChar *text,int len=-1,sPrintInfo *pi=0);
  void PrintBasic(int flags,const sRect *r,int x,int y,const sChar *text,int len=-1);
  void Print(int flags,int x,int y,const sChar *text,int len=-1);
  int Print(int flags,const sRect &r,const sChar *text,int len=-1,int margin=0,int xo=0,int yo=0,sPrintInfo *pi=0);
};

enum sFont2DCreateFlags
{
  sF2C_ITALICS     = 0x0001,
  sF2C_BOLD        = 0x0002,
  sF2C_HINTED      = 0x0004,
  sF2C_UNDERLINE   = 0x0008,
  sF2C_STRIKEOUT   = 0x0010,
  sF2C_SYMBOLS     = 0x0020,        // required to load windings or webdings
  sF2C_NOCLEARTYPE = 0x0040,        // disable cleartype on windows. for render font into texture.
};

enum sFont2DPrintFlags
{
  sF2P_LEFT         = 0x0001,       // align
  sF2P_RIGHT        = 0x0002,
  sF2P_TOP          = 0x0004,
  sF2P_BOTTOM       = 0x0008, 
  sF2P_MULTILINE    = 0x0010,       // wrap words
  sF2P_JUSTIFIED    = 0x0020,       // justify paragraphs
  sF2P_LIMITED      = 0x0040,       // squeeze into box
  sF2P_SPACE        = 0x0080,       // add a "space" to the margin

  sF2P_OPAQUE       = 0x1000,
};

/****************************************************************************/

class sImage2D
{
  friend void sRender2DBegin(sImage2D *img);
  public: struct sImage2DPrivate *prv; // TODO: make non-public
public:
  sImage2D(int xs,int ys,uint32_t *data);
  ~sImage2D();

  void Update(uint32_t *data);
  void Paint(int x,int y);
  void Paint(const sRect &source,int x,int y);
  void Stretch(const sRect &source,const sRect &dest);

  int GetSizeX();
  int GetSizeY();
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_WINDOWS
#endif


