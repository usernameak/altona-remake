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

#include "main.hpp"
#include "gui/textwindow.hpp"
#include "util/image.hpp"
#include "util/rasterizer.hpp"

/****************************************************************************/

sISGUI(sTRUE);

MyWindow::MyWindow()
{
  BackBuffer = 0;

}
void MyWindow::OnPaint2D()
{
  sRandom rnd;
  sRect r;

  if(BackBuffer) sGui->BeginBackBuffer(Client);
  sRect2D(Client,sGC_BACK);

  for(int i=50;i>1;i--)
  {
    r.x0 = Client.CenterX()-Client.SizeX()*i/110;
    r.x1 = Client.CenterX()+Client.SizeX()*i/110;
    r.y0 = Client.CenterY()-Client.SizeY()*i/110;
    r.y1 = Client.CenterY()+Client.SizeY()*i/110;
    sGuiColor color;
    switch(i%3) {
      case 0:
        color = sGC_RED;
      break;
      case 1:
        color = sGC_GREEN;
      break;
      case 2:
        color = sGC_BLUE;
      break;
    }
    sRect2D(r,color);
  }
  

  if(BackBuffer) sGui->EndBackBuffer();

  sGui->PropFont->SetColor(sGC_TEXT, sGC_BACK);
  sGui->PropFont->Print(0, Client.x0 + 4, Client.y0 + 4, BackBuffer ? L"BackBuffer" : L"No BackBuffer");
}

/****************************************************************************/

MyParentWin::MyParentWin()
{
  sVSplitFrame *v;
  MyWindow *w0,*w1;


  w0 = new MyWindow;
  w1 = new MyWindow;
  w1->BackBuffer = 1;
  v = new sVSplitFrame;
  v->AddChild(w0);
  v->AddChild(w1);
  AddChild(v);
}

void sMain()
{
  sInit(sISF_2D,800,600);
  sInitGui();
  sGui->AddBackWindow(new MyParentWin);
}

/****************************************************************************/
