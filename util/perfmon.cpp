/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "perfmon.hpp"

#if sPERFMON_ENABLED

#include "base/system.hpp"
//#include "base/graphics.hpp"
//#include "shaders.hpp"
#include "util/painter.hpp"

/****************************************************************************/

namespace sPerfMon
{
  static const int MAX_NESTING=8;

  static sBool Inited=sFALSE;

  static int MaxThreads;
  static int MaxEvents;
  static int MaxValues;
  static int MaxSwitches;
  static sBool Active;
  static sBool Show=sFALSE;
  static uint64_t  StartTime;
  static uint64_t  MaxTime=1000000/30;
  static uint64_t  FrameTime=1000000/60;

  static int NumThreads;
  static sThreadContext **ThreadList;

  enum
  {
    SHOW_MIN=-2,

    SHOW_SWITCHES=-2,
    SHOW_OFF=-1,
  };
  static int  ShowDetails=SHOW_OFF;
  static int  CurSwitch=-1;

  static sPainter *Painter;

  static sThreadLock *Lock;

  /****************************************************************************/

  struct Value
  {
    const sChar *Name;
    const int *Ptr;
    int Group;
    int Min;
    int Max;
  } *Values;
  int *ValueMap;
  int NumValues;
  sBool ValuesSorted;

  /****************************************************************************/

  struct Switch
  {
    const sChar *Name;
    const sChar *Choice;
    int *Ptr;
  } *Switches;
  int NumSwitches;

  /****************************************************************************/

  struct ThreadEvent
  {
    uint32_t  Color;
    const sChar *Name;
    const sChar8 *Name8;

    bool operator == (const ThreadEvent &b) const { return !sCmpMem(this,&b,sizeof(ThreadEvent)); }
  };

  struct Event
  {
    uint64_t  Time;
    int  Level;
    ThreadEvent Ev;
  };


  struct Thread
  {
    Event *Events;
    int NumEvents;

    int Level;
    ThreadEvent Stack[MAX_NESTING];

    int MuteCount;
  };

  uint8_t  *Memory;
 
  /****************************************************************************/

  static uint8_t *Alloc(int size)
  {
    uint8_t *ptr=0;

    sPushMemLeakDesc(L"PerfMon");

    if (sIsMemTypeAvailable(sAMF_DEBUG))
      ptr = (uint8_t*)sAllocMem(size,4,sAMF_DEBUG);

    if (!ptr) ptr=(uint8_t*)sAllocMem(size,4,0);

    sPopMemLeakDesc();

    return ptr;
  }

  /****************************************************************************/

  void BeginFrame(void *user=0);
  void EndFrame(void *user);

  void PreInit()
  {
    int tlmem=MaxThreads*sizeof(sThreadContext*);
    int vmapmem=MaxValues*sizeof(int);
    int valuemem=MaxValues*sizeof(Value);
    int switchmem=MaxSwitches*sizeof(Switch);
    int memneeded=tlmem+vmapmem+valuemem+switchmem;

    Memory=Alloc(memneeded);
    if (!Memory)
    {
      sLogF(L"perf",L"PERFMON DISABLED (no memory found)\n");
      return;
    }

    sSetMem(Memory,0,memneeded);
    uint8_t *mem=Memory;

    NumThreads=0;
    ThreadList= (sThreadContext**)mem; mem+=tlmem;

    NumValues=0;
    ValuesSorted=0;
    ValueMap = (int*)mem; mem+=vmapmem;
    Values = (Value*)mem; mem+=valuemem;

    NumSwitches=0;
    Switches = (Switch*)mem; mem+=switchmem;

    Lock = new sThreadLock;

    Inited=sTRUE;
    sPerfAddThread();

  }

  void Init()
  {
    if (Inited)
    {
      Active=sFALSE;

      Painter = new sPainter;

      sPostFlipHook->Add(BeginFrame);
      sPreFlipHook->Add(EndFrame);

      BeginFrame();
    }
  }

  void Exit()
  {
    if (Inited)
    {
      Inited=sFALSE;

      sPreFlipHook->Rem(EndFrame);
      sPostFlipHook->Rem(BeginFrame);

      sDelete(Painter);
      sDelete(Lock);

      for (int i=0; i<NumThreads; i++)
      {
        Thread *t=(Thread*)ThreadList[i]->PerfData;
        delete[] t->Events;
        delete t;
        ThreadList[i]->PerfData=0;
      }

      sFreeMem(Memory);
      Memory=0;
    }
  }

