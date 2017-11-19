/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_FRAMES_HPP
#define FILE_GUI_FRAMES_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/math.hpp"
#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/controls.hpp"

/****************************************************************************/

class sSplitFrame : public sWindow
{
protected:
  struct ChildDataStruct
  {
    int StartPos;
    int RelPos;
    int Pos;
    int Align;
  };
  sArray<ChildDataStruct> ChildData;
  int Initialized;
  int Drag;
  int DragStart;
  int Count;
  int OldT;
  int RelT;
  void MakeChildData();
public:
  sCLASSNAME(sSplitFrame);
  sSplitFrame();
  void SplitLayout(int w);
  int SplitDrag(const sWindowDrag &dd,int mousedelta,int mousepos);
  void Preset(int splitter,int value,sBool align=0);
  void PresetPos(int splitter,int value);
  void PresetAlign(int splitter,sBool align);
  int GetPos(int splitter);
  int Knop;
  sBool Proportional;
};

class sHSplitFrame : public sSplitFrame
{
public:
  sCLASSNAME(sHSplitFrame);
  sHSplitFrame();
//  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

class sVSplitFrame : public sSplitFrame
{
public:
  sCLASSNAME(sVSplitFrame);
  sVSplitFrame();
//  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
};

/****************************************************************************/

class sMenuFrame : public sWindow
{
  struct Item
  {
    sMessage Message;
    uint32_t Shortcut;
    int Column;
    sWindow *Window;
  };
  const static int MaxColumn = 34;
  sArray<Item> Items;
  void CmdPressed(ptrdiff_t);
  void CmdPressedNoKill(ptrdiff_t);
  void Kill();

  int ColumnWidth[MaxColumn];
  int ColumnHeight[MaxColumn];
public:
  sCLASSNAME(sMenuFrame);
  sMenuFrame();
  sMenuFrame(sWindow *sendto);    // add thickborder, make "sendto" focus after menu
  ~sMenuFrame();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  sBool OnShortcut(uint32_t key);
//  sBool OnCommand(int cmd);

  void AddItem(const sChar *name,const sMessage &cmd,uint32_t Shortcut,int len=-1,int column=0,uint32_t backcol=0);
  void AddCheckmark(const sChar *name,const sMessage &cmd,uint32_t Shortcut,int *refptr,int value,int len=-1,int column=0,uint32_t backcol=0,int buttonstyle=0);
  void AddSpacer(int column=0);
  void AddHeader(sPoolString name,int column=0);
  void AddChoices(const sChar *choices,const sMessage &msg);

  sWindow *SendTo;          // only used to set focus
};


void sPopupChoices(const sChar *choices,const sMessage &msg); // make an sMenuFrame with choices from popup

/****************************************************************************/

enum sLayoutFrameWindowMode
{
  sLFWM_WINDOW,
  sLFWM_HORIZONTAL,
  sLFWM_VERTICAL,
  sLFWM_SWITCH,
  sLFWM_BORDER,
  sLFWM_BORDERPRE,
};

struct sLayoutFrameWindow
{
  sLayoutFrameWindowMode Mode;
  int Pos;                       // default position, negative is bottom/left aligned
  int Align;
  int Temp;                      // this is not used. but sWire need it!
  sWindow *Window;                // if sLFWM_WINDOW
  int Switch;                    // if sLFWM_SWITCH
  sPoolString Name;               // optional name, required to switch switch windows
  sArray<sLayoutFrameWindow *> Childs;
  sBool Proportional;

  sMessage OnSwitch;

  sLayoutFrameWindow(sLayoutFrameWindowMode mode,sWindow *window=0,int pos=0);
  ~sLayoutFrameWindow();
  void Add(sLayoutFrameWindow *w);
  void Layout(sWindow *win,class sLayoutFrame *root);
  void Cleanup(sWindow *win,class sLayoutFrame *root);
};

class sLayoutFrame : public sWindow
{
  int CurrentScreen;
  void SetSubSwitchR(sLayoutFrameWindow *p,sPoolString name,int nr);
  void GetSubSwitchR(sLayoutFrameWindow *p,sPoolString name,int &nr);
public:
  sCLASSNAME(sLayoutFrame);
  sLayoutFrame();
  ~sLayoutFrame();
  void Tag();
/*
  void OnCalcSize();
  void OnLayout();
*/
  void Switch(int screen);
  int GetSwitch() { return CurrentScreen; }
  void SetSubSwitch(sPoolString name,int nr);    // switch window deeper in hirarchy
  int GetSubSwitch(sPoolString name);

  sArray<sLayoutFrameWindow *> Screens;
  sArray<sWindow *> Windows;
};

class sSwitchFrame : public sWindow
{
  int CurrentScreen;
public:
  sCLASSNAME(sSwitchFrame);
  sSwitchFrame();
  ~sSwitchFrame();
  void Tag();
  void Switch(int screen);
  int GetSwitch() { return CurrentScreen; }

