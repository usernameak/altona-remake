/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_MOVIEPLAYER_HPP
#define FILE_UTIL_MOVIEPLAYER_HPP

#include "base/types.hpp"

class sMaterial;
class sTextureBase;

/****************************************************************************/

struct sMovieInfo
{
  int  XSize;   // width in pixels
  int  YSize;   // height in pixels
  float  Aspect;  // actual aspect ratio (width/height)
  float  Length;  // length in seconds
  float  FPS;     // frames per second
};


class sMoviePlayer
{
public:

  /****************************************************************************/   
  /***                                                                      ***/
  /*** Simple API (guaranteed to be there)                                  ***/
  /***                                                                      ***/
  /****************************************************************************/   

  // starts playing from the beginning
  virtual void Play()=0;

  // returns if movie is (still) playing
  virtual sBool IsPlaying()=0;

  // renders movie to screen
  // the zoom parameter specifies what happens if the aspect ratios of movie
  // and screen are mismatched:
  // sFALSE: letterbox/pillarbox
  // sTRUE: zoom in and crop
  // this function might write to less than the full screen (in case of aspect
  // ratio mismatch) or even nothing (because of an error), so make sure you
  // clear the frame buffer beforehand!
  // Also, using this to render into a render target might not work under all
  // circumstances.
  virtual void RenderToScreen(sBool zoom=sFALSE);

  // sets volume. parameter is linear between 0 and 1
  virtual void SetVolume(float volume)=0;

  // close and delete the player
  virtual void Release()=0;

  /****************************************************************************/   
  /***                                                                      ***/
  /*** Advanced API (not on all platforms)                                  ***/
  /***                                                                      ***/
  /****************************************************************************/   

  // starts playing or seeks (if applicable), time is in seconds
  virtual void Play(float time) { sVERIFYFALSE; }

  // returns information about the movie
  virtual sMovieInfo GetInfo() { sMovieInfo info; sClear(info); return info; }

  // returns current movie position in seconds
  virtual float GetTime()=0;

  // pauses playback (if applicable)
  virtual void Pause()=0;          

  // overrides movie's aspect ratio in case you know better
  virtual void OverrideAspectRatio(float aspect)=0;

  // returns material for painting the current frame yourself. 
  // uvrect is filled with the needed UVs for painting the image.
  // in case of error, returns NULL
  // The material has the following properties:
  // - needs position, color 0 and UV 0
  // - respects vertex color and alpha
  // - uses sSimpleMaterial's constant buffers
  // - is valid for max. one frame!
  virtual sMaterial * GetFrame(sFRect &uvrect)  { sVERIFYFALSE; return 0; };

protected:
  sMoviePlayer() {}
  virtual ~sMoviePlayer() {}

};

enum sMovieFlags
{
  sMOF_NOSOUND      = 1<<0,  // don't play audio
  sMOF_DONTSTART    = 1<<1,  // start with black screen, wait for Play()
  sMOF_LOOP         = 1<<2,  // loop the movie automatically 
};

// creates a movie player instance. The alpha texture only applies to platforms
// implementing the advanced API.
sMoviePlayer * sCreateMoviePlayer(const sChar *filename, int flags, sTextureBase *alphatexture=0);

/****************************************************************************/

#endif // FILE_UTIL_MOVIEPLAYER_HPP