  // init timing stuff
  void BeginFrame(void *user)
  {
    Lock->Lock();
    for (int i=0; i<NumThreads; i++)
    {
      sThreadContext *tc=ThreadList[i];
      Thread *t=(Thread*)(tc->PerfData);
      if (!t) continue;

      t->NumEvents=1;
      Event &e=t->Events[0];
      e.Time=0;
      e.Level=t->Level;
      if (t->Level>0)
      {
        ThreadEvent &te=t->Stack[t->Level-1];
        e.Ev.Color=te.Color;
        e.Ev.Name=te.Name;
        e.Ev.Name8=te.Name8;         
      }
      else
      {
        e.Ev.Color=0x404040;
        e.Ev.Name=L"???";
        e.Ev.Name8=0;
      }
    }
    StartTime=sGetTimeUS();
    if (Show) Active=sTRUE;
    Lock->Unlock();
  }

  // draw timing stuff
  void EndFrame(void *user)
  {
    if (!Active) return;
    Lock->Lock();

    Active=sFALSE;
    uint64_t time = sGetTimeUS()-StartTime;

    // add end to all threads
    for (int i=0; i<NumThreads; i++)
    {
      Thread *t=(Thread*)ThreadList[i]->PerfData;
      const sChar *tname=ThreadList[i]->ThreadName;
      
      if (t->NumEvents==MaxEvents) 
      {
        sLogF(L"perf",L"warning: events exceeded for thread '%s'\n",tname);
        t->NumEvents--;
      }
      Event &e=t->Events[t->NumEvents++];
      e.Time=time;
      e.Level=t->Level-1;
      e.Ev.Color=0;
      e.Ev.Name=0;
      e.Ev.Name8=0;
    }

    sSetTarget(sTargetPara(sST_CLEARNONE,0,0));

    int sx, sy;
    float asp, sax, say;

    sGetScreenSize(sx,sy);
    sGetScreenSafeArea(sax,say);
    asp=sGetScreenAspect();

    Painter->SetTarget(sRect(0,0,sx,sy));
    Painter->Begin();

    float height=20;
    float theight=Painter->GetHeight(0);
    float yto=(height-theight)/2;
    
    float y=height*NumThreads+(height/2)*(NumThreads+1);
    y=sy/2+(sy*say/2)-y;
    float barsy=y;

    Painter->Box(sFRect(0,y,sx,sy),0x40000000);

    float left=(sx-sx*sax)/2;
    float right=(sx+sx*sax)/2;
    y+=height/2;

    float tw=0;
    for (int i=0; i<NumThreads; i++)
      tw=sMax(tw,Painter->GetWidth(0,ThreadList[i]->ThreadName));
    tw+=10;
     
    for (int i=NumThreads-1; i>=0; i--)
    {
      Thread *t=(Thread*)ThreadList[i]->PerfData;
      const sChar *tname=ThreadList[i]->ThreadName;

      Painter->Print(0,left,y+yto,0xffffffff,tname);
      sFRect r(left+tw,y,right,y+height);
      Painter->Box(r,0xffffffff);
      r.Extend(-1);
      Painter->Box(r,0xff000000);

      uint64_t ft=FrameTime;
      while (ft<MaxTime)
      {
        float x=r.SizeX()*ft/MaxTime+left+tw;
        Painter->Box(x,r.y0,x+1,r.y1,0xffffffc0);
        ft+=FrameTime;
      }

      r.Extend(-1);

      uint64_t rxs=(uint64_t)r.SizeX();
      for (int i=0; i<t->NumEvents-1; i++)
      {
        Event &e1=t->Events[i];
        Event &e2=t->Events[i+1];
        int x1=int(rxs*e1.Time/MaxTime+r.x0);
        int x2=int(rxs*e2.Time/MaxTime+r.x0);
        Painter->Box(x1,r.y0,x2,r.y1,e1.Ev.Color|0xff000000);
      }

      y+=height+height/2;
    }

    if (ShowDetails>=0)
    {
      static const int MAXLINES=20;

      Thread *t=(Thread*)ThreadList[ShowDetails]->PerfData;
      const sChar *tname=ThreadList[ShowDetails]->ThreadName;

      sString<256> name;
      ThreadEvent *LineEv[MAXLINES];
      uint64_t LineTime[MAXLINES];
      uint32_t LineCount[MAXLINES];
      int nLines=0;
      
      float basey=barsy-theight*1.5f;

      tw=0;
      for (int i=0; i<t->NumEvents-1; i++)
      {
        Event &e=t->Events[i];
        if (e.Ev.Color)
        {
          if (e.Ev.Name)
            tw=sMax(tw,Painter->GetWidth(0,e.Ev.Name));
          else if (e.Ev.Name8)
          {
            sCopyString(name,e.Ev.Name8);
            tw=sMax(tw,Painter->GetWidth(0,name));
          }       
        }
      }
      tw+=10;

//      uint64_t rxs=r.SizeX();
      float r2=right-Painter->GetWidth(0,L" (1234) 12345");
      uint64_t rxs=(uint64_t)(r2-(left+tw));
      for (int i=0; i<t->NumEvents-1; i++)
      {
        Event &e1=t->Events[i];

        if (e1.Ev.Color && (e1.Ev.Name || e1.Ev.Name8))
        {
          int line=0;
          for (; line<nLines; line++)
            if (e1.Ev==*LineEv[line])
              break;

          if (line==nLines)
          {
            if (line==MAXLINES) 
            {
              sLogF(L"perf",L"warning: number of detail lines exceeded\n");
              break;
            }
            LineEv[nLines]=&e1.Ev;
            LineTime[nLines]=0;
            LineCount[nLines]=0;

            float y=basey-line*1.5f*theight;
            Painter->Box(0,y,sx,y+theight*1.5f,0x40000000);
            Painter->Box(left+tw,y,r2,y+theight,0xff000000);
            if (e1.Ev.Name)
              Painter->Print(0,left,y,0xffffffff,e1.Ev.Name);
            else if (e1.Ev.Name8)
            {
              sCopyString(name,e1.Ev.Name8);
              Painter->Print(0,left,y,0xffffffff,name);
            }

            nLines++;
          }

          Event *e2=0L;
          for (int j=i+1; j<t->NumEvents; j++)
          {
            e2=&t->Events[j];
            if (e2->Level<=e1.Level || (e1.Ev.Name && !sCmpString(e1.Ev.Name,L"???")))
              break;
          }

          LineTime[line]+=e2->Time-e1.Time;
          if (!i || e1.Level>=t->Events[i-1].Level) LineCount[line]++;
          
          y=basey-line*1.5*theight;
          int x1=int(rxs*e1.Time/MaxTime+left+tw);
          int x2=int(rxs*e2->Time/MaxTime+left+tw);
          Painter->Box(x1,y,x2,y+theight,e1.Ev.Color|0xff000000);
        }
      }

      for (int i=0; i<nLines; i++)
      {
        y=basey-i*1.5*theight;
        name.PrintF(L" (%4d)%6d",LineCount[i],LineTime[i]);
        Painter->Print(0,r2,y,0xffffffff,name);
      }

      y=basey-nLines*1.5*theight;
      Painter->Box(0,y-theight/2,sx,y+theight*1.5f,0x40000000);
      name.PrintF(L"details for thread '%s'",tname);
      Painter->Print(0,left,y,0xffffffff,name);
    }
    else if (ShowDetails==SHOW_SWITCHES)
    {
      //static const int MAXLINES=20;
      float basey=barsy;

      float tw=0;
      for (int i=0; i<NumSwitches; i++)
        tw=sMax(tw,Painter->GetWidth(0,Switches[i].Name));
      tw+=30;

      float y=basey-1.5f*theight;
      for (int i=0; i<NumSwitches; i++)
      {
        Switch &s=Switches[i];

        uint32_t c1=0x40000000;
        uint32_t c2=0xffc0c0c0;
        if (i==CurSwitch)
        {
          c1=0x40808080;
          c2=0xffffffff;
        }

        Painter->Box(0,y,sx,y+theight,c1);
        Painter->Box(0,y+theight,sx,y+theight*1.5f,0x40000000);
        Painter->Print(0,left,y,c2,s.Name);

        sString<128> line;
        sString<64> ch;

        int nc=sCountChoice(s.Choice);
        for (int j=0; j<nc; j++)
        {
          sMakeChoice(ch,s.Choice,j);
          if (j==*s.Ptr)
            sSPrintF(sGetAppendDesc(line),L">%s<",ch);
          else
            sSPrintF(sGetAppendDesc(line),L" %s ",ch);
        }
        Painter->Print(0,left+tw,y,c2,line);

        
        y-=1.5f*theight;
      }

      Painter->Box(0,y-theight/2,sx,y+theight*1.5f,0x40000000);
      Painter->Print(0,left,y,0xffffffff,L"Switches:");

    }

    // paint values
    if (!ValuesSorted)
    {
      for (int i=0; i<NumValues; i++) ValueMap[i]=i;
      for (int i=0; i<NumValues; i++) for (int j=i+1; j<NumValues; j++)
        if (Values[ValueMap[i]].Group>Values[ValueMap[j]].Group)
          sSwap(ValueMap[i],ValueMap[j]);
      ValuesSorted=sTRUE;
    }

    float maxy=(sy-sy*say)/2;
    int curgroup=0;
    height=18;
    float width=110;
    float x=sx;
    float groupy=0;
    y=sy;
    for (int i=0; i<NumValues; i++)
    {
      Value &v=Values[ValueMap[i]];
      y+=height+height/2;
      if (v.Group!=curgroup || y+height>=barsy)
      {
        curgroup=v.Group;
        x+=width+height;
        if ((x+width) >= ((sx+sx*sax)/2))
        {
          x=(sx-sx*sax)/2;
          y=groupy=maxy;
        }
        else
          y=groupy;
      }
      sFRect r(x,y,x+width,y+height);
      Painter->Box(r,0xffffffff);
      r.Extend(-1);
      Painter->Box(r,0xff000000);
      r.Extend(-1);
      int range=v.Max-v.Min;
      int val=*v.Ptr;
      if (val<v.Min)
      {
        float w=sMin((v.Min-val)*(width-4)/float(range),width-4);
        Painter->Box(r.x1-w,r.y0,r.x1,r.y1,0xff802060);
      }
      else if (val<v.Max)
      {
        float w=(val-v.Min)*(width-4)/float(range);
        Painter->Box(r.x0,r.y0,r.x0+w,r.y1,0xff408040);
      }
      else
      {
        Painter->Box(r,0xff408040);
        float w=sMin((val-v.Max)*(width-4)/float(range),width-4);
        Painter->Box(r.x0,r.y0,r.x0+w,r.y1,0xffa02020);
      }
      float ty=r.y0+((height-2)-theight)/2;
      Painter->Print(0,r.x0+1,ty,0xffffffff,v.Name);
      
      sString<256> vstr;
      vstr.PrintF(L"%K",val);
      Painter->Print(0,r.x1-1-Painter->GetWidth(0,vstr),ty,0xffffffff,vstr);
    }

    Painter->End();

    Lock->Unlock();

  }

};

