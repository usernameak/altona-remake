/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_CONTROLS_HPP
#define FILE_GUI_CONTROLS_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "gui/window.hpp"
#include "gui/manager.hpp"

/****************************************************************************/

class sControl : public sWindow
{
public:
  sControl();
  ~sControl();
  void Tag();

  int Style;
  uint32_t BackColor;
  sMessage DoneMsg;
  sMessage ChangeMsg;

  void SendChange();
  void SendDone();
  void PostChange();
  void PostDone();
};

/****************************************************************************/

class sButtonControl : public sControl
{
  int Pressed; 
  int RadioValue;                // set these two to implement radio button highlighting
  int *RadioPtr;                 // local, so you can't foret to update notify.
public:
  sCLASSNAME(sButtonControl);
  sButtonControl(const sChar *,const sMessage &msg,int style=0);
  sButtonControl();
  void InitRadio(int *ptr,int value);
  void InitToggle(int *ptr);
  void InitCheckmark(int *ptr,int val);
  void InitReadOnly(int *ptr,int val=1);
  const sChar *Label;
  int LabelLength;               // length of label string, set to -1
  int Width;                     // usually, the width is calculated automatic. set this != 0 to override!
  uint32_t Shortcut;                  // shortcutkey, will be displayed inside button. this is just for display in sBCS_NOBORDER mode
  sMessage DoubleClickMsg;

  int GetRadioValue() { return RadioValue; }

  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(uint32_t key);
  sBool OnCommand(int cmd);

  static void MakeShortcut(sString<64> &buffer,uint32_t shortcut);    // this is awfully useful and belongs to gui.hpp or types.hpp
};

enum sButtonControlStyle
{
  sBCS_NOBORDER   = 0x0001,       // don't draw any border
  sBCS_STATIC     = 0x0002,       // disable interaction
  sBCS_IMMEDIATE  = 0x0004,       // react on ButtonDown, not ButtonUp
  sBCS_LABEL      = 0x0008,       // no interaction
  sBCS_RADIO      = 0x0010,       // when pressed, actually set the *radioptr=radioval
  sBCS_TOGGLE     = 0x0020,       // when pressed, toggle *radioptr. radioval should be 1
  sBCS_CHECKMARK  = 0x0040,       // add a checkmark when *radioptr==radioval
  sBCS_READONLY   = 0x0080,       // just display *radioptr==radioval, don't change value when clicked
  sBCS_TOGGLEBIT  = 0x0100,       // when pressed, eor *radioptr with radioval, toggling a bit
  sBCS_SHOWSELECT = 0x0200,       // just display in pressed-down state. 
};

/****************************************************************************/

class sFileDialogControl : public sButtonControl
{
  void CmdStart();
  sStringDesc String;
  const sChar *Text;
  const sChar *Extensions;
  int Flags;
public:
  sFileDialogControl(const sStringDesc &string,const sChar *text,const sChar *ext,int flags=0);
  ~sFileDialogControl();

  sMessage OkMsg;
};

//  sSOF_LOAD = 1,
//  sSOF_SAVE = 2,
//  sSOF_DIR = 3,

/****************************************************************************/

class sChoiceControl : public sControl
{
//  int *Value;                    // ptr to value
public:
  struct ChoiceInfo
  {
    const sChar *Label;           // display string
    int Length;                  // string length
    int Value;                   // value to set, not shifted by ValueShift.
  };
  struct ChoiceMulti
  {
    int *Ptr;
    int Default;
  };
  enum ChoicesStyles
  {
    sCBS_CYCLE     = 0,           // cycle between choices
    sCBS_DROPDOWN  = 1,           // open a dropdown list 
  };
  int ValueMask;                 // mask of bits that may be affected
  int ValueShift;                // shift by this amount
  int Width;                     // usually, the width is calculated automatic. set this != 0 to override!
  sArray<ChoiceInfo> Choices;     // created by InitChoices();
  int Pressed;                   // visual feedback

