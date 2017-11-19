/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_GUI_TABS_HPP
#define FILE_GUI_TABS_HPP

#include "base/types2.hpp"
#include "gui/gui.hpp"

/****************************************************************************/

class sTabBorderBase : public sWindow     // this is a border!
{
  struct Info
  {
    sRect Client;
    sRect Kill;
  };
  sRect Rect;
  int Height;
  sArray<Info> Rects;
  int HoverTab;
  int HoverKill;
  int ActiveTab;
  int MoveBefore;
  int ScrollTo;

  int ScrollX0;
  int ScrollX1;
  int ScrollWidth;
  int Scroll;
  int DragScroll;
public:
  sTabBorderBase();
  ~sTabBorderBase();

  void OnPaint2D();
  void OnLayout();
  void OnCalcSize();
  sBool OnKey(uint32_t key);
  void OnDrag(const sWindowDrag &dd);

  int GetTab();
  void SetTab(int tab);
  void DelTab(int tab);

  sMessage ChangeMsg;

  virtual int GetTabCount()=0;
  virtual const sChar *GetTabName(int n)=0;
  virtual void DeleteTab(int n) {}
  virtual void MoveTab(int from,int to) {}
};


template <class Type> class sTabBorder : public sTabBorderBase
{
  sArray<Type> *Tabs;
public:
  sTabBorder(sArray<Type>&t) { Tabs = &t; }
  void Tag() { sTabBorderBase::Tag(); sNeed(*Tabs); }
  int GetTabCount() { return Tabs ? Tabs->GetCount() : 0; }
  const sChar *GetTabName(int n) { return (*Tabs)[n]->Label; }
  void DeleteTab(int n) { Tabs->RemAtOrder(n); }
  void MoveTab(int from,int to) { Type e=(*Tabs)[from]; Tabs->RemAtOrder(from); Tabs->AddBefore(e,to); }
};

/****************************************************************************/

#endif // FILE_GUI_TABS_HPP