using namespace sPerfMon;

/****************************************************************************/
/***                                                                      ***/
/*** Analysis and Drawing                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sShowPerfMon(sBool show) { if (!show) CurSwitch=-1; sPerfMon::Show=show; }

void sTogglePerfMon() { Show=!Show; CurSwitch=-1; }
sBool sPerfMonInited() { return Inited; }

static const uint32_t PerfMonLeft = 0; 
static const uint32_t PerfMonRight = 0;
static const uint32_t PerfMonUp = 0;
static const uint32_t PerfMonDown = 0;

sBool sSendPerfMonInput(const sInput2Event &ie)
{
  if (!Inited || !Show) return sFALSE;

  sScopeLock lock(Lock);

  if (ie.Key==PerfMonLeft || ie.Key==sKEY_LEFT)
  {
    if (CurSwitch<0)
    {
      if (ShowDetails>SHOW_MIN) ShowDetails--; 
    }
    else
    {
      Switch &s=Switches[CurSwitch];
      int nc=sCountChoice(s.Choice);
      *s.Ptr=sClamp(*s.Ptr-1,0,nc-1);
    }
    return sTRUE;
  }
  else if (ie.Key==PerfMonRight || ie.Key==sKEY_RIGHT)
  {
    if (CurSwitch<0)
    {
      if (ShowDetails<NumThreads-1) ShowDetails++; 
    }
    else
    {
      Switch &s=Switches[CurSwitch];
      int nc=sCountChoice(s.Choice);
      *s.Ptr=sClamp(*s.Ptr+1,0,nc-1);
    }
    return sTRUE;
  }
  else if (ie.Key==PerfMonUp || ie.Key==sKEY_UP)
  {
    if (ShowDetails==SHOW_SWITCHES)
    {
      if (CurSwitch<NumSwitches-1) CurSwitch++;
    }
    return sTRUE;
  }
  else if (ie.Key==PerfMonDown || ie.Key==sKEY_DOWN)
  {
    if (ShowDetails==SHOW_SWITCHES)
    {
      if (CurSwitch>-1) CurSwitch--;
    }
    return sTRUE;
  }

  return sFALSE;
}

void sSetPerfMonTimes(float maxtime, float frametime) 
{ 
  MaxTime=uint64_t(maxtime*1000000.0f); 
  FrameTime=uint64_t(frametime*1000000.0f); 
}

/****************************************************************************/
/***                                                                      ***/
/*** Thread handling                                                      ***/
/***                                                                      ***/
/****************************************************************************/