  sCLASSNAME(sChoiceControl);
  sChoiceControl(const sChar *choices,int *val);
  sChoiceControl();
  void InitChoices(const sChar *&,int *val);  // parse the choice string. "*2x|y|3z" -> 3 values: 0:"x", 4:"y", 12:"z". s will be advanced, so that it points to after the string. for flags op.
  void AddMultiChoice(int *val); // same choice for multiple data: set *val for initchoices to 0
  template <class Type> void AddMultiChoice(sArray<Type *> &list,int Type::*offset) { Type *t; sFORALL(list,t) AddMultiChoice(&(t->*offset)); }


  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(uint32_t key);
  sBool OnCommand(int cmd);
  void FakeDropdown(int x,int y);
private:
  void Next();
  void Dropdown();
  void SetValue(ptrdiff_t newval);
  void SetDefaultValue();
  sArray<ChoiceMulti> Values;
};

/****************************************************************************/

enum sStringControlStyle
{
  sSCS_BACKCOLOR = 0x0001,
  sSCS_STATIC = 0x0002,
  sSCS_BACKBAR = 0x0004,
  sSCS_FIXEDWIDTH = 8,
};

// InitString needs some cleanup
// need multiline for Textbuffers

class sStringControl : public sControl
{
  sPrintInfo PrintInfo;
  int MarkMode;
  int MarkBegin;
  int MarkEnd;
  void DeleteMark();
  void InsertChar(int c);
  void ResetCursor();
  sChar *Buffer;
  int Size;
  sPoolString *PoolPtr;
  sTextBuffer *TextBuffer;

  const sChar *GetText();
protected:
  void SetCharPos(int x);
  sBool Changed;
public:
  sCLASSNAME(sStringControl);
  sStringControl(const sStringDesc &);
  sStringControl(int size,sChar *buffer);
  sStringControl(sPoolString *pool);
  sStringControl(sTextBuffer *tb);
  sStringControl();
  ~sStringControl();
  void Tag();
  void InitString(const sStringDesc &desc);
  void InitString(sPoolString *pool);
  void InitString(sTextBuffer *tb);

  virtual sBool MakeBuffer(sBool unconditional);   // if buffer needs to be updated, do that and return 1
  virtual sBool ParseBuffer();  // if buffer is valid, update value and return 1

  uint32_t BackColor;
  float Percent;

  void OnCalcSize();
  void OnPaint2D();
  sBool OnKey(uint32_t key);
  void OnDrag(const sWindowDrag &dd);
  sBool OnCommand(int cmd);

  sStringDesc GetBuffer();
  sStringControl *DragTogether;      // only used for ValueControl
};

/****************************************************************************/

template <typename Type> 
class sValueControl : public sStringControl
{
  sString<64> String;
  Type *ColorPtr;
public:
  Type *Value;
  Type Min;
  Type Max;
  Type OldValue;
  Type Default;
  float Step;
  float RightStep;
  Type DragStart;
  const sChar *Format;
  int DisplayShift;      // for int only: shift before display
  int DisplayMask;       // for int only: mask before display

