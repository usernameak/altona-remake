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

#ifndef HEADER_ALTONA_UTIL_MUSICPLAYER
#define HEADER_ALTONA_UTIL_MUSICPLAYER

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"

/****************************************************************************/

class sMusicPlayer
{
private:
  int16_t *RewindBuffer;               // remember all rendered samples
  int RewindSize;                  // max size in samples
  int RewindPos;                   // up to this sample the buffer has been rendered
protected:
  uint8_t *Stream;                      // the loaded data
  int StreamSize;
  sBool StreamDelete;

public:
  int Status;                      // 0=error, 1=ready, 2=pause, 3=playing
  int PlayPos;                     // current position for continuing rendering

  sMusicPlayer();
  virtual ~sMusicPlayer();
  sBool Load(const sChar *name);    // load from file, memory is deleted automatically
  sBool Load(uint8_t *data,int size);  // load from buffer, memory is not deleted automatically
  sBool LoadAndCache(const sChar *name);
  int GetCacheSamples() { return RewindSize; }
  int16_t *GetCacheData() { return RewindBuffer; }
  sBool IsPlaying() { return Status==3; }
  void InstallHandler();            // install a soundhandler.
  void AllocRewind(int bytes);     // allocate a rewindbuffer
  sBool Start(int songnr);         // initialize and start playing
  void Stop();                      // stop playing
  sBool Handler(int16_t *buffer,int samples,int Volume=256); // return sFALSE if all following calls to handler will just return silence

  virtual sBool Init(int songnr)=0;
  virtual int Render(int16_t *buffer,int samples)=0;   // return number of samples actually rendered, 0 for eof
  virtual int GetTuneLength() = 0; // in samples
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_MUSICPLAYER
#endif