// main thread is added automatically
void sPerfAddThread(sThreadContext *ctx)
{
  if (!Inited) return;

  if (!ctx) ctx=sGetThreadContext();

  sVERIFY(!ctx->PerfData);
  sVERIFY(NumThreads<MaxThreads);

  Thread *t=(Thread*)Alloc(sizeof(Thread));
  if (!t) return;

  t->Level=0;
  t->NumEvents=0;
  t->Events = (Event*)Alloc(MaxEvents*sizeof(Event));
  t->MuteCount = 0;

  Lock->Lock();
  ctx->PerfData=t;
  ThreadList[NumThreads]=ctx;
  NumThreads++;
  Lock->Unlock();
}

void sPerfRemThread(sThreadContext *ctx)
{
  if (!Inited) return;

  if (!ctx) ctx=sGetThreadContext();
  if (!ctx->PerfData) return;

  Lock->Lock();

  Thread *t=(Thread*)ctx->PerfData;
  delete[] t->Events;
  delete t;
  ctx->PerfData=0;

  for (int i=0; i<NumThreads; i++)
  {
    if (ThreadList[i]==ctx)
    {
      --NumThreads;
      for (int j=i; j<NumThreads; j++)
        ThreadList[j]=ThreadList[j+1];

      if (ShowDetails>=NumThreads) ShowDetails--;

      break;
    }
  }

  Lock->Unlock();
}

