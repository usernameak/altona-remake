/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/wire.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Text Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sTextWindow : public sWindow
{
  sPrintInfo PrintInfo;
  int MarkMode;
  int MarkBegin;
  int MarkEnd;
  int WindowHeight;
  int DragCursorStart;
  int DisableDrag;
  int GoodCursorX;
  class sFont2D *Font;
  sRect TextRect;

  sTextBuffer *Text;              // textbuffer to edit

  void GetCursorPos(int &x,int &y);
  int FindCursorPos(int x,int y);
  sBool BeginMoveCursor(sBool selmode);
  void EndMoveCursor(sBool selmode);
  void Delete(int pos,int len);
  void Insert(int pos,const sChar *s,int len=-1);
  void Mark(int start,int end);
  void MarkOff();

  int CursorTimer;
  void CmdCursorToggle();

  sMessageTimer *Timer;

  struct UndoStep
  {
    sBool Delete;
    int Pos;
    int Count;
    sChar *Text;
  };
  UndoStep *UndoBuffer;
  int UndoAlloc;
  int UndoValid;
  int UndoIndex;
  sString<256> UndoCollector;
  int UndoCollectorIndex;
  int UndoCollectorValid;
  int UndoCollectorStart;

  UndoStep *UndoGetStep();
  void UndoFlushCollector();
  void UndoInsert(int pos,const sChar *string,int len);
  void UndoDelete(int pos,int size);
  void Undo();
  void Redo();

public:
  sCLASSNAME(sTextWindow);
  sTextWindow();
  ~sTextWindow();
  void InitCursorFlash();
  void Tag();
  void SetText(sTextBuffer *);
  sTextBuffer *GetText() { return Text; }
  sBool HasSelection() { return MarkMode!=0; }

  void OnPaint2D();
  sBool OnKey(uint32_t key);
  void OnDrag(const sWindowDrag &dd);
  void OnCalcSize();

  void ResetCursorFlash();
  sBool GetCursorFlash();

  sObject *TextTag;               // this tag will be hold should be object in which the textbuffer is embedded

  int TextFlags;                 // sF2P_???
  int EditFlags;                 // sTEF_???
  int EnterCmd;                  // post this when enter is pressed
  int BackColor;                 // sGC_???
  int HintLine;                  // fine vertical line that hints the right border of the page
  uint32_t HintLineColor;             // sGC_???
  int TabSize;                   // default 8. currently, TAB insert spaces
  sMessage EnterMsg;
  sMessage ChangeMsg;
  sMessage CursorMsg;             // when cursor changes
  sMessage CursorFlashMsg;        // synchronise to cursor :-)
  sBool ShowCursorAlways;         // usually the cursor is hidden when focus is lost

  void DeleteChar();
  void DeleteBlock();
  void InsertChar(int c);
  void InsertString(const sChar *s);
  void IndentBlock(int indent);
  void OverwriteChar(int c);
  void TextChanged();             // react to the fact that the text changed.
  void SetCursor(int p);
  void SetCursor(const sChar *p);
  void SetFont(sFont2D *newFont);
  void ScrollToCursor();
  sFont2D *GetFont();
  int GetCursorPos() { return PrintInfo.CursorPos; }
  int GetCursorColumn() const;
  void Find(const sChar *string,sBool dir,sBool next);
  void GetMark(int &start,int &end);

  void UndoClear();
  void UndoGetStats(int &undos,int &redos);

};

enum sTextEditFlags
{
//  sTEF_EDIT       = 0x0001,
  sTEF_MARK       = 0x0002,
  sTEF_STATIC     = 0x0004,
  sTEF_LINENUMBER = 0x0008,
};

class sWireTextWindow : public sTextWindow
{
public:
  void InitWire(const sChar *name) { sWire->AddWindow(name,this); }
  sBool OnKey(uint32_t key) { if(sWire->HandleKey(this,key)) return 1; else return sTextWindow::OnKey(key); }
  sBool OnShortcut(uint32_t key) { return sWire->HandleShortcut(this,key); }
};

/****************************************************************************/
