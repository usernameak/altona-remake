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

#ifndef HEADER_ALTONA_BASE_SERIALIZE
#define HEADER_ALTONA_BASE_SERIALIZE

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/system.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Serialisation, new endian-safe system                              ***/
/***                                                                      ***/
/****************************************************************************/

class sWriter
{
  uint8_t *Buffer;
  int BufferSize;
  uint8_t *CheckEnd;
  sFile *File;
  sBool Ok;
  uint8_t *Data;

  // object serialization
  struct sWriteLink *WOH[256]; 
  struct sWriteLink *WOL;
  int WOCount;

public:

  sWriter();
  ~sWriter();
  void Begin(sFile *file);
  sBool End();
  void Check();
  sBool IsOk() { return Ok; }
  void Fail() { Ok = 0; }
  sINLINE sBool IsReading() const { return 0; }
  sINLINE sBool IsWriting() const { return 1; }

  int Header(uint32_t id,int currentversion);
  uint32_t PeekHeader() { return 0 /*sSerId::Error*/; }
  sBool PeekFooter() { return sFALSE; }
  uint32_t PeekU32() { return 0; }
  void Footer();
  void Skip(int bytes);
  void Align(int alignment=4);
  int RegisterPtr(void *);
  sBool IsRegistered(void *);
  void VoidPtr(const void *obj);
  template <typename T> void Ptr(const T *obj) { VoidPtr((const void *)obj); }
  template <typename T> void Enum(const T val) { U32(val); }
  sBool If(sBool c) { U32(c); return c; }
  template <typename T> void Index(T *e,sStaticArray<T *> &a) { U32(e - a[0]); }
  template <typename T> void Once(T *obj) { if(If(obj && !IsRegistered(obj))){RegisterPtr(obj);obj->Serialize(*this);} Ptr(obj); }
  template <typename T> void OnceRef(T *obj) { if(If(obj && !IsRegistered(obj))){RegisterPtr(obj);obj->Serialize(*this);} Ptr(obj); }
  template <typename T,typename A> void OnceRef(T *obj,A *x) { if(If(obj && !IsRegistered(obj))){RegisterPtr(obj);obj->Serialize(*this,x);} Ptr(obj); }
  void Bits(int *a,int *b,int *c,int *d,int *e,int *f,int *g,int *h);

  void U8(uint8_t v)     { Data[0] = v;                           Data+=1; }
  void U16(uint16_t v)   { sUnalignedLittleEndianStore16(Data,v); Data+=2; }
  void U32(uint8_t &v)   { sUnalignedLittleEndianStore32(Data,v); Data+=4; } // useful variant for streaming 8 bit variables as 32 bit stream
  void U32(uint32_t v)   { sUnalignedLittleEndianStore32(Data,v); Data+=4; }
  void U64(uint64_t v)   { sUnalignedLittleEndianStore64(Data,v); Data+=8; }
  void ArrayU8(const uint8_t *ptr,int count);
  void ArrayU16(const uint16_t *ptr,int count);
  void ArrayU16Align4(const uint16_t *ptr,int count);
  void ArrayU32(const uint32_t *ptr,int count);
  void ArrayU64(const uint64_t *ptr,int count);
  void String(const sChar *v);

  void S8(int8_t v)     { U8((uint8_t) v); }
  void S16(int16_t v)    { U16((uint16_t) v); }
  void S32(int32_t v)    { U32((uint32_t) v); }
  void S64(int64_t v)    { U64((uint64_t) v); }
  void F32(float v)   { U32(sRawCast<uint32_t,float>(v)); }
  void F64(double v)   { U64(sRawCast<uint64_t,double>(v)); }
  void ArrayS8(const int8_t *ptr,int count)     { ArrayU8((const uint8_t *)ptr,count);  }
  void ArrayS16(const int16_t *ptr,int count)   { ArrayU16((const uint16_t *)ptr,count); }
  void ArrayChar(const sChar *ptr,int count) { ArrayU16((const uint16_t *)ptr,count); }
  void ArrayS32(const int32_t *ptr,int count)   { ArrayU32((const uint32_t *)ptr,count); }
  void ArrayS64(const int64_t *ptr,int count)   { ArrayU64((const uint64_t *)ptr,count); }
  void ArrayF32(const float *ptr,int count)   { ArrayU32((const uint32_t *)ptr,count); }
  void ArrayF64(const double *ptr,int count)   { ArrayU64((const uint64_t *)ptr,count); }

