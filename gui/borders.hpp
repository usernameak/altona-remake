/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_BORDERS_HPP
#define FILE_GUI_BORDERS_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/controls.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Borders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sFocusBorder : public sWindow
{
public:
  sCLASSNAME(sFocusBorder);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
};

class sThickBorder : public sWindow
{
public:
  sCLASSNAME(sThickBorder);
  sThickBorder();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  sBool Inverted;
};

class sThinBorder : public sWindow
{
public:
  sCLASSNAME(sThinBorder);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
};

class sSpaceBorder : public sWindow
{
  int Pen;
public:
  sCLASSNAME(sThinBorder);
  sSpaceBorder(int pen);
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
};

class sToolBorder : public sWindow
{
protected:
  void OnLayout(int y0,int y1);

  enum
  {
    WF_TB_RIGHTALIGNED = sWF_USER1, // additional window flag
  };

public:
  sCLASSNAME(sToolBorder);
  sToolBorder();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();

  void AddMenu(sChar *name,const sMessage &msg);
  void AddSpace(sBool rightaligned=sFALSE);
  void AddRightAligned(sWindow *w);
  sBool Bottom;
};

class sScrollBorder : public sWindow
{
  int Width;
  int KnopMin;
  int DragMode;
  int DragStartX;
  int DragStartY;
  sRect ButtonX;
  sRect ButtonY;
  sRect ButtonZ;
  sBool CalcKnop(int &a,int &b,int client,int inner,int button,int scroll);
public:
  sCLASSNAME(sScrollBorder);
  sScrollBorder();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

struct sStatusBorderItem
{
  sStatusBorderItem();
  int Width;                     // width in pixel, negative from right border, 0 for all remaining
  sStringDesc Buffer;             // text mode
  int ProgressMax;               // progress bar mode
  int ProgressValue;
  sU32 Color;                     // 0 for default

  sRect Client;                   // calculated automatically
};

class sStatusBorder : public sWindow
{
  sArray<sStatusBorderItem> Items;
  sString<1024> PrintBuffer;
  int Height;
  sU32 Color;
public:
  sCLASSNAME(sStatusBorder);
  sStatusBorder();
  ~sStatusBorder();

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();

  void AddTab(int width,int maxstring=256);
  void Print(int tab,const sChar *string,int len=-1,sU32 color=0);
  void SetColor(int tab,sU32 rgb);        // gets reset by Print/PrintF
  void Progress(int tab,int value,int max);

  sPRINTING1(PrintF,sFormatStringBuffer buf=sFormatStringBase(PrintBuffer,format);buf,Print(arg1,PrintBuffer);,int);  
};

/****************************************************************************/

#endif
