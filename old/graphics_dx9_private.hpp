/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_BASE_GRAPHICS_DX9_PRIVATE_HPP
#define FILE_BASE_GRAPHICS_DX9_PRIVATE_HPP

#include "base/types.hpp"

/****************************************************************************/

enum
{
  sMTRL_MAXTEX = 16,                                  // max number of textures
  sMTRL_MAXPSTEX = 16,                                // max number of pixel shader samplers
  sMTRL_MAXVSTEX = 4,                                 // max number of vertex shader samplers
};

/****************************************************************************/

class sGeometryPrivate 
{
};

struct sVertexFormatHandlePrivate 
{
protected:
  struct IDirect3DVertexDeclaration9 *Decl;
public: 
  struct IDirect3DVertexDeclaration9 *GetDecl() { return Decl; }    // this is only for internal use.
};

class sTextureBasePrivate 
{
protected:
  /*
  friend void sSetRendertargetPrivate(class sTextureBase *tex,int xs,int ys,const sRect *vrp,int flags,struct IDirect3DSurface9 *cubesurface);
  friend void sSetScreen(const sRect &rect,uint32_t *data);
  friend void sResolveTargetPrivate();
*/
  friend void sRender3DEnd(sBool flip);
  friend void sSetTarget(const struct sTargetPara &para);
  friend void sResolveTargetPrivate(class sTextureBase *tex);
  friend void sCopyTexture(const struct sCopyTexturePara &para);
  friend void sBeginReadTexture(const uint8_t*& data, int32_t& pitch, enum sTextureFlags& flags,class sTexture2D *tex);
  friend class sGpuToCpu;

  enum TexturePrivateResolveFlags
  {
    RF_MultiValid = 0x0001,
    RF_SingleValid = 0x0002,
    RF_Resolve = 0x0004,
  };
  union
  {
    struct IDirect3DBaseTexture9 *TexBase;
    struct IDirect3DTexture9 *Tex2D;
    struct IDirect3DCubeTexture9 *TexCube;
    struct IDirect3DVolumeTexture9 *Tex3D;
  };
  struct IDirect3DSurface9 *Surf2D;       // used only by DXBackBuffer
  struct IDirect3DSurface9 *MultiSurf2D;  // used by DXBackBuffer and multisampled rendertargets
  virtual void OnLostDevice(sBool reinit=sFALSE);
  uint32_t ResolveFlags;                      // RF_???
  uint32_t DXFormat;

  sTextureBasePrivate();
};


class sShaderPrivate 
{
  friend sShader *sCreateShader(int type,const uint8_t *code,int bytes);
  friend sShader *sCreateShaderRaw(int type,const uint8_t *code,int bytes);
  friend void sSetShaders(sShader *vs,sShader *ps,sShader *gs,sLinkedShader **link,sCBufferBase **cbuffers,int cbcount);
  friend void sCreateShader2(sShader *shader,sShaderBlob *blob);
  friend void sDeleteShader2(sShader *shader);
  friend void sSetShader2(sShader *shader);
  friend class sMaterial;
protected:
  union
  {
    struct IDirect3DVertexShader9 *vs;  // sRENDER_DX9
    struct IDirect3DPixelShader9 *ps;   // sRENDER_DX9
  };
};

class sMaterialPrivate 
{
public: // need access from specialized sToolPlatforms...
  uint32_t **States;
  int *StateCount;
};

/****************************************************************************/

struct sOccQueryNode
{
  sDNode Node;
  int Pixels;
  struct IDirect3DQuery9 *Query;
};

class sOccQueryPrivate 
{
protected:
  sOccQueryNode *Current;
  sDList2<sOccQueryNode> Queries;
};

/****************************************************************************/

class sCBufferBasePrivate 
{
public:
  void *DataPersist;
  void **DataPtr;
  uint64_t Mask;                                // used register mask
};

/****************************************************************************/

class sGfxThreadContextPrivate
{
};

/****************************************************************************/

class sGpuToCpuPrivate
{
protected:
  int SizeX,SizeY;
  int Flags;
  int DXFormat;
  sBool Locked;
  
  sTexture2D *SrcTex;
  int SrcMipLevel;
  struct IDirect3DSurface9 *Dest;
};

/****************************************************************************/

#endif // FILE_BASE_GRAPHICS_DX9_PRIVATE_HPP