  template <class Type> void Array(const sStaticArray<Type> &a) { S32(a.GetCount()); }
  template <class Type> void ArrayNew(const sStaticArray<Type *> &a) { S32(a.GetCount()); }
  template <class Type> void ArrayNewHint(const sStaticArray<Type *> &a,int additional) { S32(a.GetCount()); }

  template <class Type> void ArrayRegister(sStaticArray<Type *> &a) {Type *e; sFORALL(a,e) RegisterPtr(e); }

  template <class Type> void ArrayAll(sStaticArray<Type> &a) { S32(a.GetCount()); for (int i=0; i<a.GetCount(); i++) { *this | a[i]; if((i&0xFF)==0) Check();} }
  template <class Type> void ArrayAllPtr(sStaticArray<Type *> &a) { S32(a.GetCount()); for (int i=0; i<a.GetCount(); i++) { *this | a[i]; if((i&0xFF)==0) Check();}  }

  ptrdiff_t GetSize();
};

/****************************************************************************/

class sReader
{
  sFile *File;
  const uint8_t *Map;
  uint8_t *Buffer;
  int BufferSize;
  const uint8_t *Data;
  const uint8_t *CheckEnd;
  uint8_t *LoadEnd;
  sBool Ok;
  int64_t ReadLeft;
  uint32_t LastId;

  void **ROL;
  int ROCount;

public:
  sBool DontMap;          // for debug purposes
  sReader();
  ~sReader();
  void Begin(sFile *file);
  sBool End();
  void Check();
  sBool IsOk() { return Ok; }
  void Fail() { Ok = 0; }
  sOBSOLETE const uint8_t *GetPtr(int bytes); // get acces to data and advance pointer
  sINLINE sBool IsReading() const { return 1; }
  sINLINE sBool IsWriting() const { return 0; }
  void DebugPeek(int count);

  int Header(uint32_t id,int currentversion);
  uint32_t PeekHeader();
  uint32_t PeekU32();
  sBool PeekFooter();
  void Footer();
  void Skip(int bytes);
  void Align(int alignment=4);
  int RegisterPtr(void *);
  sBool IsRegistered(void *) { sVERIFYFALSE; return sTRUE; } // don't use. included only to make template functions compile.
  void VoidPtr(void *&obj);
  template <typename T> void Ptr(T *&obj) { VoidPtr((void *&)obj); }
  template <typename T> void Enum(T &val) { uint32_t v; U32(v); val=(T)v; }
  sBool If(sBool c) { uint32_t c1; U32(c1); return c1; }
  template <typename T> void Index(T *&e,sStaticArray<T *> &a) { uint32_t n; U32(n); e = a[n]; }
  template <typename T> void Once(T *&obj) { if(If(0)){obj=new T;RegisterPtr(obj);obj->Serialize(*this); } Ptr(obj); }
  template <typename T> void OnceRef(T *&obj) { if(If(0)){obj=new T;RegisterPtr(obj);obj->Serialize(*this);Ptr(obj);} else {Ptr(obj);obj->AddRef();} }
  template <typename T,typename A> void OnceRef(T *&obj,A *x) { if(If(0)){obj=new T;RegisterPtr(obj);obj->Serialize(*this,x);Ptr(obj);} else {Ptr(obj);obj->AddRef();} }
  void Bits(int *a,int *b,int *c,int *d,int *e,int *f,int *g,int *h);

  void U8(uint8_t &v)    { v = Data[0];                          Data+=1; }
  void U8(int &v)   { v = Data[0];                          Data+=1; }
  void U16(uint16_t &v)  { sUnalignedLittleEndianLoad16(Data,v); Data+=2; }
  void U16(int &v)  { v = Data[0]|(Data[1]<<8);             Data+=2; }
  void U32(uint8_t &v)   { v = Data[0];                          Data+=4; } // useful variant for streaming 8 bit variables as 32 bit stream
  void U32(uint32_t &v)  { sUnalignedLittleEndianLoad32(Data,v); Data+=4; }
  void U64(uint64_t &v)  { sUnalignedLittleEndianLoad64(Data,v); Data+=8; }
  void ArrayU8(uint8_t *ptr,int count);
  void ArrayU16(uint16_t *ptr,int count);
  void ArrayU32(uint32_t *ptr,int count);
  void ArrayU64(uint64_t *ptr,int count);
  void String(sChar *v,int maxsize);

