/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "gui/tabs.hpp"

/****************************************************************************/

sTabBorderBase::sTabBorderBase()
{
  Height = sGui->PropFont->GetHeight()+6;
  Flags |= sWF_HOVER;
  HoverTab = -1;
  HoverKill = -1;
  ActiveTab = 0;
  MoveBefore = -1;

  ScrollWidth = 0;
  Scroll = 0;
  DragScroll = 0;
  ScrollX0 = 0;
  ScrollX1 = 0;
  ScrollTo = -1;
}

sTabBorderBase::~sTabBorderBase()
{
}

void sTabBorderBase::OnLayout()
{
  Rect = Parent->Inner;
  Rect.y1 = Parent->Inner.y0 = Rect.y0+Height;
}

void sTabBorderBase::OnCalcSize()
{
  ReqSizeX = Height;
}

/****************************************************************************/

void sTabBorderBase::OnPaint2D()
{
  // get total width

  sFont2D *font = sGui->PropFont;
  font->SetColor(sGC_TEXT,sGC_BACK);
  int max = GetTabCount();
  int space = (font->GetWidth(L" ")/2+1)*2;
  int kill = (Height/4)*2+1;
  if(max==1)
    kill = -2*space;
  Rects.Resize(max);
  int arrow = font->GetWidth(L"\x25c4  ");

  // layout rects

  int xs=0;
  for(int i=0;i<max;i++)
  {
    xs+=space;
    const sChar *text = GetTabName(i);
    int xw = font->GetWidth(text);

    Rects[i].Client.Init(Rect.x0+xs,Rect.y0+2,Rect.x0+xs+space*4+xw+kill,Rect.y1-1);
    xs+= space*4 + xw + kill;
    sRect r=Rects[i].Client;
    r.x0++;
    r.x1--;
    r.y0++;
    Rects[i].Kill.Init(r.x1-kill-space,r.CenterY()-kill/2,r.x1-space,r.CenterY()-kill/2+kill);
  }
  xs+=space;

  // prepare scrolling

  if(xs<=Client.SizeX())
  {
    ScrollWidth = 0;
  }
  else
  {
    ScrollWidth = xs - (Client.SizeX()-2*arrow);
    if(ScrollWidth<0)
      ScrollWidth = 0;

    if(ScrollTo>=0 && ScrollTo<max)
    {
      int over = 2*arrow;
      if(Rects[ScrollTo].Client.x0-Scroll < Client.x0+over)
        Scroll = (Rects[ScrollTo].Client.x0)-(Client.x0+over);
      if(Rects[ScrollTo].Client.x1-Scroll > Client.x1-2*arrow-over)
        Scroll = (Rects[ScrollTo].Client.x1)-(Client.x1-2*arrow-over);
      Scroll = sClamp(Scroll,0,ScrollWidth);
      ScrollTo = -1;
    }

    int xo = arrow-sMin(Scroll,ScrollWidth);

    Info *info;
    sFORALL(Rects,info)
    {
      info->Client.x0 += xo;
      info->Client.x1 += xo;
      info->Kill.x0 += xo;
      info->Kill.x1 += xo;
    }
  }

  sRect2D(Rect.x0,Rect.y1-1,Rect.x1,Rect.y1,sGC_DRAW);
  sClipPush();

  if(ScrollWidth)
  {
    sRect r;
    
    r = Rect;
    r.x1 = r.x0+arrow;
    r.y1--;
    font->Print(sF2P_OPAQUE|sF2P_LEFT,r,L" \x25c4 ");
    r = Rect;
    r.x0 = r.x1-arrow;
    r.y1--;
    font->Print(sF2P_OPAQUE|sF2P_RIGHT,r,L" \x25ba ");

    r = Rect;
    r.x0 += arrow;
    r.x1 -= arrow;
    ScrollX0 = r.x0;
    ScrollX1 = r.x1;

    sClipRect(r);
  }
  else
  {
    ScrollX0 = 0;
    ScrollX1 = 0;
    sClipRect(Rect);
  }
  sClipPush();

  // draw tabs


  for(int i=0;i<max;i++)
  {
    const sChar *text = GetTabName(i);
    sRect r = Rects[i].Client;

    sRect2D(r.x0,r.y0,r.x1,r.y0+1,(ActiveTab==i)?sGC_HIGH:sGC_LOW);
    sRect2D(r.x0,r.y0+1,r.x0+1,r.y1,(ActiveTab==i)?sGC_HIGH2:sGC_LOW2);
    sRect2D(r.x1-1,r.y0+1,r.x1,r.y1,(ActiveTab==i)?sGC_LOW:sGC_HIGH);
    r.x0++;
    r.x1--;
    r.y0++;

    if(max>1)
    {
      sRect rr = Rects[i].Kill;
      sRectFrame2D(rr,sGC_DRAW);
      rr.Extend(-1);
      sRect2D(rr,(ActiveTab==i)?sGC_BUTTON:sGC_BACK);
      int x = rr.CenterX();
      int y = rr.CenterY();
      int h = kill/2-3;
      sLine2D(x-h,y-h,x+h+1,y+h+1,(HoverKill==i)?sGC_HIGH:sGC_DRAW);
      sLine2D(x-h,y+h,x+h+1,y-h-1,(HoverKill==i)?sGC_HIGH:sGC_DRAW);
      sClipExclude(Rects[i].Kill);
    }

    font->SetColor((HoverTab==i)?sGC_HIGH:sGC_TEXT,(ActiveTab==i)?sGC_BUTTON:sGC_BACK);
    font->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,r,text);
    sClipExclude(Rects[i].Client);
  }

  // fill background

  sRect r;
  r = Rect;
  r.y1--;
  sRect2D(r,sGC_BACK);

  sClipPop();
  if(MoveBefore>=0 && MoveBefore<=max)
  {
    int x = 0;
    if(MoveBefore==max)
      x = Rects[MoveBefore-1].Client.x1+space/2;
    else 
      x = Rects[MoveBefore].Client.x0-space/2;
    int h=5;
    int y = Rect.y1-1;
    for(int i=0;i<h;i++)
      sLine2D(x-i,y-h+i,x+i+1,y-h+i,sGC_DRAW);
  }
  sClipPop();
  font->SetColor(sGC_TEXT,sGC_BACK);
}