  // CLASSNAME..
  static const sChar *ClassName();
  void MoreInit();
  const sChar *GetClassName() { return ClassName(); }
  void InitValue(Type *val,Type min,Type max,float step=0.0f,Type *colptr=0)
  {
    Min = min;  Max = max;  Value = val; Step = step;
    if(min<=0 && max>=0)
      Default = 0;
    else
      Default = min;
    OldValue = *Value;
    ColorPtr = colptr;
    ClearNotify();
    DisplayShift = 0;
    DisplayMask = ~0;
    DragTogether = 0;
    RightStep = 0.125f;
    AddNotify(Value,sizeof(Type));
    AddNotify(ColorPtr,sizeof(Type)*3);
    MoreInit();
  }
  sValueControl(Type *val,Type min,Type max,float step=0.0f,Type *colptr=0)
  {
    InitString(String); InitValue(val,min,max,step,colptr);
    Format=L"%f"; MakeBuffer(1);
  }
  sValueControl()
  {
    InitString(String); InitValue(0,0,0,0,0);
    Format=L"%f"; MakeBuffer(1);
  }
  void OnCalcSize()
  {
    ReqSizeY = sGui->PropFont->GetHeight()+4;
  }
  sBool OnKey(uint32_t key)
  {
    if(Style & sSCS_STATIC) return 0;
    sStringControl *tie;
    switch(key&(sKEYQ_BREAK|sKEYQ_MASK))
    {
    case sKEY_HOME:
      *Value = Default; if(MakeBuffer(0)) Update();
      tie = DragTogether;
      if((key&sKEYQ_CTRL) && tie)
      {
        while(tie!=this)
        { tie->OnKey(sKEY_HOME); tie = tie->DragTogether; }
      }
      PostChange();
      sGui->Notify(Value,sizeof(Type));
      PostDone();
      return 1;
    default:
      return sStringControl::OnKey(key);
    }
  }

  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd)
  {
    if(Style & sSCS_STATIC) return;
    if(Step!=0)
    {
      float step = (dd.Buttons&2)?RightStep:Step; 
      switch(dd.Mode)
      {
      case sDD_START:   
        SetCharPos(dd.StartX-Inner.x0);
        if(dd.Flags&sDDF_DOUBLECLICK)
          OnKey(sKEY_HOME|(sGetKeyQualifier()&sKEYQ_CTRL));
        DragStart = *Value;
        break;
      case sDD_DRAG:
        if(dd.DeltaX)
          SetCharPos(Inner.SizeX());
        else
          SetCharPos(dd.StartX-Inner.x0);
        if(!(dd.Flags&sDDF_DOUBLECLICK))
        { double val = DragStart;
          if(step>=0)
            val = val + dd.HardDeltaX/2*step;
          else
            val = (val==0) ? 0.0001f : sExp(sLog(float(val))-dd.HardDeltaX/2*step);
          *Value = Type(sClamp<double>(val,Min,Max));
          if(MakeBuffer(0)) { Update(); PostChange(); Changed=1; sGui->Notify(Value,sizeof(Type)); }
        } 
        if((sGetKeyQualifier()&sKEYQ_CTRL))
          OnKey('a'|sKEYQ_CTRL);
        break;
      case sDD_STOP: 
        PostChange();
        PostDone(); 
        break;
      }
    }
    if(!(dd.Buttons&0x8000))
    {
      sStringControl *tie = DragTogether;
      sWindowDrag dd2 = dd;
      dd2.Buttons |= 0x8000;
      if((sGetKeyQualifier()&sKEYQ_CTRL) && tie)
      {
        while(tie!=this)
        { tie->OnDrag(dd2); tie = tie->DragTogether; }
      }
    }
  }

  sBool MakeBuffer(sBool unconditional);
  sBool ParseBuffer();
};

typedef sValueControl<uint8_t> sByteControl;
typedef sValueControl<uint16_t> sWordControl;
typedef sValueControl<int> sIntControl;
typedef sValueControl<float> sFloatControl;

/****************************************************************************/

class sFlagControl : public sControl
{
  sBool Toggle;
  int GetValue();
  int CountChoices();
  void SetValue(int n);
  void NextChoice(int delta);
  void MakeChoice(int n,const sStringDesc &desc);

public:
  sCLASSNAME(sFlagControl);
  sFlagControl();
  sFlagControl(int *val,const sChar *choices);
  sFlagControl(int *val,const sChar *choices,int mask,int shift);

  int *Val;
  int Max;
  int Mask;
  int Shift;
  const sChar *Choices;

  void OnCalcSize();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/

class sProgressBarControl : public sControl
{
  float Percentage;

public:
  sCLASSNAME(sProgressBarControl);
  sProgressBarControl();

  void SetPercentage(float percentage);

  void OnCalcSize();
  void OnPaint2D();
};

/****************************************************************************/

class sBitmaskControl : public sControl
{
  int X[9];
  uint8_t *Val;
public:
  sCLASSNAME(sBitmaskControl);
  sBitmaskControl();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);

  void Init(uint8_t *val);
};

/****************************************************************************/

#endif
