/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_TASKSCHEDULER_HPP
#define FILE_UTIL_TASKSCHEDULER_HPP

#include "base/types.hpp"
#include "base/system.hpp"

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Splitting Task Scheduler                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sStsTask;
struct sStsSync;
class sStsThread;
class sStsWorkload;
class sStsManager;

/****************************************************************************/
/***                                                                      ***/
/***   A spin-blocking lock. Non-recursive (same thread can't pass twice) ***/
/***                                                                      ***/
/****************************************************************************/

struct sStsLock
{
  sU32 Count;

  sStsLock();
  void Lock();
  void Unlock();
};

void sSpin();                     // waste some time without doing much on the bus

/****************************************************************************/
/***                                                                      ***/
/***   A task with a range of subtasks                                    ***/
/***                                                                      ***/
/****************************************************************************/

typedef void (*sStsCode)(sStsManager *,sStsThread *,int start,int count,void *data);

struct sStsTask                   // an array of tasks to do
{
  int Start;                     // First Subtask under control of this structure
  int End;                       // last subtask +1
  void *Data;                     // user pointer
  sStsCode Code;                  // user code

  int Granularity;               // always grab this many subtasks
  int EndGame;                   // do not split below this threshold
  sStsWorkload *Workload;         // pointer to workload (for adding new tasks)
  
  int SyncCount;                 // number of syncs to decrease after this task
  sStsSync **Syncs;               // pointer to the syncs (usually directly after task structure
};

struct sStsSync                   // syncro-point
{
  sU32 Count;                     // counter for sync
  sStsTask *ContinueTask;         // task to start when sync is reached
};

/****************************************************************************/
/***                                                                      ***/
/***   A thread that can work tasks. Thread index 0 is on main thread     ***/
/***                                                                      ***/
/****************************************************************************/

struct sStsQueue
{
  sStsTask **Tasks;
  int TaskMax;
  int TaskCount;
  int SubTaskCount;
  int ExeCount;
  volatile sBool DontSteal;       // please don' steal my last task :-)
};

class sStsThread
{
  friend void sStsThreadFunc(class sThread *thread, void *_user);
  friend class sStsManager;
  friend class sStsWorkload;
  sStsManager *Manager;           // backlink to manager
  sStsLock *Lock;              // lock for manipulating task queue
  sThreadEvent *Event;
  sThread *Thread;                // thread for execution
  int Index;                     // index of this thread in manager
  sThreadLock WorkloadReadLock;

//  volatile sBool Running; 
  void DecreaseSync(sStsTask *t);
public:
  sStsThread(sStsManager *,int index,int taskcount,sBool thread);
  ~sStsThread();

  void AddTask(sStsTask *);
  void StealTasksFrom(sStsThread *);
  sBool Execute();

  int GetIndex() { return Index; }
};


/****************************************************************************/
/***                                                                      ***/
/***   Manage Tasks and Threads.                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sStsManager
{
  friend class sStsThread;
  friend class sStsWorkload;
  friend void sStsThreadFunc(class sThread *thread, void *_user);
  int ConfigPoolMem;
  int ConfigMaxTasks;

  sStsThread **Threads;           // threads[0] is the master thread
  int ThreadCount;               // number of threads, including master thread
  int ThreadBits;                // 1<<ThreadBits >= ThreadCount

//  sU8 *Mem;
//  volatile sPtr MemUsed;
//  sPtr MemEnd;
//  sU8 *AllocBytes(int bytes);
//  template <class T> T *Alloc(int count=1) { return (T *) AllocBytes(sizeof(T)*count); }

  volatile sBool Running;         // Indicate running state to threads
  sU32 TotalTasksLeft;


  sBool StealTasks(int to);      // implementation of thread stealing

  sDList2<sStsWorkload> FreeWorkloads;
  sDList2<sStsWorkload> ActiveWorkloads;
  volatile int ActiveWorkloadCount;

  void Start();                               
  void StartSingle();             // start single threaded
  void Finish();

  void WorkloadWriteLock();
  void WorkloadWriteUnlock();
public:
  sStsManager(int memory,int taskqueuelength,int maxcore=0);
  ~sStsManager();
  int GetThreadCount() { return ThreadCount; }

// call this only from master thread

  sStsWorkload *BeginWorkload();
  void StartWorkload(sStsWorkload *);
  void SyncWorkload(sStsWorkload *);
  void EndWorkload(sStsWorkload *);


  sBool HelpWorkload(sStsWorkload *wl); // build your own while-loop instead of using SyncWorkload

//  sStsSync *NewSync();            // manage memory for syncs yourself...
  void AddSync(sStsTask *t,sStsSync *s); 

  void Sync(sStsSync *);

};

extern sStsManager *sSched;         // this is created by sInitSts and destroyed automatically
void sAddSched();                   // you may create additional instances of sStsManager if you like.

/****************************************************************************/
/***                                                                      ***/
/***   Workloads                                                          ***/
/***                                                                      ***/
/****************************************************************************/