/****************************************************************************/
/***                                                                      ***/
/*** Function/Scope tracking                                              ***/
/***                                                                      ***/
/****************************************************************************/

void sPerfEnter(const sChar *name, uint32_t color, sThreadContext *tid)
{
  if (!sPerfMon::Inited) return;

  if (!tid) tid=sGetThreadContext();
  Thread *t=(Thread*)tid->PerfData;

  if (!t) return;
  if (t->Level==MAX_NESTING) return;
  if (t->MuteCount) return;

  uint64_t time = sGetTimeUS()-StartTime;
  Event &e=t->Events[t->NumEvents++];
  e.Time=time;
  e.Level=t->Level;

  ThreadEvent &te=t->Stack[t->Level++];
  e.Ev.Color=te.Color=color;
  e.Ev.Name=te.Name=name;
  e.Ev.Name8=te.Name8=0;

  if (t->NumEvents==MaxEvents) t->NumEvents--;
}

void sPerfEnter(const sChar8 *name, uint32_t color, sThreadContext* tid)
{
  if (!sPerfMon::Inited) return;

  if (!tid) tid=sGetThreadContext();
  Thread *t=(Thread*)tid->PerfData;
  if (!t) return;
  if (t->Level==MAX_NESTING) return;
  if (t->MuteCount) return;

  uint64_t time = sGetTimeUS()-StartTime;

  Event &e=t->Events[t->NumEvents++];
  e.Time=time;
  e.Level=t->Level;

  ThreadEvent &te=t->Stack[t->Level++];
  e.Ev.Color=te.Color=color;
  e.Ev.Name=te.Name=0;
  e.Ev.Name8=te.Name8=name;

  if (t->NumEvents==MaxEvents) t->NumEvents--;
}

void sPerfSet(const sChar *name, uint32_t color, sThreadContext *tid)
{
  if (!sPerfMon::Inited) return;
  uint64_t time = sGetTimeUS()-StartTime;

  if (!tid) tid=sGetThreadContext();
  Thread *t=(Thread*)tid->PerfData;
  if (!t) return;
  if (t->MuteCount) return;

  Event &e=t->Events[t->NumEvents++];
  e.Time=time;
  e.Level=t->Level-1;
  e.Ev.Color=color;
  e.Ev.Name=name;
  e.Ev.Name8=0;
  if (t->NumEvents==MaxEvents) t->NumEvents--;

}

