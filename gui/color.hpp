/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "base/types.hpp"
#include "base/math.hpp"
#include "gui/window.hpp"
#include "gui/controls.hpp"
#include "util/image.hpp"

/****************************************************************************/

struct sColorGradientKey
{
  float Time;
  sVector4 Color;
};

class sColorGradient : public sObject
{
public:
  sArray<sColorGradientKey> Keys;
  float Gamma;
  int Flags;

  void AddKey(float time,uint32_t col);
  void AddKey(float time,const sVector4 &col);
  void Sort();
  void CalcUnwarped(float time,sVector4 &col);
  void Calc(float time,sVector4 &col);

  sCLASSNAME(sColorGradient);
  sColorGradient();
  ~sColorGradient();
  void Clear();

  template <class streamer> void Serialize_(streamer &);
  void Serialize(sReader &stream);
  void Serialize(sWriter &stream);
};


class sColorGradientControl : public sControl
{
  sImage *Bar;
  int AlphaEnable;
public:
  sCLASSNAME_NONEW(sColorGradientControl);
  sColorGradientControl(sColorGradient *,sBool alpha);
  ~sColorGradientControl();
  void Tag();

  sColorGradient *Gradient;
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

class sColorPickerWindow : public sWindow
{
  class sGridFrame *Grid;

  float *FPtr;
  uint8_t *UPtr;

  float Red;
  float Green;
  float Blue;
  float Alpha;

  float Hue;
  float Sat;
  float Value;


  sRect BarRect;
  sRect PickRect;
  sRect WarpRect;
  sRect PaletteRect;

  int DragMode;
  sObject *TagRef;
  sBool AlphaEnable;
  sColorGradient *Gradient;
  int DragKey;
  float DragStart;

  sImage *PickImage;
  float PickSat;
  float PickHue;
  sImage *GradientImage;
  sImage *WarpImage;
  sBool GradientChanged;

  sRect PaletteBoxes[32];

  void MakeGui();
  void SelectKey(int nr);
public:
  sCLASSNAME(sColorPickerWindow);
  sColorPickerWindow();
  ~sColorPickerWindow();
  void Tag();

  void Init(float *,sObject *tagref,sBool alpha);
  void Init(uint32_t *,sObject *tagref,sBool alpha);
  void Init(sColorGradient *grad,sBool alpha);
  void ChangeGradient();
  void ChangeRGB();
  void ChangeHSV();
  void UpdateOutput();

  void OnPaint2D();
  void OnCalcSize();
  void OnLayout();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(uint32_t key);

  void CmdDelete();

  sMessage ChangeMsg;

  static float PaletteColors[32][4];
};

/****************************************************************************/