sBool sTabBorderBase::OnKey(sU32 key)
{
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key & sKEYQ_CTRL) key |= sKEYQ_CTRL;
  int max = GetTabCount();
  int cur = GetTab();
  switch(key & (sKEYQ_MASK|sKEYQ_SHIFT|sKEYQ_CTRL|sKEYQ_ALT|sKEYQ_BREAK))
  {
  case sKEY_LEFT:
    if(cur>=1)
      SetTab(cur-1);
    break;
  case sKEY_RIGHT:
    if(cur<max-1)
      SetTab(cur+1);
    break;
  case sKEY_DELETE:
    if(max>1 && cur>=0)
      DelTab(cur);
    break;

  default:
    return 0;
  }
  return 1;
}


void sTabBorderBase::OnDrag(const sWindowDrag &dd)
{
  if(dd.Buttons!=4)
  {
    Info *r;
    int seltab = -1;
    int selkill = -1;
    int mb = -1;
    int max = GetTabCount();
    sFORALL(Rects,r)
    {
      if(r->Client.Hit(dd.MouseX,dd.MouseY))
      {
        seltab = _i;
        if(r->Kill.Hit(dd.MouseX,dd.MouseY))
          selkill = _i;
        break;
      }
    }

    switch(dd.Mode)
    {
    case sDD_HOVER:
      if(HoverTab!=seltab || HoverKill!=selkill)
      {
        HoverTab = seltab;
        HoverKill = selkill;
        sGui->Update(Rect);
      }
      break;
    case sDD_START:
      if(selkill>=0)
        DelTab(selkill);
      else if(seltab>=0)
        SetTab(seltab);
      break;
    case sDD_DRAG:
      if(seltab>=0)
      {
        if(dd.MouseX > Rects[seltab].Client.CenterX())
          mb = seltab+1;
        else
          mb = seltab;
      }
      break;
    case sDD_STOP:
      if(ActiveTab>=0 && ActiveTab<max && MoveBefore>=0 && MoveBefore<=max && ActiveTab!=MoveBefore && ActiveTab!=MoveBefore-1)
      {
        int m = MoveBefore;
        if(MoveBefore>ActiveTab)
          m--;
        MoveTab(ActiveTab,m);
        SetTab(m);
      }
      break;
    }
    if(mb!=MoveBefore)
    {
      MoveBefore = mb;
      sGui->Update(Rect);
    }
  }
  else
  {
    int x = Scroll;
    switch(dd.Mode)
    {
    case sDD_START:
      DragScroll = sMin(ScrollWidth,Scroll);
      break;
    case sDD_DRAG:
      x = sClamp(DragScroll-dd.DeltaX,0,ScrollWidth);
      break;
    }
    if(x!=Scroll)
    {
      Scroll = x;
      sGui->Update(Rect);
    }
  }
}

/****************************************************************************/

int sTabBorderBase::GetTab() 
{ 
  if(ActiveTab>=0 && ActiveTab<GetTabCount()) 
    return ActiveTab; 
  else 
    return -1; 
}

void sTabBorderBase::SetTab(int tab) 
{
  if(ActiveTab!=tab)
  {
    ActiveTab = tab;
    ScrollTo = tab;
    sGui->Update(Rect); 
    ChangeMsg.Post();
  }
}

void sTabBorderBase::DelTab(int tab)
{
  int max = GetTabCount();
  if(tab>=0 && tab<max && max>1)
  {
    DeleteTab(tab);
    max--;
    if(tab>=max)
      tab--;
    sVERIFY(tab>=0);
    ActiveTab = -1;
    SetTab(tab);
  }
}

/****************************************************************************/
