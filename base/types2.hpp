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
/***                                                                      ***/
/***   This source contains all basic classes that require dynamic        ***/
/***   memory management.                                                 ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_BASE_TYPES2_HPP
#define FILE_BASE_TYPES2_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/serialize.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Containers                                                         ***/
/***                                                                      ***/
/****************************************************************************/

/************************************************************************/ /*! 
\ingroup altona_base_types_arrays

sArray is an automatically resizing version of sStaticArray.

- You should use HintSize() to set the initial potential size of sArray
  to avoid the overhead of growing.
- to grow the array, you need to call one of the AddXXX() functions, like
  AddTail() or AddMany(). If you index past the current count of the array
  it will assert.

*/ /*************************************************************************/

template <class Type> class sArray : public sStaticArray<Type>
{
  void ReAlloc(int max)          { if(max>=this->Used && max!=this->Alloc) { sTAG_CALLER(); Type *n=new Type[max];
                                    if (n) { for(int i=0;i<this->Used;i++) n[i]=this->Data[i]; 
                                    delete[] this->Data; this->Data=n; this->Alloc=max; } } }

public:
  void Grow(int add)             { if(this->Used+add>this->Alloc) ReAlloc(sMax(this->Used+add,this->Alloc*2)); }
  void GrowTo(int size)             { if(size>this->Alloc) ReAlloc(sMax(size,this->Alloc*2)); }
  void Swap(sArray<Type> &a)      { sStaticArray<Type>::Swap(a); }
  void Swap(int i,int j)        { sStaticArray<Type>::Swap(i,j); }
};

template <class Type> class sAutoArray : public sArray<Type>
{
public:
  ~sAutoArray<Type>() { sDeleteAll(*this); }
};

// sFixedArray is a static array that preallocates items for its maximum size
template <class Type> class sFixedArray : public sStaticArray<Type>
{
public:
  sFixedArray(int count)         { sStaticArray<Type>::HintSize(count); sStaticArray<Type>::AddMany(count); }
};


/************************************************************************/ /*! 
\ingroup altona_base_types_arrays

sEndlessArray will grow automatically when indexing elements beyond its count.
- Not derived from sStaticArray.
- You must specify a default value in the constructor.
- Inefficient for sparse data, allocation begins at element 0
- All elements will be initialized with the default value when growing.
- On reading beyond the current count, the default value will returned without 
  growing.
- Only when you write beyond the current count, the array actually grows.

*/ /*************************************************************************/

