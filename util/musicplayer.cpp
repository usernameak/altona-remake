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

#include "util/musicplayer.hpp"
#include "base/system.hpp"
#include "base/sound.hpp"
static sMusicPlayer *sMusicPlayerPtr;

/****************************************************************************/
/***                                                                      ***/
/***   MusicPlayers                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sMusicPlayer::sMusicPlayer()
{
  Stream = 0;
  StreamSize = 0;
  StreamDelete = 0;
  Status = 0;
  RewindBuffer = 0;
  RewindSize = 0;
  RewindPos = 0;
  PlayPos = 0;
}

sMusicPlayer::~sMusicPlayer()
{
  if(sMusicPlayerPtr==this)
    sClearSoundHandler();
  if(StreamDelete)
    delete[] Stream;
  if(RewindBuffer)
    delete[] RewindBuffer;
}

sBool sMusicPlayer::LoadAndCache(const sChar *name)
{
  sString<4096> buffer;
  ptrdiff_t size;
  uint8_t *mem;

  buffer = name;
  buffer.Add(L".raw");

  mem = sLoadFile(buffer,size);
  if(mem)
  {
    RewindBuffer = (int16_t *)mem;
    RewindSize = size/4;
    RewindPos = size/4;
    sVERIFY(Stream==0);
    Status = 3;
    return sTRUE;
  }
  else
  {
    if(Load(name))
    {
      Start(0);
      size = GetTuneLength() * 5;     // 25% safety for unrelyable GetTuneLength()
      mem = new uint8_t[size];
      size = Render((int16_t *)mem,size/4)*4;
      RewindBuffer = (int16_t *)mem;
      RewindSize = size/4;
      RewindPos = size/4;
      sSaveFile(buffer,mem,size);
      return sTRUE;
    }
    else
    {
      return sFALSE;
    }
  }
}

sBool sMusicPlayer::Load(const sChar *name)
{
  ptrdiff_t size;

  if(StreamDelete)
    delete[] Stream;
  Stream = sLoadFile(name,size);
  StreamSize = size;
  StreamDelete = sFALSE;
  if(Stream)
  {
    StreamDelete = sTRUE;
    Status = 1;
    return sTRUE;
  }
  return sFALSE;
}

sBool sMusicPlayer::Load(uint8_t *data,int size)
{
  if(StreamDelete)
    delete[] Stream;
  Stream = data;
  StreamSize = size;
  StreamDelete = sFALSE;
  Status = 1;
  return sTRUE;
}

void sMusicPlayer::AllocRewind(int bytes)
{
  RewindSize = bytes/4;
  RewindBuffer = new int16_t[bytes/2];
  RewindPos = 0;
}

sBool sMusicPlayer::Start(int songnr)
{
  if(Status!=0)
  {
    if(Init(songnr))
      Status = 3;
    else
      Status = 0;
  }
  return Status==3;
}

void sMusicPlayer::Stop()
{
  if(Status == 3)
    Status = 1;
}

static void sMusicPlayerSoundHandler(int16_t *samples,int count)
{
  if(sMusicPlayerPtr)
    sMusicPlayerPtr->Handler(samples,count,0x100);
}

void sMusicPlayer::InstallHandler()
{
  sVERIFY(sMusicPlayerPtr==0);
  sMusicPlayerPtr = this;
  sSetSoundHandler(44100,sMusicPlayerSoundHandler,44100);
}

sBool sMusicPlayer::Handler(int16_t *buffer,int samples,int vol)
{
  int diff,size;
  int result;
  int i;

  result = 1;
  if(Status==3)
  {
    if(RewindBuffer)
    {
      if(PlayPos+samples < RewindSize)
      {
        while(RewindPos<PlayPos+samples && result)
        {
          diff = PlayPos+samples-RewindPos;
          size = Render(RewindBuffer+RewindPos*2,diff);
          if(size==0) 
            result = 0;
          RewindPos+=size;
        }
        size = samples;
        if(PlayPos+size>RewindSize)
          size = PlayPos-RewindPos;
        if(size>0)
        {
          for(i=0;i<size*2;i++)
            buffer[i] = (RewindBuffer[PlayPos*2+i]*vol)>>8;
//          sCopyMem(buffer,RewindBuffer+PlayPos*2,size*4);
          buffer+=size*2;
          PlayPos += size;
          samples -= size;
        }
      }
      if(samples>0)
      {
        sSetMem(buffer,0,samples*4);
        Status = 1;
      }
    }
    else
    {
      while(samples>0 && result)
      {
        size = Render(buffer,samples);
        if(size==0)
        {
          result = 0;
          Status = 1;
        }
        buffer+=size*2;
        PlayPos += size;
        samples-=size;
      }
    }
  }
  else
  {
    sSetMem(buffer,0,samples*4);
  }
  return result;
}


/****************************************************************************/