  void S8(int8_t &v)     { U8((uint8_t &) v); }
  void S8(int &v)     { U8((int &) v); }
  void S16(int16_t &v)    { U16((uint16_t &) v); }
  void S16(int &v)    { U16((int &) v); }
  void S32(int32_t &v)    { U32((uint32_t &) v); }
  void S64(int64_t &v)    { U64((uint64_t &) v); }
  void F32(float &v)   { U32(*((uint32_t *) &v)); }
  void F64(double &v)   { U64(*((uint64_t *) &v)); }
  void ArrayS8(int8_t *ptr,int count)     { ArrayU8((uint8_t *)ptr,count);  }
  void ArrayS16(int16_t *ptr,int count)   { ArrayU16((uint16_t *)ptr,count); }
  void ArrayChar(sChar *ptr,int count) { ArrayU16((uint16_t *)ptr,count); }
  void ArrayS32(int32_t *ptr,int count)   { ArrayU32((uint32_t *)ptr,count); }
  void ArrayS64(int64_t *ptr,int count)   { ArrayU64((uint64_t *)ptr,count); }
  void ArrayF32(float *ptr,int count)   { ArrayU32((uint32_t *)ptr,count); }
  void ArrayF64(double *ptr,int count)   { ArrayU64((uint64_t *)ptr,count); }

  template <class Type> void Array(sStaticArray<Type> &a) 
  { int max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max); } 
  template <class Type> void ArrayNew(sStaticArray<Type *> &a) 
  { int max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max); 
    for(int i=0;i<max;i++) { sTAG_CALLER(); a[i]=new Type; } }
  template <class Type> void ArrayNewHint(sStaticArray<Type *> &a,int additional) 
  { int max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.HintSize(max+additional); a.Resize(max); 
    for(int i=0;i<max;i++) { sTAG_CALLER(); a[i]=new Type; } }

  template <class Type> void ArrayRegister(sStaticArray<Type *> &a) 
  {Type *e; sFORALL(a,e) RegisterPtr(e); }


  template <class Type> void ArrayAll(sStaticArray<Type> &a)
  {
    int max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max);
	  int interval = 4095/sizeof(Type);
	  int ic=0;
    for (int i=0; i<max; i++) 
	  {
		  *this | a[i];
		  if (++ic==interval)
		  {
			  Check();
			  ic=0;
		  }
	  }
  }

  template <class Type> void ArrayAllPtr(sStaticArray<Type *> &a)
  {
    int max; S32(max); sVERIFY(a.GetCount()==0); sTAG_CALLER(); a.Resize(max);
    int interval = 4095/sizeof(Type);
    int ic=0;
    for (int i=0; i<max; i++)
    { 
      sTAG_CALLER(); a[i] = new Type;
      *this | a[i]; 
      if (++ic==interval)
      {
        Check();
        ic=0;
      }
    }
  }

};

/****************************************************************************/
//note: these operators are not allowed here, because they could break serialization when a header gets changed.
//inline sWriter& operator| (sWriter &s,int16_t  v)  { s.S16(v); return s; } 
//inline sReader& operator| (sReader &s,int16_t &v)  { s.S16(v); return s; }
//inline sWriter& operator| (sWriter &s,uint16_t  v)  { s.U16(v); return s; }
//inline sReader& operator| (sReader &s,uint16_t &v)  { s.U16(v); return s; }

inline sWriter& operator| (sWriter &s,int32_t  v)  { s.S32(v); return s; } 
inline sReader& operator| (sReader &s,int32_t &v)  { s.S32(v); return s; }
inline sWriter& operator| (sWriter &s,uint32_t  v)  { s.U32(v); return s; }
inline sReader& operator| (sReader &s,uint32_t &v)  { s.U32(v); return s; }
inline sWriter& operator| (sWriter &s,float  v)  { s.F32(v); return s; }
inline sReader& operator| (sReader &s,float &v)  { s.F32(v); return s; }
inline sWriter& operator| (sWriter &s,int64_t  v)  { s.S64(v); return s; }
inline sReader& operator| (sReader &s,int64_t &v)  { s.S64(v); return s; }
inline sWriter& operator| (sWriter &s,uint64_t  v)  { s.U64(v); return s; }
inline sReader& operator| (sReader &s,uint64_t &v)  { s.U64(v); return s; }
inline sWriter& operator| (sWriter &s,double  v)  { s.F64(v); return s; }
inline sReader& operator| (sReader &s,double &v)  { s.F64(v); return s; }
inline sWriter& operator| (sWriter &s,const sChar *v) { s.String(v); return s; }
inline sReader& operator| (sReader &s,const sStringDesc &desc) { s.String(desc.Buffer,desc.Size); return s; }
inline sWriter& operator| (sWriter &s,const sRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }
inline sReader& operator| (sReader &s,      sRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }
inline sWriter& operator| (sWriter &s,const sFRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }
inline sReader& operator| (sReader &s,      sFRect &v) { s | v.x0 | v.y0 | v.x1 | v.y1; return s; }