template <class Type> class sEndlessArray
{
  Type Default;
  Type *Data;
  int Used;
  int Alloc;

  void Grow(int max)
  {
    if(max>Alloc)
    {
      int newalloc = sMax(max,Alloc*2);
      Type *newdata = new Type[newalloc];

      for(int i=0;i<Used;i++)
        newdata[i] = Data[i];
      delete[] Data;
      Data = newdata;
      Alloc = newalloc;
    }
    if(max>Used)
    {
      for(int i=Used;i<max;i++)
        Data[i] = Default;
      Used = max;
    }
  }
public:
  //! constructor
  sEndlessArray(Type initializer) : Default(initializer)
  { Data = 0; Used = 0; Alloc = 0; }
  //! destructor
  ~sEndlessArray()   
  { delete[] Data; }
  //! indexing
  const Type &operator[](int i) const 
  { sVERIFY(i>=0); if(i>=Used) return Default; else return Data[i]; }
  //! indexing
  Type &operator[](int i)    
  { sVERIFY(i>=0); Grow(i+1); return Data[i]; }
  //! indexing
  const Type &Get(int i) const 
  { sVERIFY(i>=0); if(i>=Used) return Default; else return Data[i]; }
  //! pre-allocate storage
  void HintSize(int i)
  { sVERIFY(i>=0); Grow(i); }
  //! clear array without freeing memory
  void Clear()
  { Used = 0; }
  //! get the highest element written to.
  int GetCount()
  { return Used; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Garbage Collection Memory Management                               ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   for each GC class, implement the Tag() member so that it calls     ***/
/***   Need() for every sObject-derived member. You don't have to care    ***/
/***   about cyclic graphs, that's all handled for you in Need().         ***/
/***   Need() may be called with a 0 pointer.                             ***/
/***                                                                      ***/
/***   To get some runtime class identification, you should add the       ***/
/***   macro sCLASSNAME(MyClass).                                          ***/
/***                                                                      ***/
/***   To Check for a certain class, you don't need to compare strings,   ***/
/***   You can compare the pointer with the one returned by the static    ***/
/***   function, like: if(o->GetClassName()==sButtonControl::ClassName()) ***/
/***                                                                      ***/
/***   You will need to register and unregister the Root of your object   ***/
/***   model with sAddRoot() / sRemRoot(). You can register as many       ***/
/***   roots as you like. make sure that every object in an object model  ***/
/***   is reached through the Tag() recursion from the root object.       ***/
/***                                                                      ***/
/***   The garbage collector is triggered with the sCollect() function.   ***/
/***   The collection takes place after processing for this frame has     ***/
/***   ended. It is save to trigger the collection multiple times per     ***/
/***   frame, it will not happen more than once per frame. Usually it     ***/
/***   is fast enough to do the collection every frame.                   ***/
/***                                                                      ***/
/***   The finalize phase solves a problem in the destructor: when the    ***/
/***   destructor is called, some childs may have already be deleted.     ***/
/***   So often important cleanup work can not be done. The finally       ***/
/***   phase is called for all objects pending for deletion before any    ***/
/***   destructor is called. In this phase you should not perform         ***/
/***   destruction, just some cleanup, like working with nonmanaged       ***/
/***   objects or stopping helper threads. After finalizing, all managed  ***/
/***   objects must be in a valid state.                                  ***/
/***                                                                      ***/
/***   For multithreaded applications you have to lock the garbage        ***/
/***   collector with sGCLock() when working on gargabe collected         ***/
/***   structures with creating new garbage collected objects.            ***/
/***                                                                      ***/
/****************************************************************************/

class sObject
{
public:

  // implement these:

  virtual void Tag() {}

  // internal functions:

  sObject();                      // adds the object to GC
  virtual ~sObject();
  static const sChar *ClassName() { return L"sObject"; }
  static sObject *NewObjectStatic() { sTAG_CALLER(); return new sObject; }
  virtual sObject *NewObjectVirtual() { sTAG_CALLER(); return new sObject; }
  virtual const sChar *GetClassName() { return ClassName(); }
  virtual void Finalize();        // finalize phase: all objects are still valid, but are soon deleted.
  int NeedFlag;                  // flag needed objects
  void Need();                    // this implements the GC recursion
};

// use this macro to implement the classname thing...

#define sCLASSNAME(name) \
static const sChar *ClassName() { return L ## #name; } \
static sObject *NewObjectStatic() { sTAG_CALLER(); return new name; } \
sObject *NewObjectVirtual() { sTAG_CALLER(); return new name; } \
const sChar *GetClassName() { return ClassName(); }

#define sCLASSNAME_NONEW(name) \
static const sChar *ClassName() { return L ## #name; } \
const sChar *GetClassName() { return ClassName(); }

/****************************************************************************/

void sAddRoot(sObject *);         // add an object as GC root
void sRemRoot(sObject *);         // add an object as GC root
void sCollect();                  // do the recursion
void sGCLock();                   // lock/unlock for multithreaded GC use
void sGCUnlock();
int sGetRootCount();

/****************************************************************************/

template <template <typename> class ArrayType,class BaseType> 
void sNeed(ArrayType<BaseType> &a)
{  
  BaseType o;
  sFORALL(a,o) o->Need();
}

/****************************************************************************/
/***                                                                      ***/
/***   A message system for sObject                                       ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Most usefull for the GUI.                                          ***/
/***                                                                      ***/
/***   A message has                                                      ***/
/***                                                                      ***/
/***   - a target object                                                  ***/
/***   - a target member function                                         ***/
/***   - an sDInt code                                                    ***/
/***                                                                      ***/
/***   message system features                                            ***/
/***                                                                      ***/
/***   - Send messages immediatly                                         ***/
/***   - Post messages for later execution                                ***/
/***   - message are copied when posting, so the original may die.        ***/
/***   - it is safe to call Send or Post with 0-this-ptr and 0-target-ptr ***/
/***                                                                      ***/
/****************************************************************************/

struct sMessage
{
  int Type;
  sDInt Code;
  sObject *Target;
  union
  {
    void (sObject::*Func1)();
    void (sObject::*Func2)(sDInt);
    void (         *Func3)(sObject *);
    void (         *Func4)(sObject *,sDInt);
    void (sObject::*Func11)(const struct sWindowDrag &);
    void (sObject::*Func12)(const struct sWindowDrag &,sDInt);
  };

  sMessage()                                                                                      { Type=0; Code=0; Target=0; Func1=0; }
//  sMessage(const sMessage &m)                                                                     { Type=m.Type; Code=m.Code; Target=m.Target; Func1=m.Func1; }
  template<class T, class TBase> sMessage(T *target,void (TBase::*func)()                                ,sDInt code=0) { Type=1;    Code=code; Target=static_cast<TBase*>(target); Func1 =(void (sObject::*)()                )func; }
  template<class T, class TBase> sMessage(T *target,void (TBase::*func)(sDInt)                           ,sDInt code=0) { Type=2;    Code=code; Target=static_cast<TBase*>(target); Func2 =(void (sObject::*)(sDInt)           )func; }
                           sMessage(sObject *target,void (*func)(sObject*)                               ,sDInt code=0) { Type=3;    Code=code; Target=(sObject *)         target ; Func3 =func; }
                           sMessage(sObject *target,void (*func)(sObject*,sDInt)                         ,sDInt code=0) { Type=4;    Code=code; Target=(sObject *)         target ; Func4 =func; }
  template<class T, class TBase> sMessage(T *target,void (TBase::*func)(const struct sWindowDrag &      ),sDInt code=0) { Type=0x11; Code=code; Target=static_cast<TBase*>(target); Func11=(void (sObject::*)(const struct sWindowDrag &      ))func; }
  template<class T, class TBase> sMessage(T *target,void (TBase::*func)(const struct sWindowDrag &,sDInt),sDInt code=0) { Type=0x12; Code=code; Target=static_cast<TBase*>(target); Func12=(void (sObject::*)(const struct sWindowDrag &,sDInt))func; }

  void Post() const;                      // enque message
  void PostASync() const;                 // thread save message posting
  void Send() const;                      // send now!
  void Drag(const struct sWindowDrag &) const;  // misuse message to send dragging. can't post dragging :-)
  static void Pump();                     // send enqued messages
  sBool IsEmpty() const { return Type==0; }     // true for unset message
  sBool IsValid() const { return Type!=0; }     // true for set message
  void Clear() { Type=0; Target=0; Code=0; Func1=0; }
};

/****************************************************************************/
/***                                                                      ***/
/***   sMessageTimer                                                      ***/
/***                                                                      ***/
/***   Repeatedly send a specific message.                                ***/
/***   Usefull to keep gui alive                                          ***/
/***   Slightly Inaccurate                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sMessageTimer
{
  friend void sMessageTimerThread(class sThread *t,void *v);
  class sThread *Thread;
  sMessage Msg;
  int Delay;
  int Loop;
public:
  sMessageTimer(const sMessage &msg,int delay,int loop);
  ~sMessageTimer();
};

/****************************************************************************/
/***                                                                      ***/
/***   Text Buffer Class (dynamic string of some kind)                    ***/
/***                                                                      ***/
/***   Recomended use:                                                    ***/
/***   - Preparing output for a text file, logging.                       ***/
/***   - As buffer for a text editor control                              ***/
/***   - for potentially long strings of variable size                    ***/
/***                                                                      ***/
/***   consider sPoolString for non-changing strings                      ***/
/***   consider sString<> for small strings (64 characters or less)       ***/
/***                                                                      ***/
/****************************************************************************/

class sTextBuffer
{
  int Alloc;                     // allocated character count
  int Used;                      // used character count
  sChar *Buffer;                  // buffer. not alway 0-terminated
  void Grow(int add);            // grow to make space for some additional chars
  void Init();
public:
  sTextBuffer();
  sTextBuffer(const sTextBuffer& tb);
  sTextBuffer(const sChar* t);
  ~sTextBuffer();

  sTextBuffer& operator=(const sTextBuffer& tb);
  sTextBuffer& operator=(const sChar* t);

  // serialization
  template <class streamer> void Serialize_(streamer &);
  void Serialize(sReader &stream);
  void Serialize(sWriter &stream);
  void Load(sChar *filename);

  int GetLength();

  // buffer management

  void Clear();                   // clear buffer. don't deallocate
  void SetSize(int count);       // enlarge and preallocate buffer
  const sChar *Get() const;       // terminate buffer and return pointer !!! THIS IS NOT CONST !!!
  sChar *Get();                   // terminate buffer and return pointer
  int GetCount() { return Used; };
  int GetChar(int pos) { if(pos>=0 && pos<Used) return Buffer[pos]; else return 0; }
  void Insert(int pos,sChar c);
  void Insert(int pos,const sChar *c,int len=-1);
  void Set(int pos,sChar c);
  void Delete(int pos);
  void Delete(int pos,int count);

  // printing. this appends!

  void Indent(int count);        // finds last linefeed and indents with spaces
  void PrintChar(int c);
  void Print(const sChar *text);
  void Print(const sChar *text,int len);
  void PrintListing(const sChar *text,int firstline=1);

  sPRINTING0(PrintF, sString<0x4000> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,Print((const sChar*)tmp););
};

/****************************************************************************/
/***                                                                      ***/
/***   Text Writer Class (formatted output to a large text file)          ***/
/***   Always saves as ANSI 8-bit.                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sTextFileWriter : public sTextStreamer
{
  static const int BufferSize = 0x4000;

  sString<BufferSize> PrintBuffer;
  sFile *Target;
  sTextBuffer Buffer;

public:
  sTextFileWriter();
  sTextFileWriter(sFile *target); // transfers ownership
  sTextFileWriter(const sChar *filename);
  ~sTextFileWriter();

  void Begin(sFile *target);
  void Begin(const sChar *filename);
  void Flush();
  void End();

  const sStringDesc GetPrintBuffer()  { return PrintBuffer; }
  virtual void Print()                { Print(PrintBuffer); }
  void Print(const sChar *text)       { Buffer.Print(text); if(Buffer.GetCount()>=BufferSize) Flush(); }
};

/****************************************************************************/
/***                                                                      ***/
/***   sPoolArray                                                         ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   useful for constructing large arrays without knowing their final   ***/
/***   size. grows without reallocation. pointers stay valid.             ***/
/***                                                                      ***/
/***   similar too sMemoryPool but:                                       ***/
/***                                                                      ***/
/***   * managing only items of one data type                             ***/
/***   + items can be enumerated                                          ***/
/***                                                                      ***/
/****************************************************************************/

template <typename T> class sPoolArray
{
  struct PoolItem
  {
    PoolItem(int size)                 { Data = new T[size]; }
    ~PoolItem()                         { sDeleteArray(Data); }
    T *Data;
  };

  sArray<PoolItem*> Items;
  int Used;
  int Alloc;
  int Log2Size;

  int ItemSize;
  int Mask;

  void Grow()                           { Items.AddTail(new PoolItem(ItemSize)); Alloc += ItemSize; }

  const T& GetUnsafe(int i) const      { return Items[i>>Log2Size]->Data[i&Mask]; }
  T& GetUnsafe(int i)                  { return Items[i>>Log2Size]->Data[i&Mask]; }
public:

  sPoolArray(int l2s=12):Used(0),Alloc(0),Log2Size(l2s) { ItemSize = 1<<Log2Size; Mask = ItemSize-1; }
  ~sPoolArray()                         { Reset(); }

  void Clear()                          { Used = 0; }
  void Reset(int l2s=-1)               { sDeleteAll(Items); Alloc = 0; Used = 0; if(l2s!=-1) {Log2Size=l2s; ItemSize = 1<<l2s; Mask = ItemSize-1; } }

  int GetCount() const                 { return Used; }
  int GetItemCount() const             { return Items.GetCount(); }
  int GetItemSize() const              { return ItemSize; }
  sBool IsEmpty() const                 { return Used==0; }
  sBool IsFull() const                  { return Used==Alloc; }

  void AddTail(const T &e)              { if(Used>=Alloc) Grow(); GetUnsafe(Used) = e; Used++; }
  T* AddTail()                          { if(Used>=Alloc) Grow(); T *ptr = &(GetUnsafe(Used)); Used++; return ptr; }
  T* AddMany(int &count)               { count = (ItemSize-(Used&Mask)); if(Used==Alloc) Grow(); T *ptr = &(GetUnsafe(Used)); Used+=count; return ptr;}

  const T& operator[](int i) const     { sVERIFY(i>=0 && i<=Used); return Items[i>>Log2Size]->Data[i&Mask]; }
  T& operator[](int i)                 { sVERIFY(i>=0 && i<=Used); return Items[i>>Log2Size]->Data[i&Mask]; }


  void RemAt(int p)                    { (*this)[p] = (*this)[Used-1]; Used--; }
  void RemTail()                        { sVERIFY(Used); Used--; }

  void Swap(sPoolArray &a)              { sSwap(Used,a.Used); sSwap(Alloc,a.Alloc); sSwap(Log2Size,a.Log2Size);
                                          sSwap(ItemSize,a.ItemSize); sSwap(Mask,a.Mask); Items.Swap(a.Items); }
  void ConvertTo(sStaticArray<T> &dest) const;
};

/****************************************************************************/

template <typename T> 
void sPoolArray<T>::ConvertTo(sStaticArray<T> &dest) const
{
  dest.HintSize(Used);
  int left = Used;
  for(int i=0;i<Items.GetCount();i++)
  {
    T *dst = dest.AddMany(sMin(ItemSize,left));
    const T *src = Items[i]->Data;
    for(int k=0;k<ItemSize && k<left;k++)
      *dst++ = *src++;
    left -= ItemSize;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   sSmallObjectPool                                                   ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   simple allocator for large numbers of small objects of one type.   ***/
/***   basically a sPoolArray with free. objects must be at least as big  ***/
/***   as a void pointer. not threadsafe!                                 ***/
/***                                                                      ***/
/****************************************************************************/

template <typename T> class sSmallObjectPool
{
  sMemoryPool InternalPool;
  void *FreeList;
  int Alignment;

public:
  sSmallObjectPool(int allocflags=sAMF_HEAP,int poolGranularity=4096,int alignment=16)
    : InternalPool(poolGranularity*sizeof(T),allocflags,1024)
  {
    sVERIFYSTATIC(sizeof(T) >= sizeof(void*));
    FreeList = 0;
    Alignment = alignment;
  }

  T *Alloc()
  {
    T *item;

    if(FreeList) // pop from head
    {
      item = (T*) FreeList;
      FreeList = *((void **) FreeList);
    }
    else
      item = (T*) InternalPool.Alloc(sizeof(T),Alignment);

    return item;
  }

  void Free(T *what)
  {
    if(what)
    {
      *((void **) what) = FreeList;
      FreeList = (void *) what;
    }
  }

  sU32 BytesAllocated()
  {
    return InternalPool.BytesAllocated();
  }
};

/****************************************************************************/
/***                                                                      ***/
/***   sSmallObjectPoolStatic                                             ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   like sSmallObjectPool, just without growing                        ***/
/***                                                                      ***/
/****************************************************************************/

template <typename T> class sSmallObjectPoolStatic
{
  int TSize;
  sU8* InternalPool;
  T* FreeList;

public:
  explicit sSmallObjectPoolStatic(int count,int alignment=4,int allocflags=sAMF_DEFAULT)
  {
    TSize=sAlign<int>(sizeof(T),alignment);
    sVERIFY(TSize >= sizeof(void*));
    InternalPool=(sU8*)sAllocMem(count*TSize,alignment,allocflags);

    for (int i=0; i<count-1; i++)
      *((T**)(InternalPool+i*TSize))=(T*)(InternalPool+(i+1)*TSize);
    *((T**)(InternalPool+(count-1)*TSize))=0;

    FreeList = (T*)InternalPool;
  }

  ~sSmallObjectPoolStatic()
  {
    sFreeMem(InternalPool);
  }

  T *Alloc()
  {
    if (!FreeList) return 0;
    T *item=FreeList;
    FreeList=*((T**)FreeList);
    return item;
  }

  void Free(T *what)
  {
    if (!what) return;
    
    *((T**)what) = FreeList;
    FreeList=what;
  }
};

/****************************************************************************/
/***                                                                      ***/
/***   StringPool of PoolStrings                                          ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   If you don't want to care about string memory management,          ***/
/***   you can add a string to a stringpool and receive a handle.         ***/
/***   - the memory is never freed                                        ***/
/***   - same strings will only be stored once                            ***/
/***   - same strings will receive the same handle                        ***/
/***   - strings can not be altered                                       ***/
/***   this is perfect for applications like parsers and symbol tables.   ***/
/***   this is awful when the strings get changed often, and              ***/
/***   catastrophic for a text editor.                                    ***/
/***                                                                      ***/
/***   there is an sClearStringPool(), but it is only valid to call       ***/
/***   when you are absolutly sure that ALL sPoolStrings are destroyed!   ***/
/***                                                                      ***/
/***   the stringpool itself is a global variable. it makes little        ***/
/***   sense to have multiple stringpools.                                ***/
/***                                                                      ***/
/****************************************************************************/

const sChar *sAddToStringPool(const sChar *s,int len, sBool *isnew=0);
const sChar *sAddToStringPool2(const sChar *a,int al,const sChar *b,int bl);
void sClearStringPool();       
extern const sChar *sPoolStringEmpty;

struct sPoolString              // a string from the pool
{
private:
  const sChar *Buffer;
public:
  sPoolString()                               { Buffer = sPoolStringEmpty; }
  sPoolString(const sChar *s)                 { Buffer = sAddToStringPool(s,sGetStringLen(s)); }
  sPoolString(const sChar *s,int len)        { Buffer = sAddToStringPool(s,len); }
  sPoolString &operator=(const sChar *s)      { Buffer = sAddToStringPool(s,sGetStringLen(s)); return *this; }
  void Init(const sChar *s)                   { Buffer = sAddToStringPool(s,sGetStringLen(s)); }
  void Init(const sChar *s,int len)          { Buffer = sAddToStringPool(s,len); }

  operator const sChar*() const               { return Buffer; }
  const sChar *Get() const                    { return Buffer; }


  int operator==(const sChar *s) const       { return sCmpString(Buffer,s)==0; }
  int operator==(const sPoolString &s) const { return Buffer==s.Buffer; }
  int operator!=(const sChar *s) const       { return sCmpString(Buffer,s)!=0; }
  int operator!=(const sPoolString &s) const { return Buffer!=s.Buffer; }
  bool operator< (const sPoolString &s) const { return sCmpString(Buffer,s.Buffer)<0; }
  bool operator> (const sPoolString &s) const { return sCmpString(Buffer,s.Buffer)>0; }
  bool operator<=(const sPoolString &s) const { return sCmpString(Buffer,s.Buffer)<=0; }
  bool operator>=(const sPoolString &s) const { return sCmpString(Buffer,s.Buffer)>=0; }
  sChar operator[](int i) const              { return Buffer[i]; }
  int Count() const                          { return sGetStringLen(Buffer); }
  sBool IsEmpty() const                       { return Buffer[0]==0; }
  sU32 GetHash() const                        { return sHashString(Buffer); }

  void Add(const sChar *a,const sChar *b)     { Buffer = sAddToStringPool2(a,sGetStringLen(a),b,sGetStringLen(b)); }
};

inline sReader& operator| (sReader &s,sPoolString &a) { int len; s | len; sChar *tmp = sALLOCSTACK(sChar,len); s.ArrayChar(tmp,len); a.Init(tmp,len); s.Align(4); return s; }

template<int size> inline sString<size>::sString(const sPoolString &s) { sCopyString(Buffer,s,size); }
template<int size> inline sString<size> &sString<size>::operator =(const sPoolString &s) { if (s) sCopyString(Buffer,s,size); else Buffer[0]=0; return *this; }
template<int size> inline sBool sString<size>::operator ==(const sPoolString &s) const { return sCmpString(Buffer,s)==0; }
template<int size> inline sBool sString<size>::operator !=(const sPoolString &s) const { return sCmpString(Buffer,s)!=0; }
template<int size> inline sBool sString<size>::operator >(const sPoolString &s) const { return sCmpString(Buffer,s)>0; }
template<int size> inline sBool sString<size>::operator <(const sPoolString &s) const { return sCmpString(Buffer,s)<0; }


template<class t0>
sPoolString sPoolF(const sChar *f,t0 a0) 
{ sString<1024> buf; buf.PrintF(f,a0); return sPoolString(buf); }
template<class t0,class t1>
sPoolString sPoolF(const sChar *f,t0 a0,t1 a1) 
{ sString<1024> buf; buf.PrintF(f,a0,a1); return sPoolString(buf); }
template<class t0,class t1,class t2>
sPoolString sPoolF(const sChar *f,t0 a0,t1 a1,t2 a2) 
{ sString<1024> buf; buf.PrintF(f,a0,a1,a2); return sPoolString(buf); }

/****************************************************************************/
/***                                                                      ***/
/***   Simple Hash Table                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   - does not manage memory for keys or values                        ***/
/***   - add does not check if the key already exists                     ***/
/***                                                                      ***/
/***   sHashTableBase contains the implementation                         ***/
/***                                                                      ***/
/***   Use the sHashTable template if the key has comparison operator     ***/
/***   and a Hash() member function                                       ***/
/***                                                                      ***/
/***   You can also derive the class yourself                             ***/
/***                                                                      ***/
/****************************************************************************/

// base class

class sHashTableBase
{
private:
  struct Node                     // a node in the hashtable
  {
    Node *Next;                   // linked list inside bin
    const void *Key;              // ptr to key
    void *Value;                  // ptr to value
  };

  // the hashtable

  int HashSize;                  // size of hashtable (power of two)
  int HashMask;                  // size-1;
  Node **HashTable;                // the hashtable itself

  // don't allocate nodes one-by-one!

  int NodesPerBlock;             // nodes per allocation block
  Node *CurrentNodeBlock;         // current node allocation block
  int CurrentNode;               // next free node in allocation block
  sArray<Node *> NodeBlocks;      // all allocation blocks
  Node *FreeNodes;                // list of free nodes for reuse

  // interface

  Node *AllocNode();
public:
  sHashTableBase(int size=0x4000,int nodesperblock=0x100);
  virtual ~sHashTableBase();
  void Clear();
  void ClearAndDeleteValues();
  void ClearAndDeleteKeys();
  void ClearAndDelete();

protected:
  void Add(const void *key,void *value);
  void *Find(const void *key);
  void *Rem(const void *key);
  void GetAll(sArray<void *> *a);

  virtual sBool CompareKey(const void *k0,const void *k1)=0;
  virtual sU32 HashKey(const void *key)=0;
};


// usage

template<class KeyType,class ValueType>
class sHashTable : public sHashTableBase
{
protected:
  sBool CompareKey(const void *k0,const void *k1)    { return (*(const KeyType *)k0)==(*(const KeyType *)k1); }
  sU32 HashKey(const void *key)                      { return ((const KeyType *)key)->Hash(); }
public:
  sHashTable(int s=0x4000,int n=0x100) : sHashTableBase(s,n)  {}
  void Add(const KeyType *key,ValueType *value)         { sHashTableBase::Add(key,value); }
  ValueType *Find(const KeyType *key)                   { return (ValueType *) sHashTableBase::Find(key); }
  ValueType *Rem(const KeyType *key)                    { return (ValueType *) sHashTableBase::Rem(key); }

  void GetAll(sArray<ValueType *> *a)                 { sHashTableBase::GetAll((sArray<void *> *)a); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Rectangular Regions                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sRectRegion
{
  void AddParts(sRect old,const sRect &sub);
public:
  sArray<sRect> Rects;
  void Clear();
  void Add(const sRect &r);
  void Sub(const sRect &r);
  void And(const sRect &r);
};

/****************************************************************************/
/***                                                                      ***/
/***   String Map                                                         ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   A simple hash table for mapping strings to pointers.               ***/
/***                                                                      ***/
/***   sStringMap_ maps to untyped pointers (void*), while the            ***/
/***   sStringMap  template allows storing of typed pointers              ***/
/***                                                                      ***/
/***   The number of buckets is fixed on intialization and                ***/
/***   should approximately equal the number of expected items.           ***/
/***   Initial memory consumption: numSlots * sizeof(void*)               ***/
/***   Speed and memory efficiency degrades gracefully (linear) with the  ***/
/***   number of inserted values.                                         ***/
/***                                                                      ***/
/***   Key strings will be copied on insertion (Set).                     ***/
/***   Setting a value for an already inserted key will overwrite the     ***/
/***   old value.                                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sStringMap_
{
protected:

  int Size;
  typedef struct Slot_ {Slot_ * next; sChar * key; void * value;} Slot;
  Slot ** Slots;

  sU32 Hash (const sChar * key) const;

public: 

  sStringMap_ (int numSlots);    // creates a map with numSlots buckets
  virtual ~sStringMap_ ();
  void Clear ();                  // deletes all key value pairs

  void * Get (const sChar * key) const; // returns the value for key, or NULL if not set
  void   Set (const sChar * key, void * value); // sets the value for key
  void   Del (const sChar * key);    // removes the value for key

  int   GetCount () const;          // returns current number of key-value pairs
  sStaticArray<sChar*> * GetKeys () const; // returns a newly allocated array with 
                                     // all the keys in arbitrary order

  void Dump () const;
};

/****************************************************************************/

template <class Type, int NumSlots> class sStringMap : public sStringMap_
{
public:

  sStringMap () : sStringMap_(NumSlots) {};

  Type Get (const sChar * key) { return (Type)sStringMap_::Get(key); };
  void Set (const sChar * key, Type value) { sStringMap_::Set(key, (void *)value); };
};

/****************************************************************************/
/***                                                                      ***/
/***   BitVector                                                          ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Set and clear bits, automatically growing.                         ***/
/***                                                                      ***/
/***   * defaults to clear                                                ***/
/***   * Will automatically grow in chunks of 128 bits (16 bytes)         ***/
/***   * Newly grown bits will be set to one if SetAll() was called       ***/
/***   * word referes to sU32 unit                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sBitVector
{
  sU32 *Data;                     // the data
  sU32 NewVal;                    // value for new bits (0 or ~0)
  sPtr Words;                     // number of words allocated. grows in chunks of 16.
public:
  sBitVector();
  ~sBitVector();
  void Resize(int bit);          // resizing large bitvectors is more efficient than autogrow

  void Set(int n);               // set to 1. with autogrow.
  void Clear(int n);             // set to 0. with autogrow.
  void Assign(int n,int v);     // set to (v&1). with autogrow.
  sBool Get(int n);              // get bit. may read beyond end of data, but will not grow data.
  void ClearAll();                // clear all. bits grown after this are cleared too.
  void SetAll();                  // set all. bits grown after this are set too.

  sBool NextBit(int &n);         // iterate. return false if last bit. start with n==-1.

  // it would not hurt to add serialization
};

#define sFORALL_BITVECTOR(bv,n) for((n)=-1;NextBit(n);)

/****************************************************************************/
/***                                                                      ***/
/*** sStream implementations                                              ***/
/***                                                                      ***/
/*** sStreamMemory   : reflects a block in memory                         ***/
/*** sStreamCache    : readahead/write cache for another stream           ***/
/*** sStreamPart     : reflects a part of another stream                  ***/
/*** sStreamTemp     : memory resident file                               ***/
/***                                                                      ***/
/****************************************************************************/
/*
class sStreamMemory : public sStream
{
protected:

  sU8 *Ptr;
  sDInt BSize, Pos;
  sBool Owner, Readonly;

public:

  sBool Open(sDInt size);   // creates new buffer
  sBool Open(const void *buffer, sDInt size, sBool iamowner=sFALSE); // opens buffer readonly
  sBool Open(void *buffer, sDInt size, sBool iamowner=sFALSE); // opens buffer read/write
  sBool Open(sStream &str); // loads other stream into memory

  sStreamMemory();
  explicit sStreamMemory(sDInt size);
  sStreamMemory(const void *buffer, sDInt size, sBool iamowner=sFALSE);
  sStreamMemory(void *buffer,sDInt size, sBool iamowner=sFALSE);
  explicit sStreamMemory(sStream &str);

  void  *Detach(sDInt *size);

  // sStream impl
  ~sStreamMemory();
  int  GetFlags();
  sBool  Close();
  sDInt Read(void *ptr, sDInt count);
  sDInt Write(const void *ptr, sDInt count);
  sSize Seek(sSize pos);
  sSize SeekEnd(sSize offset);
  sSize SeekCur(sSize offset);
  sBool EOF_();
  sSize Tell();
  sSize Size();
  const void *GetPtr(sDInt &count);
  sStream *Clone();

  using sStream::Read;
  using sStream::Write;
};


class sStreamPart : public sStream
{
protected:

  sStream *Str;
  sSize   PPos, PSize, Pos;
  sBool   Owner;

public:

  sBool Open(sStream &str, sSize from, sSize size, sBool clone=sFALSE);

  sStreamPart();
  sStreamPart(sStream &str, sSize from, sSize size, sBool clone=sFALSE);

  // sStream impl
  ~sStreamPart();
  int  GetFlags();
  sBool Close();
  sDInt Read(void *ptr, sDInt count);
  sDInt Write(const void *ptr, sDInt count);
  sSize Seek(sSize pos);
  sSize SeekEnd(sSize offset);
  sSize SeekCur(sSize offset);
  sBool EOF_();
  sSize Tell();
  sSize Size();
  sStream *Clone();

  using sStream::Read;
  using sStream::Write;
};


class sStreamCache : public sStream
{
protected:

  sStream *Str;
  sDInt BufSize;
  sU8  *WBuf, *RBuf;
  sDInt RBufPos, RBufSize;
  sDInt WBufSize;
  int Flags;

  void Flush();

public:

  sBool Open(sStream &str, sDInt granule=65536);

  sStreamCache();
  sStreamCache(sStream &str, sDInt granule=65536);

  // sStream impl
  ~sStreamCache();
  int  GetFlags();
  sBool Close();
  sDInt Read(void *ptr, sDInt count);
  sDInt Write(const void *ptr, sDInt count);
  sBool EOF_();

  using sStream::Read;
  using sStream::Write;
};


class sStreamTemp : public sStream
{
protected:

  static const int BUFSIZE=65536;

  struct Buffer
  {
    sU8 Mem[BUFSIZE];
    sDNode DNode;
  };

  sDList<Buffer,&Buffer::DNode> Nodes;
  
  sDInt FSize;
  Buffer *CurB;
  sDInt BPos,FPos;

public:

  sBool Open();
  sStreamTemp(sBool open=sFALSE);

  // sStream impl
  ~sStreamTemp();
  int  GetFlags();
  sBool Close();
  sDInt Read(void *ptr, sDInt count);
  sDInt Write(const void *ptr, sDInt count);
  sSize Seek(sSize pos);
  sSize Tell();
  sSize Size();
  sBool EOF_();
  sStream *Clone();

  using sStream::Read;
  using sStream::Write;
};

*/

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#endif // FILE_BASE_TYPES2_HPP