void sPerfLeave(sThreadContext* tid)
{
  if (!sPerfMon::Inited) return;
  uint64_t time = sGetTimeUS()-StartTime;

  if (!tid) tid=sGetThreadContext();
  Thread *t=(Thread*)tid->PerfData;
  if (!t) return;
  if (!t->Level) return;
  if (t->MuteCount) return;

  Event &e=t->Events[t->NumEvents++];
  e.Time=time;
  if (--t->Level>0)
  {
    ThreadEvent &te=t->Stack[t->Level-1];
    e.Ev=te;
  }
  else
  {
    e.Ev.Color=0x404040;
    e.Ev.Name=L"???";
    e.Ev.Name8=0;
  }
  e.Level=t->Level-1;

  if (t->NumEvents==MaxEvents) t->NumEvents--;

}

void sPerfMuteEnter(sThreadContext *tid)
{
  if (!sPerfMon::Inited) return;

  if (!tid) tid=sGetThreadContext();
  Thread *t=(Thread*)tid->PerfData;

  if (!t) return;
  
  t->MuteCount++;
}

void sPerfMuteLeave(sThreadContext *tid)
{
  if (!sPerfMon::Inited) return;

  if (!tid) tid=sGetThreadContext();
  Thread *t=(Thread*)tid->PerfData;

  if (!t) return;

  t->MuteCount--;
}

/****************************************************************************/
/***                                                                      ***/
/*** Value tracking                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sPerfAddValue(const sChar *name, const int *ptr, int min, int max, int group)
{
  if (!sPerfMon::Inited) return;
  Lock->Lock();

  sVERIFY(NumValues<MaxValues);
  int id=NumValues++;
  Value &v=Values[id];
  v.Name=name;
  v.Group=group;
  v.Min=min;
  v.Max=max;
  v.Ptr=ptr;
  ValuesSorted=0;
  Lock->Unlock();
}

void sPerfRemValue(const int *ptr)
{
  if (!sPerfMon::Inited) return;
  Lock->Lock();
  for (int i=0; i<NumValues; i++) if (Values[i].Ptr==ptr)
  {
    NumValues--;
    for (int j=i; j<NumValues; j++)
      Values[j]=Values[j+1];
    break;
  } 
  Lock->Unlock();
}

void sPerfIntGetValue(int index, const sChar *&name, int &value, sBool &groupstart)
{
  Lock->Lock();
  name=0;
  value=0;
  groupstart=0;
  if (!Inited || index<0 || index>=NumValues)
  {
     Lock->Unlock();
     return;
  }

  Value &v=Values[index];
  name=v.Name;
  value=*v.Ptr;
  if (index==0 || v.Group!=Values[index-1].Group)
    groupstart=sTRUE;

  Lock->Unlock();
}

/****************************************************************************/

void sPerfAddSwitch(const sChar *name, const sChar *choice, int *ptr)
{
  if (!sPerfMon::Inited) return;
  Lock->Lock();
  sVERIFY(NumSwitches<MaxSwitches);
  Switch &s=Switches[NumSwitches++];
  s.Name=name;
  s.Choice=choice;
  s.Ptr=ptr;
  Lock->Unlock();
}

void sPerfRemSwitch(int *ptr)
{
  if (!sPerfMon::Inited) return;
  Lock->Lock();
  for (int i=0; i<NumSwitches; i++) if (Switches[i].Ptr==ptr)
  {
    NumSwitches--;
    for (int j=i; j<NumSwitches; j++)
      Switches[j]=Switches[j+1];
    break;
  } 
  Lock->Unlock();
}

void sPerfIntGetSwitch(int index, const sChar *&name, const sChar *&choice, int &value)
{
  Lock->Lock();
  name=0;
  choice=0;
  value=0;
  if (!Inited || index<0 || index>=NumSwitches)
  {
    Lock->Unlock();
    return;
  }

  Switch &s=Switches[index];
  name=s.Name;
  choice=s.Choice;
  value=*s.Ptr;

  Lock->Unlock();
}

void sPerfIntSetSwitch(const sChar *name, int newvalue)
{
  Lock->Lock();
  for (int i=0; i<NumSwitches; i++)
    if (!sCmpString(name,Switches[i].Name))
    {
      *Switches[i].Ptr=sClamp(newvalue,0,sCountChoice(Switches[i].Choice));
      break;
    }
  Lock->Unlock();
}

/****************************************************************************/

void sAddPerfMon(int maxThreads, int maxEvents, int maxValues, int maxSwitches)
{
  sPerfMon::MaxThreads=maxThreads;
  sPerfMon::MaxEvents=maxEvents;
  sPerfMon::MaxValues=maxValues;
  sPerfMon::MaxSwitches=maxSwitches;
  sPerfMon::PreInit();
  sAddSubsystem(L"PerfMon",0xe0,sPerfMon::Init,sPerfMon::Exit);
}

#endif