  sArray<sWindow *> Windows;
};

/****************************************************************************/

enum sGridFrameLayoutFlags
{
  sGFLF_GROUP = 1,                // display label as group. has different layout.
  sGFLF_NARROWGROUP = 2,          // add this to group for a more narrow display
  sGFLF_HALFUP = 4,               // moves the box up by half a gridheight
  sGFLF_CENTER = 8,               // center horizontally
  sGFLF_LEAD = 16,                // draw leading - a line that makes tables to read easier
};

struct sGridFrameLayout           // discribe the content of the grid window!
{
  sRect GridRect;                 // a gridcell
  sWindow *Window;                // is either a window
  const sChar *Label;             // or a static label string
  int Flags;                     // sGFLF
};                                // every child has to be in this array!

class sGridFrame : public sWindow
{
public:
  sCLASSNAME(sGridFrame);
  sGridFrame();
  ~sGridFrame();
  void Tag();

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  sBool OnKey(uint32_t key);

  sArray<sGridFrameLayout> Layout;
  int Columns;
  int Height;

  void Reset();
  void AddGrid(sWindow *,int x,int y,int xs,int ys=1,int flags=0);
  void AddLabel(const sChar *,int x,int y,int xs,int ys=1,int flags=0);
};

struct sGridFrameHelper
{
  // configuration
  sGridFrame *Grid;               // link to grid, should be 12 wide
  int LabelWidth;                // width of a label, usually 3
  int ControlWidth;              // width of average control, usually 2
  int WideWidth;                 // width of wide controls, like strings.
  int BoxWidth;                  // width of a box, usually 1
  sMessage DoneMsg;               // message to add for "done" event
  sMessage ChangeMsg;             // message to add for "change" event
  sBool Static;                   // create controls as static (if possible)
  
  float ScaleRange;
  float RotateRange;
  float TranslateRange;
  float ScaleStep;
  float RotateStep;
  float TranslateStep;

  // private layut counters
  int Line;                      // current line
  int Left;                      // controls are layouted from left to right
  int Right;                     // boxes are layouted from right to left
  int EmptyLine;                 // a new line has already been started.
  int TieMode;                   // 0=off, 1=first, 2=cont
  sStringControl *TiePrev,*TieFirst;

  // general
  sGridFrameHelper(sGridFrame *); // initialize and connect to frame
  ~sGridFrameHelper();            // delete DoneMsg and ChangeMsg
  void Reset();                   // reset frame.
  void NextLine();                // advance to next line
  void InitControl(class sControl *con); // used internally to copy messaging to control
  void BeginTied();               // tie together controls so thay can be dragged together with CTRL
  void EndTied();
  void Tie(sStringControl *);      // tie this control, automatically called for INT, FLOAT and BYTE.
  void SetColumns(int left,int middle,int right);
  void MaxColumns(int left,int middle,int right);

  // special controls
  void Label(const sChar *label); // start new line and add label at the left
  void LabelC(const sChar *label); // label withouzt new line
  void Group(const sChar *label=0); // begin a new group in new line
  void GroupCont(const sChar *label=0); // begin a new group in current line (rarely used)
  void Textline(const sChar *text=0); // one line of text
  class sButtonControl *PushButton(const sChar *label,const sMessage &done,int layoutflags=0);    // add pushbutton, large one from the left
  class sButtonControl *Box(const sChar *label,const sMessage &done,int layoutflags=0);           // add pushbutton, small one from the right
  class sButtonControl *BoxToggle(const sChar *label,int *x,const sMessage &done,int layoutflags=0);           // add pushbutton, small one from the right
  void BoxFileDialog(const sStringDesc &string,const sChar *text,const sChar *ext,int flags=0);

  // value controls
  void Radio(int *val,const sChar *choices=L"off|on",int width=-1);                 // create radiobuttons from left to right. you may overwrite the width
  sChoiceControl *Choice(int *val,const sChar *choices=L"off|on");             // dropdownlist with choices, or toggle if only two choices
  sControl *Flags(int *val,const sChar *choices=L"off|on");                         // build multiple buttons separated by colon: "*0on|off:*1a|b|c|d:*3bli|bla"
  sButtonControl *Toggle(int *val,const sChar *label);                              // create toggle button
  sButtonControl *Button(const sChar *label,const sMessage &msg);                          // create pushbutton
  void Flags(int *val,const sChar *choices,const sMessage &msg);
  sStringControl *String(const sStringDesc &string,int width=0);
  sStringControl *String(sPoolString *pool,int width=0);
  sStringControl *String(sTextBuffer *tb,int width=0);
  class sTextWindow *Text(sTextBuffer *tb,int lines,int width=0);
  sFloatControl *Float(float *val,float min,float max,float step=0.25f,float *colptr=0);
  sIntControl *Int(int *val,int min,int max,float step=0.25f,int *colptr=0,const sChar *format=0);
  sByteControl *Byte(uint8_t *val,int min,int max,float step=0.25f,uint8_t *colptr=0);
  sWordControl *Word(uint16_t *val,int min,int max,float step=0.25f,uint16_t *colptr=0);
  void Color(uint32_t *,const sChar *config);
  void ColorF(float *,const sChar *config);
  void ColorPick(uint32_t *,const sChar *config,sObject *tagref);
  void ColorPickF(float *,const sChar *config,sObject *tagref);
  class sColorGradientControl *Gradient(class sColorGradient *,sBool alpha);
  void Bitmask(uint8_t *x,int width = 1);