enum sStsWorkloadMode
{
  sSWM_IDLE = 0,
  sSWM_READY = 1,
  sSWM_RUNNING = 2,
  sSWM_FINISHED = 3,
};

class sStsWorkload
{
  friend class sStsThread;
  friend class sStsManager;

  sStsManager *Manager;
  int ThreadCount;

  sU8 *Mem;
  volatile sPtr MemUsed;
  sPtr MemEnd;

  sStsQueue **Queues;
  sU32 TasksLeft;
  sU32 TasksRunning;

  sStaticArray<sStsTask *> Tasks;
  sString<128> StatBuffer;
public:
  sStsWorkload(sStsManager *);
  ~sStsWorkload();

  sStsTask *NewTask(sStsCode code,void* data,int subtasks,int syncs);
  void AddTask(sStsTask *);                

  void Start() { Manager->StartWorkload(this); }
  void Sync()  { Manager->SyncWorkload(this); }
  void End()   { Manager->EndWorkload(this); }
  sU8 *AllocBytes(int bytes);
  template <class T> T *Alloc(int count=1) { return (T *) AllocBytes(sizeof(T)*count); }


  sDNode Node;

  // stats

  int Mode;

  sU32 StealCount;
  sU32 SpinCount;
  sU32 ExeCount;
  sU32 FailedLockCount;
  sU32 FailedStealCount;

  const sChar *PrintStat();
};

/****************************************************************************/
/***                                                                      ***/
/***   Cool Performance Meter                                             ***/
/***                                                                      ***/
/****************************************************************************/

#if sPLATFORM==sPLAT_WINDOWS
extern "C" unsigned __int64 __rdtsc();
#pragma intrinsic(__rdtsc)
inline sU64 sGetTimeStamp() { return __rdtsc(); }
#else
inline sU64 sGetTimeStamp() { return sGetTimeUS(); }
#endif

class sStsPerfMon
{
  struct Entry
  {
    sU32 Timestamp;
    sU32 Color;
  };
  int ThreadCount;
  int DataCount;
  int CountMask;

  int DataMax;
  int **Counters;
  Entry **Datas;
  int **OldCounters;
  Entry **OldDatas;
  sU64 OldStart;
  sU64 OldEnd;
  sU64 TimeStart;
  int Enable;

  sRect *Rects;

  class sGeometry *Geo;
  class sMaterial *Mtrl;
  int GeoAlloc,GeoUsed;
  struct sVertexBasic *GeoPtr;
  void Box(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 col);
  void BoxEnd();
public:
  sStsPerfMon();
  ~sStsPerfMon();

  void FlipFrame();
  void Begin(int thread,sU32 color)  { if(!Enable) return; int cnt = *Counters[thread]; *Counters[thread]=cnt+1; Entry *e = &Datas[thread][cnt&CountMask]; e->Timestamp = sU32(sGetTimeStamp()-TimeStart); e->Color = color|0xff000000; }
  void End(int thread)               { if(!Enable) return; int cnt = *Counters[thread]; *Counters[thread]=cnt+1; Entry *e = &Datas[thread][cnt&CountMask]; e->Timestamp = sU32(sGetTimeStamp()-TimeStart); e->Color = 0; }

  void Paint(const struct sTargetSpec &ts);

  int Scale;
};

extern sStsPerfMon *sSchedMon;

/****************************************************************************/

#endif // FILE_EXPERIMENTAL_TASK_HPP

