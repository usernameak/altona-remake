/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "gui/gui.hpp"
#include "base/graphics.hpp"
#include "shaders.hpp"

/****************************************************************************/


class s3DWindow : public sWireClientWindow
{
  sBool Enable;
  sBool ScreenshotFlag;
  float DragDist,DragRotX,DragRotY,DragRotZ;
  sMatrix34 DragPos;

  int QuakeTime;
  sBool QuakeMode;
  int QuakeMask;
  sVector30 QuakeSpeed;
protected:
  sSimpleMaterial *WireMtrl;
  sSimpleMaterial *WireMtrlNoZ;
  sGeometry *WireGeo;
  void PaintGrid();

  sRay DragRay;
  int OldMouseHardX;
  int OldMouseHardY;

#if sRENDERER==sRENDER_DX11
  int RTMultiLevel;
  sTexture2D *ColorRT;
  sTexture2D *DepthRT;
#endif

public:
  sCLASSNAME_NONEW(s3DWindow);
  s3DWindow();
  ~s3DWindow();
  void Tag();
  void InitWire(const sChar *name);

  void SetEnable(sBool enable);

  void OnPaint2D();
  void OnPaint3D();

  virtual void SetLight() {}
  virtual void Paint(sViewport &view) {}
  virtual void PaintWire(sViewport &view) {}
  // old style. overload old style or new style, but not both
  virtual void Paint(sViewport &view,const sTargetSpec &spec) {}
  virtual void PaintWire(sViewport &view,const sTargetSpec &spec) {}

  void Lines(sVertexBasic *vp,int linecount,sBool zOn=sTRUE);
  void Circle(const sVector31 &center, const sVector30 &normal, float radius, uint32_t color=0xffffff00, sBool zon=sTRUE, int segs=32);

  sRay MakeRay(int x, int y); // makes ray from position within client window
  void SetCam(const sMatrix34 &mat,float zoom); // move camera to position

  float Zoom;
  sVector31 Focus;
  sMatrix34 Pos;
  float RotX;
  float RotY;
  float RotZ;
  uint32_t GridColor;
  sBool Grid;
  sBool Continuous;
  float GridUnit;

  float SideSpeed,ForeSpeed;     // quakecam speed, default 0.000020f
  float SpeedDamping;
  int GearShift;               // infinite speed settings through mousewheel. from -40 to +40 in sqrt(4) steps
  int GearShiftDisplay;        // display speed factor if(GerShiftDisplay>sGetTime());
  float GearSpeed;               // current gear speed

  sViewport View;
  void PrepareView();

  void SetFocus(const sAABBox &bounds,const sVector31 &center);
  sBool OnKey(uint32_t key);
  void OnDrag(const sWindowDrag &dd);
  void QuakeCam();
  void PrintGear(sPainter *p,int x,int &y);


  void CmdReset();
  void CmdResetTilt();
  void CmdGrid();
  void CmdQuakeCam();
  void CmdScreenshot();
  void CmdQuakeForwToggle(ptrdiff_t);
  void CmdQuakeBackToggle(ptrdiff_t);
  void CmdQuakeLeftToggle(ptrdiff_t);
  void CmdQuakeRightToggle(ptrdiff_t);
  void CmdQuakeUpToggle(ptrdiff_t);
  void CmdQuakeDownToggle(ptrdiff_t);
  void CmdGearShift(ptrdiff_t);
  void DragOrbit(const sWindowDrag &dd);
  void DragRotate(const sWindowDrag &dd);
  void DragMove(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd);
  void DragDolly(const sWindowDrag &dd);
  void DragTilt(const sWindowDrag &dd);
};


/****************************************************************************/