template <class Type> inline sWriter& operator| (sWriter &s,Type *a) { a->Serialize(s); return s; }
template <class Type> inline sReader& operator| (sReader &s,Type *a) { a->Serialize(s); return s; }

/****************************************************************************/

template<class Type> Type *sLoadObject(const sChar *name)
{
  sFile *file = sCreateFile(name,sFA_READ); if(!file) return 0; 
  sPushMemLeakDesc(sFindFileWithoutPath(name));
  Type *obj = new Type;
  sReader stream; 
  stream.Begin(file); 
  obj->Serialize(stream); 
  stream.End(); 
  delete file; 
  if(!stream.IsOk())
  {
    sLogF(L"file",L"error loading <%s>, Serialize failed 1\n",name);
    sDelete(obj);
  }
  sPopMemLeakDesc();
  return obj;
}

template<class Type> sBool sLoadObject(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_READ); if(!file) return 0; 
  sPushMemLeakDesc(sFindFileWithoutPath(name));
  sReader stream; stream.Begin(file); obj->Serialize(stream); stream.End();
  if(!stream.IsOk())
    sLogF(L"file",L"error loading <%s>, Serialize failed 2\n",name);
  delete file; 
  sPopMemLeakDesc();
  return stream.IsOk(); 
}

template<class Type> sBool sSaveObject(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_WRITE); if(!file) return 0; 
  sWriter stream; stream.Begin(file); obj->Serialize(stream); stream.End(); 
  if(!stream.IsOk())
    sLogF(L"file",L"error saving <%s>, Serialize failed 3\n",name);
  delete file; return stream.IsOk(); 
}

template<class Type> sBool sLoadObjectConfig(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_READ); if(!file) return 0; 
  sPushMemLeakDesc(sFindFileWithoutPath(name));
  sReader stream; stream.Begin(file); obj->SerializeConfig(stream); stream.End();
  if(!stream.IsOk())
    sLogF(L"file",L"error loading <%s>, Serialize failed 2\n",name);
  delete file; 
  sPopMemLeakDesc();
  return stream.IsOk(); 
}

template<class Type> sBool sSaveObjectConfig(const sChar *name,Type *obj)
{
  sFile *file = sCreateFile(name,sFA_WRITE); if(!file) return 0; 
  sWriter stream; stream.Begin(file); obj->SerializeConfig(stream); stream.End(); 
  if(!stream.IsOk())
    sLogF(L"file",L"error saving <%s>, Serialize failed 3\n",name);
  delete file; return stream.IsOk(); 
}


template<class Type> sBool sSaveObjectFailsafe(const sChar *name,Type *obj)
{
  sFile *file = sCreateFailsafeFile(name,sFA_WRITE); if(!file) return 0; 
  sWriter stream; stream.Begin(file); obj->Serialize(stream); stream.End(); 
  if(!stream.IsOk())
    sLogF(L"file",L"error saving <%s>, Serialize failed 3\n",name);
  delete file; return stream.IsOk(); 
}

template <typename T> sBool sCalcObjectMD5(sChecksumMD5 &md5, T* obj)
{
  sCalcMD5File c;
  sWriter s;
  s.Begin(&c);
  obj->Serialize(s);
  if(!s.End()) return sFALSE;
  c.Close();
  md5 = c.Checksum;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   official registry for serialization ids                            ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***  - always state the enum-values explicitly!                          ***/
/***  - don't leave gaps.                                                 ***/
/***  - if you remove a class, don't delete it, comment it out!           ***/
/***  - mark id's which are reserved but not yet used, so they can be     ***/
/***    safely reassigned.                                                ***/
/***                                                                      ***/
/****************************************************************************/

namespace sSerId
{
  enum Users_
  {
    Altona                = 0x00010000,
    Farbrausch            = 0x00030000,
    Chaos                 = 0x00040000,
    Werkkzeug4            = 0x00050000,
  };

  enum Altona_
  {
    Error                 = 0x00000000,
    sImage                = 0x00010001,
    sTexture              = 0x00010002,
    sGeometry             = 0x00010003,
    sImageData            = 0x00010004,
    sColorGradient        = 0x00010005,
    sImageI16             = 0x00010006,
    sStaticArray          = 0x00010007,
    sGuiTheme             = 0x00010008,
  };


  // 0x80000000 .. 0xffffffff -> free for random numbers...
};

/****************************************************************************/


// HEADER_ALTONA_BASE_SERIALIZE
#endif