  // complex composites

  void Scale(sVector31 &);
  void Rotate(sVector30 &);
  void Translate(sVector31 &);
  void SRT(sSRT &);

  // add your own control

  void Control(sControl *con,int width=-1);
  void Custom(sWindow *con,int width=-1,int height=1);
};


struct sGridFrameTemplate
{
  int Type;                      // sGFT_???
  int Flags;                     // sGFF_???
  const sChar *Label;             // Label 
  const sChar *Choices;           // RADIO,CHOICE,FLAGS: choices ; COLOR,COLORF: rgb or rgba
  int Offset;                    // byte-offset to value 
  int Count;                     // INT,FLOAT,BYTE: number of controls ; STRING: number of chars
  float Min;                       // INT,FLOAT,BYTE: minimum value
  float Max;                       // INT,FLOAT,BYTE: maximum value
  float Step;                      // INT,FLOAT,BYTE: step for dragging
  int ConditionOffset;           // if((obj[offset] & mask) == value)
  int ConditionMask;
  int ConditionValue;
  sMessage Message;               // BOX,PUSHBUTTON: message to send when pressed

  void Init();
  sBool Condition(void *obj_);
  void Add(sGridFrameHelper &gh,void *obj_,const sMessage &changemsg,const sMessage &relayoutmsg);
  sGridFrameTemplate *HideCond(int offset,int mask,int value);
  sGridFrameTemplate *HideCondNot(int offset,int mask,int value);

  // special controls
  sGridFrameTemplate *InitLabel (const sChar *label);
  sGridFrameTemplate *InitGroup (const sChar *label=0);
  sGridFrameTemplate *InitPushButton(const sChar *label,const sMessage &done);
  sGridFrameTemplate *InitBox   (const sChar *label,const sMessage &done);

  // value controls
  sGridFrameTemplate *InitRadio (const sChar *label,int offset,const sChar *choices=L"off|on");
  sGridFrameTemplate *InitChoice(const sChar *label,int offset,const sChar *choices=L"off|on");
  sGridFrameTemplate *InitFlags (const sChar *label,int offset,const sChar *choices=L"off|on");
  sGridFrameTemplate *InitString(const sChar *label,int offset,int count);
  sGridFrameTemplate *InitFloat (const sChar *label,int offset,int count,float min,float max,float step=0.25f);
  sGridFrameTemplate *InitInt   (const sChar *label,int offset,int count,int min,int max,float step=0.25f);
  sGridFrameTemplate *InitByte  (const sChar *label,int offset,int count,int min,int max,float step=0.25f);
  sGridFrameTemplate *InitColor (const sChar *label,int offset,const sChar *config);
  sGridFrameTemplate *InitColorF(const sChar *label,int offset,const sChar *config);
};

enum sGridFrameTemplateFlags
{
  sGFF_NOLABEL      = 0x00000001, // even if label is specified, do not use it
  sGFF_WIDERADIO    = 0x00000002, // use wide radiobuttons
  sGFF_RELAYOUT     = 0x00000004, // redo layout when value changes      
  
  sGFF_CONDGRAY     = 0x00000100, // gray out when condition is met
  sGFF_CONDHIDE     = 0x00000200, // hide when condition is met
  sGFF_CONDNEGATE   = 0x00000800, // negate condition
  sGFF_USEDFLAGS    = 0xffff0000,
};

enum sGridFrameTemplateTypes
{
  sGFT_NOP = 0,
  sGFT_LABEL,
  sGFT_GROUP,
  sGFT_PUSHBUTTON,
  sGFT_BOX,

  sGFT_RADIO,
  sGFT_CHOICE,
  sGFT_FLAGS,
  sGFT_STRING,
  sGFT_INT,
  sGFT_FLOAT,
  sGFT_BYTE,
  sGFT_COLOR,
  sGFT_COLORF,

  sGFT_USER,
};

/****************************************************************************/

#endif
