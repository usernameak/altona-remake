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

#include "base/types.hpp"
#include "base/system.hpp"
//#include "base/graphics.hpp"
#include "base/windows.hpp"
#include "base/serialize.hpp"
#include "base/math.hpp"
#include "util/image.hpp"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define FASTHALF 0    // enable this to use a faster image.half mathod

/****************************************************************************/

static sDecompressImageDataHandler DecompressImageHandler[sICT_COUNT] = { 0 };


//void UnpackDXT(uint32_t *d32,uint8_t *s,int level,int xs,int ys);
uint64_t sTotalImageDataMem;

/****************************************************************************/
/***                                                                      ***/
/***   sImageData                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sImageData::sImageData()
{
  Format = 0;
  SizeX = 0;
  SizeY = 0;
  SizeZ = 1;
  Mipmaps = 0;
  DataSize = 0;
  UnpackedSize = 0;
  Data = 0;
  Palette = 0;
  PaletteCount = 0;
  NameId = 0;
  Quality = 1;
  CodecType = sICT_RAW;
}

sImageData::sImageData(sImage *img,int f,int ditherquality)
{
  Quality = 1;
  Format = 0;
  SizeX = 0;
  SizeY = 0;
  SizeZ = 1;
  Mipmaps = 0;
  DataSize = 0;
  UnpackedSize = 0;
  Data = 0;
  Palette = 0;
  PaletteCount = 0;
  NameId = 0;
  Quality = ditherquality;
  CodecType = sICT_RAW;

  Format = f;
  Init2(f,(f&sTEX_NOMIPMAPS)?1:0,img->SizeX,img->SizeY,1);
  ConvertFrom(img);
}

sImageData::sImageData(sImage *img,int f)
{
  Quality = 1;
  Format = 0;
  SizeX = 0;
  SizeY = 0;
  SizeZ = 1;
  Mipmaps = 0;
  DataSize = 0;
  UnpackedSize = 0;
  Data = 0;
  Palette = 0;
  PaletteCount = 0;
  NameId = 0;
  Quality = 1;
  CodecType = sICT_RAW;

  Format = f;
  Init2(f,(f&sTEX_NOMIPMAPS)?1:0,img->SizeX,img->SizeY,1);
  ConvertFrom(img);
}

sImageData::~sImageData()
{
  sTotalImageDataMem -= DataSize;
  delete[] Data;
  delete[] Palette;
}

static void sGetPixelFormatInfo(int Format,int &BitsPerPixel,int &minx,int &miny,int &PaletteCount)
{
  minx = miny = 1;
  PaletteCount = 0;

  switch(Format&sTEX_FORMAT)
  {
  case sTEX_ARGB8888:
  case sTEX_QWVU8888:
  case sTEX_MRGB8:
    BitsPerPixel = 32;
    break;
  case sTEX_A8:
  case sTEX_I8:
  case sTEX_IA4:
  case sTEX_8TOIA:
    BitsPerPixel = 8;
    break;
  case sTEX_I4:
    BitsPerPixel = 4;
    break;

  case sTEX_RGB5A3:
  case sTEX_IA8:
  case sTEX_ARGB1555:
  case sTEX_ARGB4444:
  case sTEX_RGB565:
  case sTEX_GR8:
  case sTEX_R16F:
    BitsPerPixel = 16;
    break;

  case sTEX_DXT1:
  case sTEX_DXT1A:
    BitsPerPixel = 4;
    minx = 4;
    miny = 4;
    break;

  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    BitsPerPixel = 8;
    minx = 4;
    miny = 4;
    break;

  case sTEX_INDEX8:
    BitsPerPixel = 8;
    PaletteCount = 256;
    break;

  case sTEX_INDEX4:
    BitsPerPixel = 4;
    minx = 2;
    PaletteCount = 16;
    break;

  case sTEX_ARGB32F:
    BitsPerPixel = 128;
    break;
  case sTEX_R32F:
    BitsPerPixel = 32;
    break;
  case sTEX_GR32F:
  case sTEX_ARGB16F:
  case sTEX_MRGB16:
    BitsPerPixel = 64;
    break;

  case sTEX_PCF16:
  case sTEX_DEPTH16:
  case sTEX_DEPTH16NOREAD:
    BitsPerPixel = 16;
    break;
  case sTEX_PCF24:
  case sTEX_DEPTH24:
  case sTEX_DEPTH24NOREAD:
    BitsPerPixel = 24;
    break;

  default:
    sVERIFYFALSE;    
  }
}

static int MiplevelSize(int format,int xs,int ys,int zs,int bitsPerPixel)
{
  switch(format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:   return int64_t(xs)*ys*bitsPerPixel/8;
  case sTEX_CUBE: return 6 * (int64_t(xs)*ys*bitsPerPixel/8); 
  case sTEX_3D:   return int64_t(xs)*ys*zs*bitsPerPixel/8; 
  default:        sVERIFYFALSE; return -1;
  }
  return 0; // GCC complains...
}

void sImageData::Init2(int format,int mipmaps,int xs,int ys,int zs)
{
  int mipsize = 1;
  int minx = 1;
  int miny = 1;
  int minz = 1;

  sVERIFY((format&sTEX_TYPE_MASK)!=0);

  int olddatasize = DataSize;
  int oldpalettecount = PaletteCount;

  // initialize

  Format = format;
  SizeX = xs;
  SizeY = ys;
  SizeZ = zs;
  Mipmaps = 0;
  PaletteCount = 0;
  CubeFaceSize = 0;


  // size of largest mipmap

  sGetPixelFormatInfo(Format,BitsPerPixel,minx,miny,PaletteCount);

  // find out mipmaps

  DataSize = 0;

  if(mipmaps==0) mipmaps=32;
  if (Format & sTEX_NOMIPMAPS) mipmaps=1;

  switch(Format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    mipsize = int64_t(xs)*ys*BitsPerPixel/8;
    while(Mipmaps<mipmaps && xs>=minx && ys>=miny)
    {
      DataSize += mipsize;
      xs = xs/2;
      ys = ys/2;
      mipsize = mipsize/4;
      Mipmaps++;
    }
    break;
  case sTEX_3D:
    mipsize = int64_t(xs)*ys*zs*BitsPerPixel/8;
    while(Mipmaps<mipmaps && xs>=minx && ys>=miny && zs>=minz)
    {
      DataSize += mipsize;
      xs = xs/2;
      ys = ys/2;
      zs = zs/2;
      mipsize = mipsize/8;
      Mipmaps++;
    }
    break;
  case sTEX_CUBE:
    mipsize = int64_t(xs)*ys*BitsPerPixel/8;
    while(Mipmaps<mipmaps && xs>=minx && ys>=miny)
    {
      DataSize += mipsize;
      xs = xs/2;
      ys = ys/2;
      mipsize = mipsize/4;
      Mipmaps++;
    }
    CubeFaceSize = sAlign(DataSize,4);
    DataSize = CubeFaceSize*6;
    break;
  default:
    sVERIFYFALSE;
  }

  UnpackedSize = DataSize;
  sVERIFY(Mipmaps>=1);
 
  // allocate memory, if size has changed.
  // avoid costly reallocations of large arrays!

  sTotalImageDataMem -= olddatasize;
  if(oldpalettecount!=PaletteCount || PaletteCount==0)
    sDeleteArray(Palette);
  if(olddatasize!=DataSize)
    sDeleteArray(Data);
  if(Palette==0 && PaletteCount>0)
    Palette = new uint32_t[PaletteCount];
  if(Data==0)
    Data = new uint8_t[DataSize];
  sTotalImageDataMem += DataSize;
  sVERIFY(!(Palette && PaletteCount==0));
}

void sImageData::InitCoded(int codecType,int format,int mipmaps,int xs,int ys,int zs,const uint8_t *data,int dataSize)
{
  sDeleteArray(Palette);
  sDeleteArray(Data);

  sVERIFY(codecType > sICT_RAW && codecType < sICT_COUNT); // just use Init2 for unencoded data

  Format = format;
  SizeX = xs;
  SizeY = ys;
  SizeZ = zs;

  int minx,miny,minz;

  sGetPixelFormatInfo(Format,BitsPerPixel,minx,miny,PaletteCount);
  sVERIFY(PaletteCount == 0);

  // determine number of miplevels
  minz = (format & sTEX_TYPE_MASK) == sTEX_3D ? minx : 0;

  if(mipmaps==0) mipmaps=32;
  if(Format & sTEX_NOMIPMAPS) mipmaps=1;

  Mipmaps = 0;
  UnpackedSize = 0;
  while(Mipmaps<mipmaps && (xs>>Mipmaps)>=minx && (ys>>Mipmaps)>=miny && (zs>>Mipmaps)>=minz)
  {
    UnpackedSize += MiplevelSize(format,xs>>Mipmaps,ys>>Mipmaps,zs>>Mipmaps,BitsPerPixel);
    Mipmaps++;
  }

  DataSize = dataSize;
  Data = new uint8_t[dataSize];
  if(data)
    sCopyMem(Data,data,dataSize);
  NameId = 0;
  Quality = 0;
  CodecType = codecType;
}

void sImageData::Copy(const sImageData *source)
{
  Format = source->Format;
  SizeX = source->SizeX;
  SizeY = source->SizeY;
  SizeZ = source->SizeZ;
  BitsPerPixel = source->BitsPerPixel;
  Mipmaps = source->Mipmaps;
  Quality = source->Quality;
  CodecType = source->CodecType;
  if(PaletteCount!=source->PaletteCount)
  {
    PaletteCount = source->PaletteCount;
    sDeleteArray(Palette);
    if(PaletteCount>0)
      Palette = new uint32_t[PaletteCount];
  }
  if(DataSize!=source->DataSize)
  {
    sTotalImageDataMem -= DataSize;
    DataSize = source->DataSize;
    delete[] Data;
    Data = new uint8_t[DataSize];
    sTotalImageDataMem += DataSize;
  }

  UnpackedSize = source->UnpackedSize;

  if(Palette)
    sCopyMem(Palette,source->Palette,PaletteCount*4);
  sCopyMem(Data,source->Data,DataSize);
  sVERIFY(!(Palette && PaletteCount==0));
}

void sImageData::Swap( sImageData *source)
{
  sSwap(DataSize,source->DataSize);
  sSwap(UnpackedSize,source->UnpackedSize);
  sSwap(CubeFaceSize,source->CubeFaceSize);

  sSwap(Format,source->Format);
  sSwap(SizeX,source->SizeX);
  sSwap(SizeY,source->SizeY);
  sSwap(SizeZ,source->SizeZ);
  sSwap(Mipmaps,source->Mipmaps);
  sSwap(BitsPerPixel,source->BitsPerPixel);
  sSwap(Data,source->Data);
  sSwap(PaletteCount,source->PaletteCount);
  sSwap(Palette,source->Palette);
  sSwap(NameId,source->NameId);
  sSwap(Quality,source->Quality);
  sSwap(CodecType,source->CodecType);
}

template <class streamer> void sImageData::Serialize_(streamer &stream)
{
  int version = stream.Header(sSerId::sImageData,3);
  sTotalImageDataMem -= DataSize;
  if(version)
  {
    stream | Format | SizeX | SizeY | SizeZ | Mipmaps | PaletteCount | DataSize | NameId;
    if(version >= 3)
      stream | Quality | CodecType;
    else if(stream.IsReading())
    {
      Quality = 1;
      CodecType = sICT_RAW;
    }

    if(stream.IsReading())
    {
      sVERIFY(Format & sTEX_TYPE_MASK);

      int minx,miny,pc;
      sGetPixelFormatInfo(Format,BitsPerPixel,minx,miny,pc);
      if((Format&sTEX_TYPE_MASK)==sTEX_CUBE)
        CubeFaceSize = DataSize/6;
      else
        CubeFaceSize = -1;

      UnpackedSize = 0;
      for(int i=0;i<Mipmaps;i++)
        UnpackedSize += MiplevelSize(Format,SizeX>>i,SizeY>>i,SizeZ>>i,BitsPerPixel);

      delete[] Palette;
      if(PaletteCount>0)
        Palette = new uint32_t[PaletteCount];
      else
        Palette = 0;
      delete[] Data;
      Data = new uint8_t[DataSize];
    }

    stream.ArrayU32(Palette,PaletteCount);
    if(BitsPerPixel==32 && CodecType == sICT_RAW)
      stream.ArrayU32((uint32_t *)Data,DataSize/4);
    else
      stream.ArrayU8(Data,DataSize);
    stream.Align();
    stream.Footer();
  }
  sTotalImageDataMem += DataSize;
}
void sImageData::Serialize(sWriter &s) { Serialize_(s); }

#define DEBUG_SaveAllBitmaps 0        // turn this on to see ALL 2d-textures loaded!

static int sDebugSaveAllBitmaps = 0;

void sImageData::Serialize(sReader &s) 
{ 
  Serialize_(s); 
  if(DEBUG_SaveAllBitmaps && (Format&sTEX_2D))
  {
    sImage img(SizeX,SizeY);
    ConvertTo(&img,0);
    sString<64> f;
    f.PrintF(L"c:/temp/%04d.bmp",sDebugSaveAllBitmaps++);
    img.Save(f);
  }
}

/*class sTextureBase *sImageData::CreateTexture() const
{
  sVERIFY(CodecType == sICT_RAW); // can't create textures directly from compressed images

  sTextureBase *tex=0;
  sTexture2D *tex2d;
  sTextureCube *texcube;

  switch(Format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    tex = tex2d = new sTexture2D(SizeX,SizeY,Format,Mipmaps);
    tex2d->LoadAllMipmaps(Data);
    break;
  case sTEX_CUBE:
    tex = texcube = new sTextureCube(SizeY,Format);
    texcube->LoadAllMipmaps(Data);
    break;
  default:
    sFatal(L"invalid texture format\n");
  }

  return tex;
}

class sTextureBase *sImageData::CreateCompressedTexture() const
{
  sVERIFY(CodecType == sICT_RAW); // can't create textures directly from compressed images

  sTextureBase *tex=0;
  sTexture2D *tex2d;
  sTextureCube *texcube;

  switch(Format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    { if((Format & sTEX_FORMAT)==sTEX_ARGB8888)
      { // neu komprimieren....
        //sImageData *id=new sImageData(*this);
        int format=sTEX_DXT1;
        sImageData *id=(sImageData*)this;
        id->Init2(format|sTEX_2D,0,SizeX,SizeY,0);
      }
      tex = tex2d = new sTexture2D(SizeX,SizeY,Format,Mipmaps);
      tex2d->LoadAllMipmaps(Data);
    }break;
  case sTEX_CUBE:
    tex = texcube = new sTextureCube(SizeY,Format);
    texcube->LoadAllMipmaps(Data);
    break;
  default:
    sFatal(L"invalid texture format\n");
  }

  return tex;
}
*/
/*void sImageData::UpdateTexture(sTextureBase *tex) const
{
  sVERIFY(CodecType == sICT_RAW); // can't create textures directly from compressed images

  switch(Format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    sVERIFY((Format & sTEX_TYPE_MASK)==sTEX_2D);

    ((sTexture2D*)tex)->Init(SizeX,SizeY,Format,Mipmaps);
    ((sTexture2D*)tex)->LoadAllMipmaps(Data);
    break;
  case sTEX_CUBE:
    sVERIFY((Format & sTEX_TYPE_MASK)==sTEX_CUBE);

    ((sTextureCube*)tex)->Init(SizeX,Format,Mipmaps);
    ((sTextureCube*)tex)->LoadAllMipmaps(Data);
    break;
  default:
    sFatal(L"invalid texture format %08x\n",Format);
  }
}

void sImageData::UpdateTexture2(sTextureBase *&tex) const
{
  sVERIFY(CodecType == sICT_RAW); // can't create textures directly from compressed images

  switch(Format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    if(!tex) tex = new sTexture2D;
    sVERIFY((Format & sTEX_TYPE_MASK)==sTEX_2D);

    ((sTexture2D*)tex)->Init(SizeX,SizeY,Format,Mipmaps);
    ((sTexture2D*)tex)->LoadAllMipmaps(Data);
    break;
  case sTEX_CUBE:
    if(!tex) tex = new sTextureCube;
    sVERIFY((Format & sTEX_TYPE_MASK)==sTEX_CUBE);

    ((sTextureCube*)tex)->Init(SizeX,Format,Mipmaps);
    ((sTextureCube*)tex)->LoadAllMipmaps(Data);
    break;
  default:
    sFatal(L"invalid texture format %08x\n",Format);
  }
}

/****************************************************************************/

void sImageData::ConvertFrom(const sImage *imgorig)
{
  sVERIFY(imgorig->SizeX == SizeX);
  sVERIFY(imgorig->SizeY == SizeY);
  sVERIFY((Format&sTEX_TYPE_MASK)==sTEX_2D);
  sVERIFY(CodecType == sICT_RAW); // can only write to raw textures

  const sImage *img=imgorig;
  sImage *other;
  uint8_t *d,*s;
  int m;

  s = 0;
  d = Data;
  uint16_t *d16 = (uint16_t *) Data;

  m = 0;
  for(;;)
  {
    int pc = (imgorig->SizeX >> m) * (imgorig->SizeY >> m);
    switch(Format & sTEX_FORMAT)
    {
    case sTEX_ARGB8888:
      sCopyMem(d,img->Data,pc*4);
#if 0
      if(Format&sTEX_NORMALIZE)
      {
        sVector30 v;
        for(int i=0;i<pc;i++)
        {
          v.x = d[0]-128;
          v.y = d[1]-128;
          v.z = d[2]-128;
          v.Unit();
          d[0] = uint8_t(v.x*127+128);
          d[1] = uint8_t(v.y*127+128);
          d[2] = uint8_t(v.z*127+128);
          d += 4;          
        }
      }
      else
#endif
      d+=pc*4;
      break;
    case sTEX_QWVU8888:
      {
        int8_t *s = (int8_t*) img->Data;
        for(int i=0;i<pc;i++)
        {
          d[i*4+0] = (s[2]-0x80);
          d[i*4+1] = (s[1]-0x80);
          d[i*4+2] = (s[0]-0x80);
          d[i*4+3] = (s[3]-0x80);

          s += 4;
        }
      }
#if 0
      if(Format&sTEX_NORMALIZE)
      {
        sVector30 v;
        int8_t *s = (int8_t*)d;
        for(int i=0;i<pc;i++)
        {
          v.x = s[0];
          v.y = s[1];
          v.z = s[2];
          v.Unit();
          s[0] = int8_t(v.x*127);
          s[1] = int8_t(v.y*127);
          s[2] = int8_t(v.z*127);
          s += 4;
        }
      }
#endif
      d+=pc*4;
      break;

    case sTEX_ARGB1555:
      s = (uint8_t*) img->Data;
      for(int i=0;i<pc;i++)
      {
        *d16++ = ((s[3]&0x80)<<8) | ((s[2]&0xf8)<<7) | ((s[1]&0xf8)<<2) | ((s[0]&0xf8)>>3);
        s+=4;
      }
      d += pc*2;
      break;

    case sTEX_ARGB4444:
      s = (uint8_t*) img->Data;
      for(int i=0;i<pc;i++)
      {
        *d16++ = ((s[3]&0xf0)<<8) | ((s[2]&0xf0)<<4) | ((s[1]&0xf0)) | ((s[0]&0xf0)>>4);
        s+=4;
      }
      d += pc*2;
      break;

    case sTEX_RGB565:
      s = (uint8_t*) img->Data;
      for(int i=0;i<pc;i++)
      {
        *d16++ = ((s[2]&0xf8)<<8) | ((s[1]&0xfc)<<3) | ((s[0]&0xf8)>>3);
        s+=4;
      }
      d += pc*2;
      break;

    case sTEX_RGB5A3:
      s = (uint8_t*) img->Data;
      for(int i=0;i<pc;i++)
      {
        if(s[3]<0xe0)
        {
          *d16++ = ((s[3]&0xe0)<<7)|((s[0]&0xf0)<<4)|(s[1]&0xf0)|((s[2]&0xf0)>>4);
        }
        else  // this could be better by checking both possibilities!
        {
          *d16++ = 0x8000|((s[0]&0xf8)<<7)|((s[1]&0xf8)<<2)|((s[2]&0xf8)>>3);
        }
        s+=4;
      }
      d += pc*2;
      break;

    case sTEX_A8:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i++)
      {
        *d++ = s[3];
        s+=4;
      }
      break;

    case sTEX_I8:
    case sTEX_8TOIA:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i++)
      {
        *d++ =  (11*s[0]+59*s[1]+30*s[2])/100;
        s+=4;
      }
      break;
/*
    case sTEX_A4:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i+=2)
      {
        *d++ = ((s[3]&0xf0)>>4)+(s[7]&0xf0);
        s+=8;
      }
      break;
      */
    case sTEX_I4:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i+=2)
      {
        int c0 = s[0]+s[1]+s[1]+s[2]+2;
        int c1 = s[4]+s[5]+s[5]+s[6]+2;
        *d++ = ((c0>>6)&0x0f)|((c1>>2)&0xf0);
        s+=8;
      }
      break;

    case sTEX_IA4:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i++)
      {
        *d++ = ((s[0]+s[1]+s[1]+s[2])/64)|(s[3]&0xf0);
        s+=4;
      }
      break;

    case sTEX_IA8:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i++)
      {
        *d++ = s[3];
        *d++ = (s[0]+s[1]+s[1]+s[2])/4;
        s+=4;
      }
      break;

    /*case sTEX_DXT1:
    case sTEX_DXT1A:
      sPackDXT(d,img->Data,img->SizeX,img->SizeY,Format,Quality);
      d += pc/2;
      break;
    case sTEX_DXT3:
    case sTEX_DXT5:
    case sTEX_DXT5N:
    case sTEX_DXT5_AYCOCG:
      sPackDXT(d,img->Data,img->SizeX,img->SizeY,Format,Quality);
      d += pc;
      break;*/
    case sTEX_ARGB32F:
      {
        sVector4 *dst = (sVector4*)Data;
        uint32_t *src = (uint32_t*)img->Data;
        for(int i=0;i<pc;i++)
          dst[i].InitColor(*src++);
      }

    case sTEX_GR8:
      s = (uint8_t *)img->Data;
      for(int i=0;i<pc;i++)
      {
        *d++ = s[1];
        *d++ = s[2];
        s+=4;
      }
      break;

    default:
      sVERIFYFALSE;
    }

    m++;
    if(m>=Mipmaps)
    {
      if(img!=imgorig) delete img;
      break;
    }

    other = img->Half();
    if(img!=imgorig) delete img;
    img = other;
  }
}

void sImageData::ConvertFromCube(sImage **imgptr)
{
  sVERIFY(SizeX == SizeY);
  sVERIFY((Format&sTEX_TYPE_MASK)==sTEX_CUBE);
  sVERIFY(CodecType == sICT_RAW); // can only write to raw textures

  sImage *images[6];
  sImage *other;
  uint8_t *d=0,*s=0;
  int m;


  for(int i=0;i<6;i++)
    images[i] = imgptr[i];

  for(int face=0;face<6;face++)
  {
    d = Data+CubeFaceSize*face;
    uint16_t *d16 = (uint16_t *)d;
    m = 0;
    for(;;)
    {
      int pc = images[face]->SizeX*images[face]->SizeY;
      switch(Format & sTEX_FORMAT)
      {
      case sTEX_ARGB8888:
        sCopyMem(d,images[face]->Data,pc*4);

        if(Format&sTEX_NORMALIZE)
        {
          sVector30 v;
          for(int i=0;i<pc;i++)
          {
            v.x = d[0]-128;
            v.y = d[1]-128;
            v.z = d[2]-128;
            v.Unit();
            d[0] = uint8_t(v.x*127+128);
            d[1] = uint8_t(v.y*127+128);
            d[2] = uint8_t(v.z*127+128);
            d += 4;          
          }
        }
        else
          d+=pc*4;
        break;

      case sTEX_QWVU8888:
        {
          int8_t *s = (int8_t*) images[face]->Data;
          for(int i=0;i<pc;i++)
          {
            d[i*4+0] = (s[2]-0x80);
            d[i*4+1] = (s[1]-0x80);
            d[i*4+2] = (s[0]-0x80);
            d[i*4+3] = (s[3]-0x80);

            s += 4;
          }
        }

        if(Format&sTEX_NORMALIZE)
        {
          sVector30 v;
          int8_t *s = (int8_t*)d;
          for(int i=0;i<pc;i++)
          {
            v.x = s[0];
            v.y = s[1];
            v.z = s[2];
            v.Unit();
            s[0] = int8_t(v.x*127);
            s[1] = int8_t(v.y*127);
            s[2] = int8_t(v.z*127);
            s += 4;
          }
        }
        d+=pc*4;
        break;

      case sTEX_ARGB1555:
        for(int i=0;i<pc;i++)
        {
          *d16++ = ((s[3]&0x80)<<8) | ((s[0]&0xf8)<<7) | ((s[1]&0xf8)<<2) | ((s[2]&0xf8)>>3);
          s+=4;
        }
        d += pc*2;
        break;

      case sTEX_ARGB4444:
        for(int i=0;i<pc;i++)
        {
          *d16++ = ((s[3]&0xf0)<<8) | ((s[2]&0xf0)<<4) | ((s[1]&0xf0)) | ((s[0]&0xf0)>>4);
          s+=4;
        }
        d += pc*2;
        break;

      case sTEX_A8:
        s = (uint8_t *)images[face]->Data;
        for(int i=0;i<pc;i++)
        {
          *d++ = s[3];
          s+=4;
        }
        break;

      case sTEX_I8:
      case sTEX_8TOIA:
        s = (uint8_t *)images[face]->Data;
        for(int i=0;i<pc;i++)
        {
          *d++ =  (11*s[0]+59*s[1]+30*s[2])/100;
          s+=4;
        }
        break;

      /*case sTEX_DXT1:
      case sTEX_DXT1A:
        sPackDXT(d,images[face]->Data,images[face]->SizeX,images[face]->SizeY,Format & sTEX_FORMAT,Quality);
        d += pc/2;
        break;
      case sTEX_DXT3:
      case sTEX_DXT5:
      case sTEX_DXT5N:
      case sTEX_DXT5_AYCOCG:
        sPackDXT(d,images[face]->Data,images[face]->SizeX,images[face]->SizeY,Format & sTEX_FORMAT,Quality);
        d += pc;
        break;*/
      case sTEX_ARGB32F:
        {
          sVector4 *dst = (sVector4*)d;
          uint32_t *src = (uint32_t*)images[face]->Data;
          for(int i=0;i<pc;i++)
            dst[i].InitColor(*src++);
        }
      break;

      default:
        sVERIFYFALSE;
      }

      m++;
      if(m>=Mipmaps)
      {
        if(m!=1)
          delete images[face];
        break;
      }

      other = images[face]->Half();
      if(m!=1)
        delete images[face];
      images[face] = other;
    }
  }
}

void sImageData::ConvertToCube(sImage **imgptr, int mipmap/*=0*/)const
{
  sVERIFY(mipmap>=0 && mipmap<Mipmaps);
  sVERIFY((Format&sTEX_TYPE_MASK)==sTEX_CUBE);
  sVERIFY(SizeX==SizeY);
  sVERIFY(CodecType == sICT_RAW); // no compressed cubemaps yet

  int offset = 0;

  int xs = SizeX;
  int ys = SizeY;
  
  for(int i=0;i<mipmap;i++)
  {
    offset += xs*ys*BitsPerPixel/8;
    xs = xs/2;
    ys = ys/2;
  }

  for(int f=0;f<6;f++)
  {
    sVERIFY(imgptr[f]->SizeX == xs);
    sVERIFY(imgptr[f]->SizeY == ys);
    uint8_t *data = Data+f*CubeFaceSize+offset;

    int pc = xs*ys;
    uint8_t *d = (uint8_t *)imgptr[f]->Data;
    uint32_t *d32 = (uint32_t *)imgptr[f]->Data;
    uint8_t *s = data;
    uint16_t *s16 = (uint16_t *) data;
    switch(Format & sTEX_FORMAT)
    {
    case sTEX_ARGB8888:
      sCopyMem(d,s,pc*4);
      break;

    case sTEX_QWVU8888:
      for(int i=0;i<pc;i++)
      {
        d[2] = ((int8_t*)s)[0]+0x80;
        d[1] = ((int8_t*)s)[1]+0x80;
        d[0] = ((int8_t*)s)[2]+0x80;
        d[3] = ((int8_t*)s)[3]+0x80;
        d += 4;
        s += 4;
      }
      break;

    case sTEX_ARGB1555:
      for(int i=0;i<pc;i++) // 0xf8 0x07
      {
        int r = ((*s16)>>10)&0x001f;
        int g = ((*s16)>> 5)&0x001f;
        int b = ((*s16)>> 0)&0x001f;
        d[2] = (r<<3)|(r>>2);
        d[1] = (g<<3)|(g>>2);
        d[0] = (b<<3)|(b>>2);
        d[3] = (*s16)&0x8000 ? 0xff : 0x00;
        d+=4;
        s16++;
      }
      break;

    case sTEX_ARGB4444:
      for(int i=0;i<pc;i++) // 0xf8 0x07
      {
        int a = ((*s16)>>12)&0x000f;
        int r = ((*s16)>> 8)&0x000f;
        int g = ((*s16)>> 4)&0x000f;
        int b = ((*s16)>> 0)&0x000f;
        d[0] = (b<<4)|(b);
        d[1] = (g<<4)|(g);
        d[2] = (r<<4)|(r);
        d[3] = (a<<4)|(a);
        d+=4;
        s16++;
      }
      break;

    case sTEX_RGB565:
      for(int i=0;i<pc;i++) 
      {
        int r = ((*s16)>>11)&0x001f;
        int g = ((*s16)>> 5)&0x003f;
        int b = ((*s16)>> 0)&0x001f;
        d[2] = (r<<3)|(r>>2);
        d[1] = (g<<2)|(g>>4);
        d[0] = (b<<3)|(b>>2);
        d[3] = 0xff;
        d+=4;
        s16++;
      }
      break;

    case sTEX_RGB5A3:
      for(int i=0;i<pc;i++)
      {
        uint16_t val = *s16++;
        if(val&0x8000)
        {
          *d32++ = 0xff000000
            | ((val & 0x7c00)<<9) | ((val & 0x7000)<<4)
            | ((val & 0x03e0)<<6) | ((val & 0x0380)<<1)
            | ((val & 0x001f)<<3) | ((val & 0x001c)>>2);
        }
        else
        {
          *d32++ = ((val & 0x7000)<<17) | ((val & 0x7000)<<14) | ((val & 0x6000)<<11)
            | ((val & 0x0f00)<<12) | ((val & 0x0f00)<< 8)
            | ((val & 0x00f0)<< 8) | ((val & 0x00f0)<< 4)
            | ((val & 0x000f)<< 4) | ((val & 0x000f)    );
        }
      }
      break;

    case sTEX_A8:
      for(int i=0;i<pc;i++)
      {
        d[0] = 0;
        d[1] = 0;
        d[2] = 0;
        d[3] = s[0];
        d+=4;
        s+=1;
      }
      break;

    case sTEX_I8:
      for(int i=0;i<pc;i++)
      {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[0];
        d[3] = 255;
        d+=4;
        s+=1;
      }
      break;

    case sTEX_8TOIA:
      for(int i=0;i<pc;i++)
      {
        d[0] = s[0];
        d[1] = s[0];
        d[2] = s[0];
        d[3] = s[0];
        d+=4;
        s+=1;
      }
      break;

    case sTEX_IA8:
      for(int i=0;i<pc;i++)
      {
        d[0] = s[1];
        d[1] = s[1];
        d[2] = s[1];
        d[3] = s[0];
        d+=4;
        s+=2;
      }
      break;

    case sTEX_IA4:
      for(int i=0;i<pc;i++)
      {
        int val = *s++;
        d[0] = d[1] = d[2] = (val&0x0f) | ((val&0x0f)<<4);
        d[3]               = (val&0xf0) | ((val&0xf0)>>4);
        d+=4;
      }
      break;

    case sTEX_I4:
      for(int i=0;i<pc;i+=2)
      {
        int val = *s++;
        d[0] = d[1] = d[2] = (val&0x0f) | ((val&0x0f)<<4);
        d[3]               = 0xff;
        d+=4;
        d[0] = d[1] = d[2] = (val&0xf0) | ((val&0xf0)>>4);
        d[3]               = 0xff;
        d+=4;
      }
      break;

    /*case sTEX_DXT1:
    case sTEX_DXT1A:
    case sTEX_DXT3:
    case sTEX_DXT5:
    case sTEX_DXT5N:
    case sTEX_DXT5_AYCOCG:
      UnpackDXT((uint32_t *)d,s,Format & sTEX_FORMAT,imgptr[f]->SizeX,imgptr[f]->SizeY);
      break;*/
    case sTEX_ARGB32F:
      {
        sVector4 *src = (sVector4*)data;
        for(int i=0;i<pc;i++)
          imgptr[f]->Data[i] = src[i].GetColor();
      }
      break;
    case sTEX_MRGB8:
      {
        uint32_t *src = (uint32_t*)data;
        for(int i=0;i<pc;i++)
        {
          sVector4 tmp; tmp.InitMRGB8(src[i]);
          tmp.w = 1.0f;
          imgptr[f]->Data[i] = tmp.GetColor();
        }
      }
      break;
    case sTEX_MRGB16:
      {
        uint64_t *src = (uint64_t*)data;
        for(int i=0;i<pc;i++)
        {
          sVector4 tmp; tmp.InitMRGB16(src[i]);
          tmp.w = 1.0f;
          imgptr[f]->Data[i] = tmp.GetColor();
        }
      }
      break;

    case sTEX_GR8:
      for(int i=0;i<pc;i++)
      {
        d[0] = 0;
        d[1] = s[0];
        d[2] = s[1];
        d[3] = 0xff;
        d+=4;
        s+=2;
      }
      break;

    default:
      sVERIFYFALSE;
    }
  }
}

void sImageData::ConvertTo(sImage *img,int mipmap) const
{
  if(CodecType != sICT_RAW)
  {
    sImageData *me = sDecompressAndConvertImageData(this);
    me->ConvertTo(img,mipmap);
    delete me;
    return;
  }

  sVERIFY(mipmap>=0 && mipmap<Mipmaps);
  sVERIFY((Format&sTEX_TYPE_MASK)==sTEX_2D);

  int xs = SizeX;
  int ys = SizeY;
  uint8_t *data = Data;
  
  for(int i=0;i<mipmap;i++)
  {
    data += xs*ys*BitsPerPixel/8;
    xs = xs/2;
    ys = ys/2;
  }

  sVERIFY(img->SizeX == xs);
  sVERIFY(img->SizeY == ys);

  int pc = xs*ys;
  uint8_t *d = (uint8_t *)img->Data;
  uint32_t *d32 = (uint32_t *)img->Data;
  uint8_t *s = data;
  uint16_t *s16 = (uint16_t *) data;
  switch(Format & sTEX_FORMAT)
  {
  case sTEX_ARGB8888:
    sCopyMem(d,s,pc*4);
    break;

  case sTEX_QWVU8888:
    for(int i=0;i<pc;i++)
    {
      d[2] = ((int8_t*)s)[0]+0x80;
      d[1] = ((int8_t*)s)[1]+0x80;
      d[0] = ((int8_t*)s)[2]+0x80;
      d[3] = ((int8_t*)s)[3]+0x80;
      d += 4;
      s += 4;
    }
    break;

  case sTEX_ARGB1555:
    for(int i=0;i<pc;i++) // 0xf8 0x07
    {
      int r = ((*s16)>>10)&0x001f;
      int g = ((*s16)>> 5)&0x001f;
      int b = ((*s16)>> 0)&0x001f;
      d[2] = (r<<3)|(r>>2);
      d[1] = (g<<3)|(g>>2);
      d[0] = (b<<3)|(b>>2);
      d[3] = (*s16)&0x8000 ? 0xff : 0x00;
      d+=4;
      s16++;
    }
    break;

  case sTEX_ARGB4444:
    for(int i=0;i<pc;i++) // 0xf8 0x07
    {
      int a = ((*s16)>>12)&0x000f;
      int r = ((*s16)>> 8)&0x000f;
      int g = ((*s16)>> 4)&0x000f;
      int b = ((*s16)>> 0)&0x000f;
      d[0] = (b<<4)|(b);
      d[1] = (g<<4)|(g);
      d[2] = (r<<4)|(r);
      d[3] = (a<<4)|(a);
      d+=4;
      s16++;
    }
    break;

  case sTEX_RGB565:
    for(int i=0;i<pc;i++) 
    {
      int r = ((*s16)>>11)&0x001f;
      int g = ((*s16)>> 5)&0x003f;
      int b = ((*s16)>> 0)&0x001f;
      d[2] = (r<<3)|(r>>2);
      d[1] = (g<<2)|(g>>4);
      d[0] = (b<<3)|(b>>2);
      d[3] = 0xff;
      d+=4;
      s16++;
    }
    break;

  case sTEX_RGB5A3:
    for(int i=0;i<pc;i++)
    {
      uint16_t val = *s16++;
      if(val&0x8000)
      {
        *d32++ = 0xff000000
             | ((val & 0x7c00)<<9) | ((val & 0x7000)<<4)
             | ((val & 0x03e0)<<6) | ((val & 0x0380)<<1)
             | ((val & 0x001f)<<3) | ((val & 0x001c)>>2);
      }
      else
      {
        *d32++ = ((val & 0x7000)<<17) | ((val & 0x7000)<<14) | ((val & 0x6000)<<11)
             | ((val & 0x0f00)<<12) | ((val & 0x0f00)<< 8)
             | ((val & 0x00f0)<< 8) | ((val & 0x00f0)<< 4)
             | ((val & 0x000f)<< 4) | ((val & 0x000f)    );
      }
    }
    break;

  case sTEX_A8:
    for(int i=0;i<pc;i++)
    {
      d[0] = 0;
      d[1] = 0;
      d[2] = 0;
      d[3] = s[0];
      d+=4;
      s+=1;
    }
    break;

  case sTEX_I8:
    for(int i=0;i<pc;i++)
    {
      d[0] = s[0];
      d[1] = s[0];
      d[2] = s[0];
      d[3] = 255;
      d+=4;
      s+=1;
    }
    break;

  case sTEX_8TOIA:
    for(int i=0;i<pc;i++)
    {
      d[0] = s[0];
      d[1] = s[0];
      d[2] = s[0];
      d[3] = s[0];
      d+=4;
      s+=1;
    }
    break;

  case sTEX_IA8:
    for(int i=0;i<pc;i++)
    {
      d[0] = s[1];
      d[1] = s[1];
      d[2] = s[1];
      d[3] = s[0];
      d+=4;
      s+=2;
    }
    break;

  case sTEX_IA4:
    for(int i=0;i<pc;i++)
    {
      int val = *s++;
      d[0] = d[1] = d[2] = (val&0x0f) | ((val&0x0f)<<4);
      d[3]               = (val&0xf0) | ((val&0xf0)>>4);
      d+=4;
    }
    break;

  case sTEX_I4:
    for(int i=0;i<pc;i+=2)
    {
      int val = *s++;
      d[0] = d[1] = d[2] = (val&0x0f) | ((val&0x0f)<<4);
      d[3]               = 0xff;
      d+=4;
      d[0] = d[1] = d[2] = (val&0xf0) | ((val&0xf0)>>4);
      d[3]               = 0xff;
      d+=4;
    }
    break;

  /*case sTEX_DXT1:
  case sTEX_DXT1A:
  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    UnpackDXT((uint32_t *)d,s,Format & sTEX_FORMAT,img->SizeX,img->SizeY);
    break;*/
  case sTEX_INDEX8:
    for(int i=0;i<pc;i++)
      img->Data[i]=Palette[s[i]];
    break;
  case sTEX_INDEX4:
    for(int i=0;i<pc/2;i++)
    {
      int val = s[i];
      img->Data[i*2+0]=Palette[val&15];
      img->Data[i*2+1]=Palette[val>>4];
    }
    break;
  case sTEX_ARGB32F:
    {
      sVector4 *src = (sVector4*)Data;
      for(int i=0;i<pc;i++)
        img->Data[i] = src[i].GetColor();
    }
    break;
  case sTEX_MRGB8:
    {
      uint32_t *src = (uint32_t*)Data;
      for(int i=0;i<pc;i++)
      {
        sVector4 tmp; tmp.InitMRGB8(src[i]);
        tmp.w = 1.0f;
        img->Data[i] = tmp.GetColor();
      }
    }
    break;
  case sTEX_MRGB16:
    {
      uint64_t *src = (uint64_t*)Data;
      //for(int i=0;i<pc;i++)
      //{
      //  sVector4 tmp; tmp.InitMRGB16(src[i]);
      //  tmp.w = 1.0f;
      //  img->Data[i] = tmp.GetColor();
      //}
      for(int i=0;i<pc;i++)
      {
        uint64_t in = src[i];
        
        int w = ((in >> 48) & 0xffff) + 1;
        int r = sClamp((int((in >> 32) & 0xffff) * w) >> 8,0,255);
        int g = sClamp((int((in >> 16) & 0xffff) * w) >> 8,0,255);
        int b = sClamp((int((in >>  0) & 0xffff) * w) >> 8,0,255);
        img->Data[i] = 0xff000000 | (r<<16) | (g<<8) | b;
      }
    }
    break;

  case sTEX_GR8:
    for(int i=0;i<pc;i++)
    {
      d[0] = 0;
      d[1] = s[0];
      d[2] = s[1];
      d[3] = 0xff;
      d+=4;
      s+=2;
    }
    break;

  case sTEX_R16F:
    {
      sHalfFloat h;
      for(int i=0;i<pc;i++)
      {
        h.Val = s16[i];
        int v = sClamp(int(h.Get()*255),0,255);
        d[0] = v;
        d[1] = v;
        d[2] = v;
        d[3] = 0xff;
        d+=4;
      } 
    }
    break;

  default:
    sVERIFYFALSE;
  }
}

/****************************************************************************/

void sGenerateMipmaps(sImageData *img)
{
  // currently only float and sTEX_ARGB8888 textures
  int format = img->Format&sTEX_FORMAT;
  if(format!=sTEX_ARGB8888 && format!=sTEX_ARGB32F && !sCheckMRGB(format))
    sFatal(L"currently only sTEX_ARGB8888, sTEX_ARGB32F and sTEX_MRGB supported");

  int xs = img->SizeX;
  int ys = img->SizeY;

  switch(img->Format&sTEX_FORMAT)
  {
  case sTEX_ARGB8888:
    {
      uint32_t *src = (uint32_t*)img->Data;

      for(int m=1;m<img->Mipmaps;m++)
      {
        uint32_t *dst = src+xs*ys;
        int ysn = ys/2;
        int xsn = xs/2;
        for(int y=0;y<ysn;y++)
        {
          for(int x=0;x<xsn;x++)
          {
            uint8_t *p0 = (uint8_t*) &src[x*2+y*2*xs];
            uint8_t *p1 = (uint8_t*) &src[x*2+1+y*2*xs];
            uint8_t *p2 = (uint8_t*) &src[x*2+1+y*2*xs+xs];
            uint8_t *p3 = (uint8_t*) &src[x*2+y*2*xs+xs];

            uint8_t *dst8 = (uint8_t*) &dst[y*xsn+x];

            dst8[0] = (p0[0]+p1[0]+p2[0]+p3[0])/4;
            dst8[1] = (p0[1]+p1[1]+p2[1]+p3[1])/4;
            dst8[2] = (p0[2]+p1[2]+p2[2]+p3[2])/4;
            dst8[3] = (p0[3]+p1[3]+p2[3]+p3[3])/4;
          }
        }
        src = dst;
        xs = xsn;
        ys = ysn;
      }
    }
    break;
  case sTEX_ARGB32F:
    {
      sVector4 *src = (sVector4*)img->Data;

      for(int m=1;m<img->Mipmaps;m++)
      {
        sVector4 *dst = src+xs*ys;
        int ysn = ys/2;
        int xsn = xs/2;
        for(int y=0;y<ysn;y++)
          for(int x=0;x<xsn;x++)
            dst[y*xsn+x] = (src[x*2+y*2*xs]+src[x*2+1+y*2*xs]+src[x*2+1+y*2*xs+xs]+src[x*2+y*2*xs+xs])*0.25f;

        src = dst;
        xs = xsn;
        ys = ysn;
      }
    }
    break;
  case sTEX_MRGB8:
    {
      uint32_t *src = (uint32_t*)img->Data;

      for(int m=1;m<img->Mipmaps;m++)
      {
        uint32_t *dst = src+xs*ys;
        int ysn = ys/2;
        int xsn = xs/2;
        for(int y=0;y<ysn;y++)
        {
          for(int x=0;x<xsn;x++)
          {
            sVector4 c0; c0.InitMRGB8(src[x*2+y*2*xs]);
            sVector4 c1; c0.InitMRGB8(src[x*2+1+y*2*xs]);
            sVector4 c2; c0.InitMRGB8(src[x*2+1+y*2*xs+xs]);
            sVector4 c3; c0.InitMRGB8(src[x*2+y*2*xs+xs]);
            sVector4 result = (c0+c1+c2+c3)*0.25f;
            dst[y*xsn+x] = result.GetMRGB8();
          }
        }

        src = dst;
        xs = xsn;
        ys = ysn;
      }
    }
  case sTEX_MRGB16:
    {
      uint64_t *src = (uint64_t*)img->Data;

      for(int m=1;m<img->Mipmaps;m++)
      {
        uint64_t *dst = src+xs*ys;
        int ysn = ys/2;
        int xsn = xs/2;
        for(int y=0;y<ysn;y++)
        {
          for(int x=0;x<xsn;x++)
          {
            sVector4 c0; c0.InitMRGB16(src[x*2+y*2*xs]);
            sVector4 c1; c0.InitMRGB16(src[x*2+1+y*2*xs]);
            sVector4 c2; c0.InitMRGB16(src[x*2+1+y*2*xs+xs]);
            sVector4 c3; c0.InitMRGB16(src[x*2+y*2*xs+xs]);
            sVector4 result = (c0+c1+c2+c3)*0.25f;
            dst[y*xsn+x] = result.GetMRGB16();
          }
        }

        src = dst;
        xs = xsn;
        ys = ysn;
      }
    }
    break;
  }
}

/****************************************************************************/

sImage *sDecompressImageData(const sImageData *src)
{
  sVERIFY(src->CodecType != sICT_RAW);
  sVERIFY(src->CodecType >= 0 && src->CodecType < sICT_COUNT);

  // if this assert triggers, you most likely need to #include "engine/imagecodec.hpp"
  // and then call sAddImageCodec() in your main().
  sVERIFY(DecompressImageHandler[src->CodecType] != 0); // handler must be registered!

  // decompress to sImage
  sImage *img = DecompressImageHandler[src->CodecType](src);
  return img;
}

sImageData *sDecompressAndConvertImageData(const sImageData *src)
{
  sImage *img = sDecompressImageData(src);
  if(!img)
    return 0;

  // convert to target format
  uint32_t outFmt = src->Format;
  //outFmt = (outFmt & ~sTEX_FORMAT) | sTEX_ARGB8888; // RYG DEBUG

  sImageData *out = new sImageData;
  out->Init2(outFmt,src->Mipmaps,src->SizeX,src->SizeY,src->SizeZ);
  out->Quality = src->Quality;

  sVERIFY((src->Format & sTEX_TYPE_MASK) == sTEX_2D); // only 2D right now
  out->ConvertFrom(img);

  delete img;
  return out;
}

void sSetDecompressHandler(int codecType,sDecompressImageDataHandler handler)
{
  sVERIFY(codecType >= 0 && codecType < sICT_COUNT);
  DecompressImageHandler[codecType] = handler;
}

/****************************************************************************/

/*sTextureBase *sStreamImageAsTexture(sReader &s)
{
  int version = s.Header(sSerId::sImageData,3);
  if(version==0) return 0;

  int Format,SizeX,SizeY,SizeZ,Mipmaps,PaletteCount,DataSize,NameId;
  int Quality = 1, CodecType = sICT_RAW;

  s | Format | SizeX | SizeY | SizeZ | Mipmaps | PaletteCount | DataSize | NameId;
  if(version >= 3)
    s | Quality | CodecType;

  sVERIFY(Format & sTEX_TYPE_MASK);

  if(CodecType != sICT_RAW) // compressed image
  {
    sImageData *coded = new sImageData;
    coded->InitCoded(CodecType,Format,Mipmaps,SizeX,SizeY,SizeZ,0,DataSize);
    coded->Quality = Quality;
    s.ArrayU8(coded->Data,DataSize);
    s.Align();
    s.Footer();

    sTextureBase *tb = 0;

    if(s.IsOk())
    {
      sImageData *decomp = sDecompressAndConvertImageData(coded);
      sVERIFY(decomp != 0);
      tb = decomp->CreateTexture();
      sVERIFY(tb != 0);

      tb->NameId = NameId;
      delete decomp;
    }

    delete coded;
    return tb;
  }

  sTextureBase *tb=0;
  sTexture2D *t2d;
  
  uint8_t *dest;
  int pitch;
  int bytes;
  int ys;
  int read;

  t2d = 0;
  read = 0;
  if(!s.IsOk()) return 0;
  switch(Format & sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    t2d = new sTexture2D(SizeX,SizeY,Format,Mipmaps);
    if(PaletteCount)
    {
      uint32_t *pal = (uint32_t *)t2d->BeginLoadPalette();
      s.ArrayU32(pal,PaletteCount);
      t2d->EndLoadPalette();
    }
    ys = t2d->SizeY;
    bytes = t2d->SizeX*t2d->BitsPerPixel/8;
    if(sIsFormatDXT(Format))
    {
      ys = ys/4;
      bytes = bytes*4;
    }
    for(int level=0;level<Mipmaps;level++)
    {
      if(level<t2d->Mipmaps)
      {
        t2d->BeginLoad(dest,pitch,level);
        for(int y=0;y<ys;y++)
        {
          s.ArrayU8(dest,bytes);
//          sSetMem(dest,0xff,bytes);
          dest += pitch;
          read += bytes;
        }
        t2d->EndLoad();
      }
      else
      {
        for(int y=0;y<ys;y++)
        {
          s.Skip(bytes);
        }        
      }
      bytes = bytes / 2;
      ys = ys / 2;
    }

    tb = t2d;
    break;
  case sTEX_CUBE:
    {
      sTextureCube *tc=new sTextureCube(SizeX,Format,Mipmaps);
      sVERIFY(!PaletteCount);
      int face_ys = tc->SizeXY;
      int face_bytes = face_ys*tc->BitsPerPixel/8;
      if(sIsFormatDXT(Format))
      {
        face_ys /= 4;
        face_bytes *= 4;
      }
      for(int face=0;face<6;face++)
      {
        ys = face_ys;
        bytes = face_bytes;
        for(int level=0;level<Mipmaps;level++)
        {
          if(level<tc->Mipmaps)
          {
            tc->BeginLoad((sTexCubeFace)face,dest,pitch,level);
            for(int y=0;y<ys;y++)
            {
              s.ArrayU8(dest,bytes);
              dest += pitch;
              read += bytes;
            }
            tc->EndLoad();
          }
          else
          {
            for(int y=0;y<ys;y++)
              s.Skip(bytes);
          }
          bytes /= 2;
          ys /= 2;
        }
      }
      tb = tc;
    }
    break;
  default:
    sVERIFYFALSE;
  }

  s.Align(4);
  s.Footer();
  tb->NameId = NameId;
  return s.IsOk() ? tb : 0;
}


/****************************************************************************/
/***                                                                      ***/
/***   sImage                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sImage::sImage()
{
  SizeX = 0;
  SizeY = 0;
  Data = 0;
}

sImage::sImage(int xs,int ys)
{
  SizeX = 0;
  SizeY = 0;
  Data = 0;
  Init(xs,ys);
}

sImage::~sImage()
{
  delete[] Data;
}

void sImage::Init(int xs,int ys)
{
  if (Data && SizeX == xs && SizeY == ys)
    return;

  delete[] Data;

  SizeX = xs;
  SizeY = ys;
  Data = new uint32_t[xs*ys];
}

void sImage::CopyRect(sImage *src,const sRect &destpos,int srcx,int srcy)
{
  uint32_t *dp = Data+destpos.x0+destpos.y0*SizeX;
  uint32_t *sp = src->Data + srcx + srcy*src->SizeX;
  int xs = destpos.SizeX();
  int ys = destpos.SizeY();

  sVERIFY(destpos.x0>=0);
  sVERIFY(destpos.y0>=0);
  sVERIFY(destpos.x1<=SizeX);
  sVERIFY(destpos.y1<=SizeY);
  sVERIFY(srcx>=0);
  sVERIFY(srcy>=0);
  sVERIFY(srcx+xs<=src->SizeX);
  sVERIFY(srcy+ys<=src->SizeY);

  for(int y=0;y<ys;y++)
  {
    sCopyMem(dp,sp,4*xs);
    sp += src->SizeX;
    dp += SizeX;
  }
}

template <class streamer> void sImage::Serialize_(streamer &stream)
{
  int version = stream.Header(sSerId::sImage,1);
  if(version>0)
  {
    stream | SizeX | SizeY;
    stream.Skip(2*4);

    if(stream.IsReading())
    {
      int xs=SizeX; SizeX=0;
      int ys=SizeY; SizeY=0;
      if(xs*ys==0) stream.Fail();
      Init(xs,ys);
      if(xs!=SizeX || ys!=SizeY) stream.Fail();
    }

    stream.ArrayU32(Data,SizeX*SizeY);
    stream.Footer();
  }
}
void sImage::Serialize(sWriter &stream) { Serialize_(stream); }
void sImage::Serialize(sReader &stream) { Serialize_(stream); }

/****************************************************************************/

void sImage::SwapEndian()
{
  uint32_t *ptr = Data;
  int size = SizeX*SizeY;
  for(int i=0;i<size;i++)
    sSwapEndianI(*ptr++);
}

sBool sImage::HasAlpha()
{
  for(int i=0;i<SizeX*SizeY;i++)
    if((Data[i]&0xff000000) != 0xff000000)
      return 1;
  return 0;
}

void sImage::Fill(uint32_t color)
{
  color = sSwapIfBE(color);
  for(int i=0;i<SizeX*SizeY;i++)
    Data[i] = color;
}
/*
void sImage::Fill(uint32_t srcBitmask, uint32_t color)
{
  for(int i=0;i<SizeX*SizeY;i++)
    Data[i] = (Data[i] & srcBitmask) | color;
}
*/
void sImage::SwapRedBlue()
{
  for(int i=0;i<SizeX*SizeY;i++)
  {
#if sCONFIG_LE
    Data[i] = (Data[i]&0xff00ff00) 
            | ((Data[i]&0x00ff0000)>>16) 
            | ((Data[i]&0x000000ff)<<16);
#else
    Data[i] = (Data[i]&0x00ff00ff) 
            | ((Data[i]&0xff000000)>>16) 
            | ((Data[i]&0x0000ff00)<<16);
#endif
  }
}

void sImage::Checker(uint32_t col0,uint32_t col1,int maskx,int masky)
{
  uint32_t *data = Data;
  for(int y=0;y<SizeY;y++)
    for(int x=0;x<SizeX;x++)
      *data++ = (!(x&maskx) != !(y&masky))?col0:col1;
}

void sImage::Glow(float cx,float cy,float rx,float ry,uint32_t color,float alpha,float power)
{
  uint32_t *data = Data;

  float fx,fy,r;

  for(int y=0;y<SizeY;y++)
  {
    fy = (float(y+0.5f)/SizeY-cy)/ry;
    for(int x=0;x<SizeX;x++)
    {
      fx = (float(x+0.5f)/SizeX-cx)/rx;
      r = (1-(sFPow(fx*fx+fy*fy,power)))*alpha;
      if(r>0)
        *data = sFadeColor(int(r*0x10000),*data,color);
      data++;
    }
  }
}


void sImage::Rect(float x0,float x1,float y0,float y1,uint32_t color)
{
  if (x0>x1) sSwap(x0,x1);
  if (y0>y1) sSwap(y0,y1);
  const int ix0=int(sClamp<float>(x0*SizeX,0.f,float(SizeX)));
  const int ix1=int(sClamp<float>(x1*SizeX,0.f,float(SizeX)));
  const int iy0=int(sClamp<float>(y0*SizeY,0.f,float(SizeY)));
  const int iy1=int(sClamp<float>(y1*SizeY,0.f,float(SizeY)));
  const int w=ix1-ix0;
  const int h=iy1-iy0;
  uint32_t *data=Data+iy0*SizeX+ix0;

  for (int y=0; y<h; y++)
  {
    for (int x=0; x<w; x++)
      data[x]=color;
    data+=SizeX;
  }
}


float sPerlin2DFixed(float x,float y,int start,int octaves,float falloff,int mode,int seed)
{
  int xi = int(x*0x10000);
  int yi = int(y*0x10000);
  float sum,amp;

  sum = 0;
  amp = 1.0f;

  for(int i=start;i<start+octaves && i<8;i++)
  {
    float val = sPerlin2D(xi<<i,yi<<i,(1<<i)-1,seed);
    if(mode&1)
      val = (float)sFAbs(val)*2-1;
    if(mode&2)
      val = (float)sFSin(val*sPI2F);
    sum += val * amp;

    amp *= falloff;
  }

  return sum;
}

void sImage::Perlin(int start,int octaves,float falloff,int mode,int seed,float amount)
{
  uint32_t *data = Data;
  float sx = float(1<<start)/SizeX;
  float sy = float(1<<start)/SizeY;
  for(int y=0;y<SizeY;y++)
  {
    for(int x=0;x<SizeX;x++)
    {
      float val = (sPerlin2DFixed(x*sx,y*sy,start,octaves,falloff,mode,seed)*amount+0.5f);
      int c = sClamp(int(val*255),0,255);
      *data++ = 0xff000000 | (c<<16) | (c<<8) | c;
    }
  }
}


void sImage::MakeSigned()
{
  uint8_t s[4];
  int8_t *d = (int8_t *) Data;
  for(int i=0;i<SizeX*SizeY;i++)
  {
    s[0] = d[0];
    s[1] = d[1];
    s[2] = d[2];
    s[3] = d[3];

    d[0] = (s[2]-0x80);
    d[1] = (s[1]-0x80);
    d[2] = (s[0]-0x80);
    d[3] = (s[3]-0x80);

    d += 4;
  }
}

void sImage::Copy(sImage *img)
{
  Init(img->SizeX,img->SizeY);
  sCopyMem(Data,img->Data,img->SizeX*img->SizeY*4);
}

void sImage::Add(sImage *img)
{
  sVERIFY(SizeX==img->SizeX && SizeY==img->SizeY);
  uint8_t *s = (uint8_t *) img->Data;
  uint8_t *d = (uint8_t *) Data;

  for(int i=0;i<SizeX*SizeY*4;i++)
    d[i] = sClamp(d[i]+s[i],0,255);
}

void sImage::Mul(uint32_t color)
{
  uint8_t *d = (uint8_t *) Data;
  uint8_t *col = (uint8_t*)&color;

  for(int i=0;i<SizeX*SizeY*4;i++)
    d[i] = d[i]*col[i&3]/255;
}

void sImage::ContrastBrightness(int contrast,int brightness)
{
  uint8_t *d = (uint8_t *) Data;

  int c = contrast;
  int b = brightness + 128;

  for(int i=0;i<SizeX*SizeY*4;i+=4)
  {
    d[i+0] = sClamp((((d[i+0]-128)*c)>>8)+b,0,255);
    d[i+1] = sClamp((((d[i+1]-128)*c)>>8)+b,0,255);
    d[i+2] = sClamp((((d[i+2]-128)*c)>>8)+b,0,255);
  }
}

void sImage::Mul(sImage *img)
{
  sVERIFY(SizeX==img->SizeX && SizeY==img->SizeY);
  uint8_t *s = (uint8_t *) img->Data;
  uint8_t *d = (uint8_t *) Data;

  for(int i=0;i<SizeX*SizeY*4;i++)
    d[i] = d[i]*s[i]/255;
}

void sImage::Blend(sImage *img, sBool premultiplied)
{
  sVERIFY(SizeX==img->SizeX && SizeY==img->SizeY);
  uint32_t *s = img->Data;
  uint32_t *d = Data;

  if (premultiplied)
  {
    for(int i=0;i<SizeX*SizeY;i++,s++,d++)
      *d = sAddColor(sScaleColorFast(*d,0x100-(*s>>24)),*s);
  }
  else
  {
    for(int i=0;i<SizeX*SizeY;i++,s++,d++)
      *d = sFadeColor((*s>>16)&0xff00,*d,*s);
  }

}

/****************************************************************************/

sImage *sImage::Scale(int xs,int ys) const
{
  sImage *img = new sImage;
  img->Init(xs,ys);
  img->Scale(this, sTRUE);
  return img;
}


void sImage::Scale(const sImage *src, sBool filter)
{
  sImage *dest = this;

  int xs = dest->SizeX;
  int ys = dest->SizeY;

  if(filter && xs<src->SizeX && ys<src->SizeY)  // integer box filter. will look bad for non-integral shrinks
  {
    uint8_t *d,*s;
    sRect r;
    int c[4],area;

    d = (uint8_t *)Data;
    for(int y=0;y<ys;y++)
    {
      r.y0 = sMulDiv(y+0,src->SizeY,ys);
      r.y1 = sMulDiv(y+1,src->SizeY,ys);
      r.x1 = 0;
      for(int x=0;x<xs;x++)
      {
        r.x0 = r.x1;
        r.x1 = sMulDiv(x+1,src->SizeX,xs);
        area = r.SizeX()*r.SizeY();

        c[0] = c[1] = c[2] = c[3] = 0;
        for(int yy=r.y0;yy<r.y1;yy++)
        {
          for(int xx=r.x0;xx<r.x1;xx++)
          {
            s = (uint8_t*) &src->Data[xx + yy*src->SizeX];
            c[0] += s[0];
            c[1] += s[1];
            c[2] += s[2];
            c[3] += s[3];
          }
        }
        d[0] = c[0]/area;
        d[1] = c[1]/area;
        d[2] = c[2]/area;
        d[3] = c[3]/area;
        d+=4;
      }
    }
  }
  else                      // general case, does not filter at all
  {
    uint32_t *d;
    d = dest->Data;
    for(int y=0;y<dest->SizeY;y++)
    {
      for(int x=0;x<dest->SizeX;x++)
      {
        *d++ = src->Data[(x*src->SizeX/dest->SizeX) + (y*src->SizeY/dest->SizeY)*src->SizeX];
      }
    }
  }
  
}


sImage *sImage::Half(sBool gammacorrect) const
{
  sImage *img;
  uint8_t *d;

  sVERIFY(SizeX > 1); 
  sVERIFY(SizeY > 1);

  img = new sImage;
  img->Init(SizeX/2,SizeY/2);
  d = (uint8_t *)img->Data;
#if FASTHALF
  if (gammacorrect) sVERIFYFALSE; // not yet implemented!
  if(img->SizeX>4096)  // security fallback, in case of insane resolutions...
  {
#endif
    if (gammacorrect)
    {
      for(int y=0;y<img->SizeY;y++)
      {
        for(int x=0;x<img->SizeX;x++)
        {
          uint8_t *p0 = (uint8_t *)&Data[(x*2+0) + (y*2+0)*SizeX];
          uint8_t *p1 = (uint8_t *)&Data[(x*2+1) + (y*2+0)*SizeX];
          uint8_t *p2 = (uint8_t *)&Data[(x*2+0) + (y*2+1)*SizeX];
          uint8_t *p3 = (uint8_t *)&Data[(x*2+1) + (y*2+1)*SizeX];
          for (int c=0; c<3; c++)
            *d++=(uint8_t)sSqrt((sSquare<int>(p0[c])+sSquare<int>(p1[c])+sSquare<int>(p2[c])+sSquare<int>(p3[c])+512)/4);
          *d++= (p0[3]+p1[3]+p2[3]+p3[3]+2)/4;
        }
      }
    }
    else
    {
      for(int y=0;y<img->SizeY;y++)
      {
        for(int x=0;x<img->SizeX;x++)
        {
          uint8_t *p0 = (uint8_t *)&Data[(x*2+0) + (y*2+0)*SizeX];
          uint8_t *p1 = (uint8_t *)&Data[(x*2+1) + (y*2+0)*SizeX];
          uint8_t *p2 = (uint8_t *)&Data[(x*2+0) + (y*2+1)*SizeX];
          uint8_t *p3 = (uint8_t *)&Data[(x*2+1) + (y*2+1)*SizeX];

          d[0] = (p0[0]+p1[0]+p2[0]+p3[0]+2)/4;
          d[1] = (p0[1]+p1[1]+p2[1]+p3[1]+2)/4;
          d[2] = (p0[2]+p1[2]+p2[2]+p3[2]+2)/4;
          d[3] = (p0[3]+p1[3]+p2[3]+p3[3]+2)/4;

          d+=4;
        }
      }
    }
#if FASTHALF
  } else
  { // this version is faster and a lot more cache friendlier
    uint16_t line[4096*4];
    for(int y=0;y<img->SizeY;y++)
    { uint8_t *p0 = (uint8_t *)&Data[(y*2+0)*SizeX];
      // 1st row
      for(int x=0,o=0;x<img->SizeX;x++,o+=4,p0+=8)
      { // point 1
        line[o  ] = p0[0]+p0[4];
        line[o+1] = p0[1]+p0[5];
        line[o+2] = p0[2]+p0[6];
        line[o+3] = p0[3]+p0[7];
      }
      // 2nd row
      for(int x=0,o=0;x<img->SizeX;x++,o+=4,p0+=8)
      { // point 1
        line[o  ]+= p0[0]+p0[4];
        line[o+1]+= p0[1]+p0[5];
        line[o+2]+= p0[2]+p0[6];
        line[o+3]+= p0[3]+p0[7];
      }
      // zeile jetzt wegschreiben.
      for(int x=0,o=0;x<img->SizeX;x++,o++)
      { *d++=line[o]>>2;
      }
    }
  }
#endif
  return img;
}
/*
void sImage::ExchangeChannels(IChannel channelX,IChannel channelY)
{
  if (channelX == channelY)
    return;

  int xShift = channelX*8;
  int yShift = channelY*8;

  int i  = SizeX*SizeY;
  uint32_t *d = Data;
  while (i--)
  {
    uint32_t valX = ((*d>>xShift)&0xff)<<yShift;
    uint32_t valY = ((*d>>yShift)&0xff)<<xShift;
    *d = (*d & ~((0xff<<xShift)|(0xff<<yShift))) | valX | valY;
    d++;
  }
}

void sImage::CopyChannelFrom(IChannel channelDst,IChannel channelSrc)
{
  if (channelDst == channelSrc)
    return;

  int dstShift = channelDst*8;
  int srcShift = channelSrc*8;

  int i  = SizeX*SizeY;
  uint32_t *d = Data;
  while (i--)
  {
    uint32_t valSrc = ((*d>>srcShift)&0xff)<<dstShift;
    *d = (*d & ~(0xff<<dstShift)) | valSrc;
    d++;
  }
}

void sImage::SetChannel(IChannel channelX,uint8_t val)
{
  int xShift = channelX*8;
  
  int i  = SizeX*SizeY;
  uint32_t *d = Data;
  while (i--)
  {
    uint32_t valX = val<<xShift;
    *d = (*d & ~(0xff<<xShift)) | valX;
    d++;
  }
}
*/
sImage *sImage::Copy() const
{
  sImage *img;

  img = new sImage;
  img->Init(SizeX,SizeY);
  sCopyMem(img->Data,Data,SizeX*SizeY*4);

  return img;
}

void sImage::BlitFrom(const sImage *src, int sx, int sy, int dx, int dy, int width, int height)
{
  int sstartx = sx;
  if (sstartx>=src->SizeX)
    return;

  int sstarty = sy;
  if (sstarty>=src->SizeY)
    return;

  int endx = sstartx+width;
  int endy = sstarty+height;

  if (endx>src->SizeX)
    width -= endx - src->SizeX;

  if (endy>src->SizeY)
    height -= endy - src->SizeY;

  if (sstartx<0) 
  {
    width += sstartx;
    dx    -= sstartx;
    sstartx = 0;
  }
  
  if (sstarty<0) 
  {
    height += sstarty;
    dy     -= sstarty;
    sstarty = 0;
  }

  
  int dstartx = dx;
  if (dstartx>=SizeX)
    return;
  
  
  int dstarty = dy;
  if (dstarty>=SizeY)
    return;

  endx = dstartx+width;
  endy = dstarty+height;

  if (endx>SizeX)
    width -= endx - SizeX;

  if (endy>SizeY)
    height -= endy - SizeY;


  if (dstartx<0)
  {
    width   += dstartx;
    sstartx -= dstartx;
    dstartx = 0;
  }

  if (dstarty<0)
  {
    height   += dstarty;
    sstarty  -= dstarty;
    dstarty   = 0;
  }

  if (height<0 || width <0 || sstartx<0 || sstarty<0)
    return;

  uint32_t *s = src->Data + sstarty * src->SizeX + sstartx;
  uint32_t *d =      Data + dstarty *      SizeX + dstartx;
  
  width *= 4;
  for (int i=0; i<height; i++)
  {
    sCopyMem(d,s,width);
    s += src->SizeX;
    d += SizeX;
  }
}

/****************************************************************************/

uint32_t sImage::Filter(int x,int y) const
{
  int x0 = (x>>8)&(SizeX-1);
  int y0 = (y>>8)&(SizeY-1);
  int x1 = (x0+1)&(SizeX-1);
  int y1 = (y0+1)&(SizeY-1);

  uint8_t *col00 = (uint8_t *)&Data[x0+y0*SizeX];
  uint8_t *col01 = (uint8_t *)&Data[x1+y0*SizeX];
  uint8_t *col10 = (uint8_t *)&Data[x0+y1*SizeX];
  uint8_t *col11 = (uint8_t *)&Data[x1+y1*SizeX];
  
  x &= 0xff;
  y &= 0xff;
  int f00 = (256-x)*(256-y);
  int f01 =      x *(256-y);
  int f10 = (256-x)*     y ;
  int f11 =      x *     y ;

  uint8_t col[4];

  for(int i=0;i<4;i++)
    col[i] = (col00[i]*f00 + col01[i]*f01 + col10[i]*f10 + col11[i]*f11)/0x10000;

  return col[0]+(col[1]<<8)+(col[2]<<16)+(col[3]<<24);
}

/****************************************************************************/

sBool sImage::Load(const sChar *name)
{
  uint8_t *data;
  ptrdiff_t size;
  int result;
  const sChar *ext;
  sFile *file;

  // load file

  file = sCreateFile(name,sFA_READ);
  if(!file)
    return 0;
  data = file->MapAll();
  if(data==0)
  {
    delete file;
    return 0;
  }
  size = file->GetSize();

  // interpret

  result = 0;
  ext = sFindFileExtension(name);
  if(sCmpStringI(ext,L"pic")==0) result = LoadPIC(data,size);
  if(sCmpStringI(ext,L"tga")==0) result = LoadTGA(name);
  if(sCmpStringI(ext,L"bmp")==0) result = LoadBMP(data,size);
  if(sCmpStringI(ext,L"jpg")==0) result = LoadJPG(data,size);
  if(sCmpStringI(ext,L"png")==0) result = LoadPNG(data,size);
  // done

  delete file;
  return result;
}

sBool sImage::Save(const sChar *name)
{
  const sChar *ext = sFindFileExtension(name);
  if(!sCmpStringI(ext,L"bmp")) return SaveBMP(name);
  if(!sCmpStringI(ext,L"pic")) return SavePIC(name);
  if(!sCmpStringI(ext,L"tga")) return SaveTGA(name);
  if(!sCmpStringI(ext,L"png")) return SavePNG(name);
  return 0;
}


/****************************************************************************/

/*void sImage::CopyRenderTarget()
{
  const uint8_t *data;
  int32_t pitch;
  sTextureFlags flags;

  sBeginSaveRT(data, pitch, flags);
  sVERIFY(flags == sTEX_ARGB8888);

  int xs,ys;
  sGetRendertargetSize(xs,ys);
  Init(xs,ys);
  uint8_t *dest = (uint8_t*) Data;
  for (int y=0; y<ys; y++)
  {
    const uint8_t* ptr = data+y*pitch;
    for (int x=0; x<xs; x++)
    {
      *dest++ = *ptr++;
      *dest++ = *ptr++;
      *dest++ = *ptr++;
      *dest++ = *ptr++;
    }
  }
  sEndSaveRT();
}
*/
/****************************************************************************/

void sImage::Diff(const sImage *img0, const sImage *img1)
{
  sVERIFY(img0->SizeX == img1->SizeX && img0->SizeY == img1->SizeY);
  
  Init(img0->SizeX, img0->SizeY);  

  uint8_t *src0 = (uint8_t*) img0->Data;
  uint8_t *src1 = (uint8_t*) img1->Data;
  uint8_t *dst = (uint8_t*) Data;
  for (int y=0; y<SizeY; y++)
  {
    for (int x=0; x<SizeX; x++)
    {
      *dst++ = *src0++ - *src1++;
      *dst++ = *src0++ - *src1++;
      *dst++ = *src0++ - *src1++;
      *dst++ = *src0++ - *src1++;
    }
  }
}

/****************************************************************************/

#pragma pack(push,2)
struct BMPHeader
{
  uint16_t Magic;                     // 'B'+'M'*256
  uint32_t FileSize;                  // size of the file
  uint32_t pad0;                      // unused
  uint32_t Offset;                    // offset from start to bits
              
  uint32_t InfoSize;                  // size of the info part
  uint32_t XSize;                     // Pixel Size
  uint32_t YSize;
  uint16_t Planes;                    // always 1
  uint16_t BitCount;                  // 4,8,24
  uint32_t Compression;               // always 0
  uint32_t ImageSize;                 // size in bytes or 0
  uint32_t XPelsPerMeter;             // bla
  uint32_t YPelsPerMeter;
  uint32_t ColorUsed;                 // for 8 bit 
  uint32_t ColorImportant;            // bla
  uint8_t Color[256][4];              // optional xbgr

  void FixEndian()
  {
#if sCONFIG_BE
    sSwapEndianI(Magic);
    sSwapEndianI(FileSize);
    sSwapEndianI(Offset);
    sSwapEndianI(InfoSize);
    sSwapEndianI(XSize);
    sSwapEndianI(YSize);
    sSwapEndianI(Planes);
    sSwapEndianI(BitCount);
    sSwapEndianI(Compression);
    sSwapEndianI(ImageSize);
    sSwapEndianI(ColorUsed);
    sSwapEndianI(ColorImportant);
#endif
  }

};
#pragma pack(pop)

sBool sImage::LoadBMP(const uint8_t *data,int size)
{
  BMPHeader *hdr;
  int x,y;
  uint8_t *d;
  const uint8_t *s;
  uint8_t val;
  
  // check for sane file

  if(size<0x36) return 0;

  hdr = (BMPHeader *) data;

  hdr->FixEndian();

  if(hdr->XSize<1 || 
      hdr->YSize<1 || 
      hdr->Planes!=1 ||
      hdr->Magic != 'B'+'M'*256 ||
      hdr->Compression != 0)
  {
    return 0;
  }

  if(int(hdr->Offset+(hdr->XSize*hdr->YSize*hdr->BitCount/8)) > size)
    return 0;

  // initialize bitmap

  Init(hdr->XSize,hdr->YSize);
  s = data+hdr->Offset;

  switch(hdr->BitCount)
  {
  case 8:
    if(size<int(sizeof(BMPHeader))) return 0;  // space for palette

    for(y=0;y<SizeY;y++)
    {
      d = (uint8_t*)(Data + (SizeY-1-y)*SizeX);
      for(x=0;x<SizeX;x++)
      {
        val = *s++;
        *d++ = hdr->Color[val][0];
        *d++ = hdr->Color[val][1];
        *d++ = hdr->Color[val][2];
        *d++ = 255;
      }
    }
    break;

  case 24:
    for(y=0;y<SizeY;y++)
    {
      d = (uint8_t*)(Data + (SizeY-1-y)*SizeX);
      for(x=0;x<SizeX;x++)
      {
        *d++ = s[0];
        *d++ = s[1];
        *d++ = s[2];
        *d++ = 255;
        s+=3;
      }
    }
    break;

  case 32:    // this is not like the BMP standard, but it's what photoshop does
    for(y=0;y<SizeY;y++)
    {
      d = (uint8_t*)(Data + (SizeY-1-y)*SizeX);
      for(x=0;x<SizeX;x++)
      {
        *d++ = s[0];
        *d++ = s[1];
        *d++ = s[2];
        *d++ = s[3];
        s+=4;
      }
    }
    break;

  default:
    sLogF(L"ERROR",L"can't load bmp with %d bits per pixel\n",hdr->BitCount);
    return 0;
  }

  SwapIfBE();
  return 1;
}

int sImage::SaveBMP(uint8_t *data, int size)
{
  int bpr=sAlign(24*SizeX/8,4);
  int Size = sizeof(BMPHeader)+SizeY*bpr;
  if (size<Size) return 0;

  uint8_t *ptr = data;

  BMPHeader *hdr = (BMPHeader*) data;
  hdr->Magic = 0x4d42;
  hdr->FileSize = (sizeof(BMPHeader)+bpr*SizeY);
  hdr->pad0 = 0;
  hdr->Offset = sizeof(BMPHeader);
  hdr->InfoSize = sizeof(BMPHeader)-1038;
  hdr->XSize = SizeX;
  hdr->YSize = SizeY;
  hdr->Planes = 1;
  hdr->BitCount = 24;
  hdr->Compression = 0;
  hdr->ImageSize = 0;
  hdr->XPelsPerMeter = 0;
  hdr->YPelsPerMeter = 0;
  hdr->ColorUsed = 0;
  hdr->ColorImportant = 0;

  sSetMem(hdr->Color, 0, sizeof(hdr->Color));

  hdr->FixEndian();

  ptr += sizeof(BMPHeader);

// copy image
  uint8_t *src=0;
  for(int y=SizeY-1;y>=0;y--)
  {
    src=(uint8_t*)(Data+y*SizeX);
    for(int x=0;x<SizeX;x++)
    {
      ptr[x*3+0] = src[0];
      ptr[x*3+1] = src[1];
      ptr[x*3+2] = src[2];
      src+=4;
    }
    ptr+=bpr;
  }

  sVERIFY(ptr == data + Size);

  return Size;
}


sBool sImage::SaveBMP(const sChar *name)
{
  SwapIfBE();

  int bpr=sAlign(24*SizeX/8,4);
  int Size = sizeof(BMPHeader)+SizeY*bpr;
  uint8_t *Convert = new uint8_t[Size];
  SaveBMP(Convert,Size);

  sBool result = sSaveFile(name,Convert,Size);
  delete[] Convert;

  SwapIfBE();
  return result;
}

sBool sImage::SavePNG(const sChar *name)
{
  sBool ok = 0;
  int len;

  uint8_t *data = new uint8_t[SizeX*SizeY*4];
  const uint8_t *src = (const uint8_t *) Data;
  for(int i=0;i<SizeX*SizeY;i++)
  {
    data[i*4+0] = src[i*4+2];
    data[i*4+1] = src[i*4+1];
    data[i*4+2] = src[i*4+0];
    data[i*4+3] = src[i*4+3];
  }

  unsigned char *png = stbi_write_png_to_mem((unsigned char *)data,SizeX*sizeof(uint32_t),SizeX,SizeY,4,&len);
  if(png)
  {
    ok = sSaveFile(name,png,len);
    delete[] png;
  }
  delete[] data;
  return ok;
}

/****************************************************************************/

static uint32_t Swap32(const uint8_t *&scan)
{
  uint32_t val;
  val = (scan[0]<<24)|(scan[1]<<16)|(scan[2]<<8)|(scan[3]);
  scan+=4;
  return val;
}

static uint32_t Swap16(const uint8_t *&scan)
{
  uint32_t val;
  val = (scan[0]<<8)|(scan[1]);
  scan+=2;
  return val;
}

sBool sImage::LoadPIC(const uint8_t *data,int size)
{
  uint8_t *d;
  int xs,ys,x,y;
  sBool hasalpha;
  int count;
  int i;
  sBool ok;
  const uint8_t *end = data+size;

  ok = 1;

  if(Swap32(data)!=0x5380f634) ok = sFALSE;
  data+=84;
  if(*data++!='P') ok = sFALSE;
  if(*data++!='I') ok = sFALSE;
  if(*data++!='C') ok = sFALSE;
  if(*data++!='T') ok = sFALSE;
  xs = Swap16(data);
  ys = Swap16(data);
  data+=8;
  if(ok)
    Init(xs,ys);

  hasalpha = *data++;
  sBool compressed = sTRUE;
  if(*data++!=8) ok = sFALSE;           // bits per channel
  if(*data++!=2) compressed = sFALSE;    // compressed/uncompressed
  if(*data++!=0xe0) ok = sFALSE;        // channelcode
  if(hasalpha)
  {
    if(*data++!=0) ok = sFALSE;
    if(*data++!=8) ok = sFALSE;
    if((*data++!=2) && compressed) ok = sFALSE;
    if(*data++!=0x10) ok = sFALSE;
  }

  if(ok)
  {
    if(compressed)
    {
      for(y=0;y<ys;y++)
      {
        d = (uint8_t *)(Data + y*SizeX);   // color
        x = 0;
        while(x<xs)
        {
          count = *data++;
          if(count>=128)              // run
          {
            if(count==128)
              count = Swap16(data);
            else
              count = count-127;      

            // Range check for RLE encoded images:
            if ((end - data) < 3) goto failed;

            for(i=0;i<count;i++)
            {
              d[0] = data[2];
              d[1] = data[1];
              d[2] = data[0];
              d[3] = 255;
              d+=4;
            }
            data+=3;
          }
          else                        // copy
          {
            count++;

            // Range-Check for broken RLE encoded images:
            if ((end - data) < 3) goto failed;

            for(i=0;i<count;i++)
            {
              d[0] = data[2];
              d[1] = data[1];
              d[2] = data[0];
              d[3] = 255;
              d+=4;
              data+=3;            
            }
          }
          x+=count;
        }
        sVERIFY(x==xs);

        if(hasalpha)
        {
          d = (uint8_t *)(Data + y*SizeX);   // alpha
          x = 0;
          while(x<xs)
          {
            count = *data++;
            if(count>=128)              // run
            {
              if(count==128)
                count = Swap16(data);
              else
                count = count-127;      
              for(i=0;i<count;i++)
              {
                d[3] = data[0];
                d+=4;
              }
              data+=1;
            }
            else                        // copy
            {
              count++;
              for(i=0;i<count;i++)
              {
                d[3] = data[0];
                d+=4;
                data+=1;            
              }
            }
            x+=count;
          }
          sVERIFY(x==xs);
        }
      }

      // we sometimes have .pic with additional data
      // seem to be meta data, so just skip it and give a warning
      if(data!=end)
        sPrintWarningF(L"sImage::LoadPIC: ignoring additional %Kb of data\n",ptrdiff_t(end-data));
    }
    else
    {
      // uncompressed
      for(y=0;y<ys;y++)
      {
        uint8_t *dest = (uint8_t*) (&Data[y*SizeX]);
        for(x=0;x<xs;x++)
        {
          dest[x*4+2] = *data++;
          dest[x*4+1] = *data++;
          dest[x*4+0] = *data++;
        }
        for(x=0;x<xs;x++)
          dest[x*4+3] = *data++;
      }
    }
  }

  SwapIfBE();
  return ok;
failed:
  // This place is reached if we've tried to read past the end of the
  // raw image buffer (for RLE only currently)
  return sFALSE;
}

uint16_t saveantiintel(uint16_t x) {return (((x&0x00ff)<<8)|((x&0xff00)>>8));}  

sBool sImage::SavePIC(const sChar * name)
{
#define NO_FIELD    0
#define ODD_FIELD   1
#define EVEN_FIELD  2
#define FULL_FRAME  3

#define UNCOMPRESSED     0x0;
#define MIXED_RUN_LENGTH 0x2;

#define RED_CHANNEL   0x80
#define GREEN_CHANNEL 0x40
#define BLUE_CHANNEL  0x20
#define ALPHA_CHANNEL 0x10

  struct channelinfosect
  {
    uint8_t   chained;    //0 -> last, 1 -> another packet
    uint8_t   size;       //number of bits for each pixel value
    uint8_t   type;       //data and encoding type
    uint8_t   channel;    //channel code
  };


  struct PicsInfoSect
  {
    uint32_t magic;     //0x5380f634
    uint32_t version;
    uint8_t  comment[80];
    uint8_t  id[4];    //PICT
    uint16_t width;
    uint16_t height;
    float ratio; //pixel aspect ratio
    uint16_t fields;
    uint16_t pad;   //unused => 0 

    channelinfosect channelinfo[2];
  };

  int picSize;
  PicsInfoSect header;
  int x,y;
  uint32_t pos;

  uint8_t *picData;
  uint8_t *imgData = (uint8_t*)Data;

  sFile *file=sCreateFile(name,sFA_WRITE);
  if(!file)
    return sFALSE;

  //  header.magic=0x5380f634;
  header.magic=0x34f68053;
  header.version=0x14ae2740;
  header.comment[0] = '4';
  header.comment[1] = '9';
  header.comment[2] = 'G';
  header.comment[3] = 'a';
  header.comment[4] = 'm';
  header.comment[5] = 'e';
  header.comment[6] = 's';
  header.comment[7] = 0;
  header.id[0]='P';
  header.id[1]='I';
  header.id[2]='C';
  header.id[3]='T';
  header.width=saveantiintel(SizeX);
  header.height=saveantiintel(SizeY);
  header.ratio=1.0f;
  header.fields=saveantiintel(FULL_FRAME);
  header.pad=0;
  header.channelinfo[0].chained=1;
  header.channelinfo[0].size=8;
  header.channelinfo[0].type=UNCOMPRESSED;
  header.channelinfo[0].channel=RED_CHANNEL|GREEN_CHANNEL|BLUE_CHANNEL;
  header.channelinfo[1].chained=0;
  header.channelinfo[1].size=8;
  header.channelinfo[1].type=UNCOMPRESSED;
  header.channelinfo[1].channel=ALPHA_CHANNEL;

  picSize=sizeof(header)+SizeX*SizeY*4;
  picData=new uint8_t[picSize];

  sCopyMem(picData,&header,sizeof(header));

  pos=sizeof(header);

  for (y=0;y<SizeY;y++)
  {
    for (x=0;x<SizeX;x++)
    {
      picData[pos++]=imgData[(y*SizeX*4)+(x*4)+2];
      picData[pos++]=imgData[(y*SizeX*4)+(x*4)+1];
      picData[pos++]=imgData[(y*SizeX*4)+(x*4)+0];
    }
    for (x=0;x<SizeX;x++)
    {
      picData[pos++]=imgData[(y*SizeX*4)+(x*4)+3];
    }
  }

  // save to disk
  //sBool result = sSaveFile(name,picData,picSize);
  sBool result = file->Write(picData,picSize);
  delete file;

  // clean up
  delete[] picData;

  return result;
}


struct sTGAHead
{
  uint8_t ID;                         // has to be 0
  uint8_t colorMapType;               // has to be 0
  uint8_t imageTypeCode;              // has to be 2
  uint8_t colorMapOrigin[2];          // have to be 0
  uint8_t colorMapLength[2];          // have to be 0
  uint8_t colorMapEntrySize;          // has to be 0
  uint8_t xOrigin[2];                 // has to be 0
  uint8_t yOrigin[2];                 // has to be 0
  uint16_t width;                     // width of the picture in pixel, plz split your 16 bit values in 2 parts
  uint16_t height;                    // height of the picture in pixel, plz split your 16 bit values in 2 parts
  uint8_t pixelSize;                  // 32 bit for BGRA, 24 for BGR, other types we do not support
  uint8_t imageDescriptorByte;        // has to be 0
};

sBool sImage::LoadTGA(const sChar *name)
{
  sFile *file = sCreateFile(name,sFA_READ);

  sTGAHead tgaHead;
  uint8_t *data = file->MapAll();
  sCopyMem(&tgaHead,data,18);
  tgaHead.width = sSwapIfBE(tgaHead.width);
  tgaHead.height = sSwapIfBE(tgaHead.height);

  if (tgaHead.colorMapType != 0 || tgaHead.imageTypeCode != 2 || (tgaHead.pixelSize != 24 && tgaHead.pixelSize != 32))
  {
    sDPrintF(L"Unsupported TGA format. We only support uncompressed RGB / RGBA images\n");
    delete file;
    return sFALSE;
  }

  data += 18;

  Init(tgaHead.width, tgaHead.height);

  if (tgaHead.pixelSize == 24)
  {    
    for(int i=SizeY-1; i>-1; i--)
    {
      uint8_t *ptr = (uint8_t*)(&Data[i*SizeX]);

      for (int j = 0; j < SizeX; ++j)
      {
        *ptr++ = *data++;
        *ptr++ = *data++;
        *ptr++ = *data++;
        *ptr++ = 255;
      }
    }
  }
  else
  {
    for(int i=SizeY-1; i>-1; i--)
    {
      sCopyMem(&Data[i*SizeX],data,SizeX*4);
      data += SizeX*4;
    }
  }  

  SwapIfBE();
  delete file;
  return sTRUE;
}

sBool sImage::SaveTGA(const sChar *name)
{
  sTGAHead tgaHead;
  tgaHead.ID = 0; // no ID
  tgaHead.colorMapType = 0; // no colormap
  tgaHead.imageTypeCode = 2; // RGB(24Bit) no compression
  tgaHead.colorMapOrigin[0] = 0; tgaHead.colorMapOrigin[1] = 0; // no colormap
  tgaHead.colorMapLength[0] = 0; tgaHead.colorMapLength[1] = 0; // no colormap
  tgaHead.colorMapEntrySize = 0; // no colormap
  tgaHead.xOrigin[0] = 0; tgaHead.xOrigin[1] = 0; // left
  tgaHead.yOrigin[0] = 0; tgaHead.yOrigin[1] = 0; // down
  tgaHead.width = SizeX; // width of the picture in pixel
  tgaHead.height = SizeY; // height of the picture in pixel
  tgaHead.pixelSize = 32; // 32 bits per pixel (ARGB)
  tgaHead.imageDescriptorByte = 0; // standard value

  sFile *file=sCreateFile(name,sFA_WRITE);

  file->Write(&tgaHead, 18);

  for(int i=SizeY-1; i>-1; i--)
  {
    uint8_t *tempArray = new uint8_t[SizeX*4];
    for(int j=0; j<SizeX; j++)
    {
      tempArray[j*4] = 0x000000ff & Data[i*SizeX+j];
      tempArray[j*4+1] = (0x0000ff00 & Data[i*SizeX+j])/0x00000100;
      tempArray[j*4+2] = (0x00ff0000 & Data[i*SizeX+j])/0x00010000;
      tempArray[j*4+3] = Data[i*SizeX+j]/0x01000000;
    }
    file->Write(tempArray, SizeX*4);
    delete[] tempArray;
  }

  delete file;

  return true;
}

/****************************************************************************/

typedef unsigned char stbi_uc;

extern "C" 
{
  /*extern stbi_uc *stbi_load_from_memory(stbi_uc *buffer, int len, int *x, int *y, int *comp, int req_comp);
  extern void     stbi_image_free      (stbi_uc *retval_from_stbi_load);*/
  void *sStbAlloc(int size)
  {
    return sAllocMem(size,16,sAMF_DEFAULT|sAMF_ALT);
  }

  void sStbFree(void *ptr)
  {
    return sFreeMem(ptr);
  }
}

#include "util/stb_image.h"

sBool sImage::LoadJPG(const uint8_t *data,int size)
{
  int x,y;
  int comp;

  stbi_uc *image = stbi_load_from_memory((stbi_uc  *)data,size,&x,&y,&comp,4);
  if(!image)
    return 0;
  Init(x,y);
  sCopyMem(Data,image,x*y*4);
  SwapRedBlue();
  SwapIfBE();
  stbi_image_free(image);
  return 1;
}

sBool sImage::LoadPNG(const uint8_t *data,int size)
{
  return LoadJPG(data,size);
}

/****************************************************************************/
/***                                                                      ***/
/***   helper functions                                                   ***/
/***                                                                      ***/
/****************************************************************************/

/*class sTexture2D *sLoadTexture2D(const sChar *name,int formatandflags)
{
  sImage *img;
  sTexture2D *tex;

  img = new sImage;
  if(!img->Load(name))
  {
    delete img;
    return 0;
  }

  tex = sLoadTexture2D(img,formatandflags);

  delete img;

  return tex;
}

/****************************************************************************/

/*class sTexture2D *sLoadTexture2D(const sImage *img,int formatandflags)
{
  sImageData *id;
  sTexture2D *tex;

  if((formatandflags & sTEX_TYPE_MASK)==0)
    formatandflags |= sTEX_2D;

  sVERIFY((formatandflags & sTEX_TYPE_MASK)==sTEX_2D);
  tex = new sTexture2D(img->SizeX,img->SizeY,formatandflags,(formatandflags & sTEX_NOMIPMAPS)?1:0);

  sBeginAlt();
  id = new sImageData;
  id->Init2(formatandflags,(formatandflags & sTEX_NOMIPMAPS)?1:0,img->SizeX,img->SizeY,1);
  id->ConvertFrom(img);
  sEndAlt();
//  id = img->Convert(formatandflags,(formatandflags & sTEX_NOMIPMAPS)?1:0);

  tex->LoadAllMipmaps(id->Data);

  delete id;

  return tex;
}

class sTexture2D *sLoadTexture2D(const sImageData *img)
{
  sTexture2D *tex;

  tex = new sTexture2D(img->SizeX,img->SizeY,img->Format,img->Mipmaps);
  tex->LoadAllMipmaps(img->Data);

  return tex;
}

/****************************************************************************/

/*class sTextureCube *sLoadTextureCube(const sImageData *img)
{
  sTextureCube *tex;

  tex = new sTextureCube(img->SizeX,img->Format,img->Mipmaps);
  tex->LoadAllMipmaps(img->Data);

  return tex;
}

/****************************************************************************/

/*class sTextureCube *sLoadTextureCube(const sChar *t0,const sChar *t1,const sChar *t2,const sChar *t3,const sChar *t4,const sChar *t5,int formatandflags)
{
  sImage *img[6];
  sTextureCube *tex=0;
  sImageData *data=0;
  int mipmaps = 0;
  int size = 0;

  for(int i=0;i<6;i++)
    img[i] = new sImage;
  if(!img[0]->Load(t0)) goto error;
  if(!img[1]->Load(t1)) goto error;
  if(!img[2]->Load(t2)) goto error;
  if(!img[3]->Load(t3)) goto error;
  if(!img[4]->Load(t4)) goto error;
  if(!img[5]->Load(t5)) goto error;
  size = img[0]->SizeX;
  for(int i=0;i<6;i++)
  {
    if(img[i]->SizeX!=size) goto error;
    if(img[i]->SizeY!=size) goto error;
  }

  if(formatandflags & sTEX_NOMIPMAPS) mipmaps = 1;
  data = new sImageData;
  data->Init2(formatandflags,mipmaps,size,size,6);
  data->ConvertFromCube(img);

  tex = (sTextureCube *)data->CreateTexture();


  for(int i=0;i<6;i++)
    delete img[i];
  delete data;
  return tex;
error:
  for(int i=0;i<6;i++)
    delete img[i];
  delete data;
  return 0;
}
*/
/****************************************************************************/

/****************************************************************************/

int sDiffImage(sImage *dimg, const sImage *img0, const sImage *img1, uint32_t mask/*=0xffffffff*/)
{
  sVERIFY(img0->SizeX == img1->SizeX && img0->SizeY == img1->SizeY);
  dimg->Init(img0->SizeX, img1->SizeY);

  int errors = 0;

  uint8_t *src0 = (uint8_t*) img0->Data;
  uint8_t *src1 = (uint8_t*) img1->Data;
  uint8_t *dest = (uint8_t*) dimg->Data;
  uint32_t *dest32 = dimg->Data;
  for (int y=0; y<img0->SizeY; y++)
  {
    for (int x=0; x<img0->SizeX; x++)
    {
      *dest++ = *src0++ - *src1++;
      *dest++ = *src0++ - *src1++;
      *dest++ = *src0++ - *src1++;
      *dest++ = *src0++ - *src1++;
      *dest32 &= mask;
      if (*dest32++)
        errors++;
    }
  }

  return errors;
}


/****************************************************************************/

// code for converter based on
// http://www.ati.com/developer/sdk/radeonSDK/html/Tools/ToolsPlugIns.html

static inline int GetRed(uint32_t col) { return (col >> 16) & 0xff; }
static inline int Pack8(float val)  { return int((val+1.0f)*127.5f) & 0xff; }

void Heightmap2Normalmap(sImage *destimage, const sImage *src, float scale/*= 1.0f*/)
{
  int width = src->SizeX;
  int height = src->SizeY;
  uint32_t *data = src->Data;

  destimage->Init(src->SizeX, src->SizeY);
  uint32_t *dest = destimage->Data;
  
  scale /= 255.0f; // normalize by max pixel magnitude

  for(int y = 0; y < height; y++)
  {
    int ym1w = ((y-1+height) % height) * width;  // (y-1)*width
    int yp0w = y * width;                        // (y+0)*width 
    int yp1w = ((y+1) % height) * width;         // (y+1)*width

    for(int x = 0, xm1 = width-1; x < width; xm1 = x, x++)
    {
      int xp0 = x;                       // x+0
      int xp1 = (x+1) < width ? x+1 : 0; // x+1 with wrap

      // Y sobel filter
      int dy =     (GetRed(data[ym1w + xm1]) - GetRed(data[yp1w + xm1]))
              + 2 * (GetRed(data[ym1w + xp0]) - GetRed(data[yp1w + xp0]))
              +     (GetRed(data[ym1w + xp1]) - GetRed(data[yp1w + xp1]));

      // X sobel filter
      int dx =     (GetRed(data[ym1w + xp1]) - GetRed(data[ym1w + xm1]))
              + 2 * (GetRed(data[yp0w + xp1]) - GetRed(data[yp0w + xm1]))
              +     (GetRed(data[yp1w + xp1]) - GetRed(data[yp1w + xm1]));

      // Cross Product of components of gradient reduces to
      float nx = -dx * scale;
      float ny = -dy * scale;
      float nz = 1.0f;
      
      // Normalize
      float invlen = 1.0f/sFSqrt(nx*nx + ny*ny + nz*nz);
      nx *= invlen;
      ny *= invlen;
      nz *= invlen;

      dest[yp0w + xp0] = (data[yp0w + xp0] & 0xff000000)
                       | (Pack8(nx) << 16)
                       | (Pack8(ny) <<  8)
                       | (Pack8(nz) <<  0);
    }
  }
}

/****************************************************************************/

sImageData *sMergeNormalMaps(const sImageData *img0, const sImageData *img1)
{
  if(img0->SizeX!=img1->SizeX || img0->SizeY!=img1->SizeY)
    return 0;

  sVERIFY((img0->Format&sTEX_FORMAT)==sTEX_ARGB8888);
  sVERIFY((img1->Format&sTEX_FORMAT)==sTEX_ARGB8888);
  sVERIFY(img0->SizeX==img1->SizeX);
  sVERIFY(img0->SizeY==img1->SizeY);
  sVERIFY(img0->Mipmaps==img1->Mipmaps);
  sVERIFY(img0->CodecType==sICT_RAW && img1->CodecType==sICT_RAW);

  sImageData *result = new sImageData;
  result->Init2(img0->Format,img0->Mipmaps,img0->SizeX,img0->SizeY,1);

  uint32_t *dest = (uint32_t*)result->Data;
  const uint32_t *src0 = (const uint32_t*)img0->Data;
  const uint32_t *src1 = (const uint32_t*)img1->Data;

  for(int i=0,iend=result->GetByteSize()/sizeof(uint32_t);i<iend;i++)
  {
    sVector30 nrm0; nrm0.InitColor(*src0);
    nrm0 = nrm0*2.0f - sVector30(1,1,1);
    nrm0.Unit();
    sVector30 nrm1; nrm1.InitColor(*src1);
    nrm1 = nrm1*2.0f - sVector30(1,1,1);
    nrm1.Unit();

    sMatrix34 trn;
    trn.k = nrm0;
    float c0 = sVector30(0,1,0)^nrm0;
    float c1 = sVector30(1,0,0)^nrm0;
    if(sFAbs(c0)<sFAbs(c1))
    {
      trn.i = sVector30(0,1,0)%nrm0;
      trn.i.Unit();
      trn.j = nrm0%trn.i;
    }
    else
    {
      trn.j = nrm0%sVector30(1,0,0);
      trn.j.Unit();
      trn.i = trn.j%nrm0;
    }

    sVector30 nrm = nrm1*trn;
    nrm = nrm*0.5f+sVector30(0.5f,0.5f,0.5f);
    uint32_t dst_nrm = nrm.GetColor();
    *dest++ = dst_nrm;
    src0++;
    src1++;
  }
  return result;
}

/****************************************************************************/

void sCompressMRGB(sImageData *dst_img, int format, const sImageData *src_img)
{
  sVERIFY((src_img->Format&sTEX_FORMAT)==sTEX_ARGB32F);
  sVERIFY(src_img->CodecType==sICT_RAW);
  sVERIFY((format&sTEX_FORMAT)==sTEX_MRGB8 || (format&sTEX_FORMAT)==sTEX_MRGB16);

  dst_img->Init2((src_img->Format&~sTEX_FORMAT)|(format&sTEX_FORMAT),src_img->Mipmaps,src_img->SizeX,src_img->SizeY,src_img->SizeZ);
  int iend = src_img->GetByteSize()/sizeof(sVector4);

  const sVector4* src = (sVector4*) src_img->Data;
  if((format&sTEX_FORMAT)==sTEX_MRGB8)
  {
    uint32_t *dst = (uint32_t*) dst_img->Data;
    for(int i=0;i<iend;i++)
    {
      sVector4 tmp = *src++;
      *dst++ = tmp.GetMRGB8();
    }
  }
  else
  {
    uint64_t *dst = (uint64_t*) dst_img->Data;
    for(int i=0;i<iend;i++)
    {
      sVector4 tmp = *src++;
      *dst++ = tmp.GetMRGB16();
    }
  }
}

void sDecompressMRGB(sImageData *dst_img, const sImageData *src_img, float alpha/*=1.0f*/)
{
  sVERIFY((src_img->Format&sTEX_FORMAT)==sTEX_MRGB8 || (src_img->Format&sTEX_FORMAT)==sTEX_MRGB16);
  sVERIFY(src_img->CodecType==sICT_RAW);
  dst_img->Init2((src_img->Format&~sTEX_FORMAT)|sTEX_ARGB32F,src_img->Mipmaps,src_img->SizeX,src_img->SizeY,src_img->SizeZ);

  sVector4* dst = (sVector4*) dst_img->Data;
  int iend = int(8*src_img->GetByteSize()/src_img->BitsPerPixel);

  if((src_img->Format&sTEX_FORMAT)==sTEX_MRGB8)
  {
    const uint32_t *src = (uint32_t*) src_img->Data;
    for(int i=0;i<iend;i++)
    {
      dst->InitMRGB8(*src++);
      dst->w = alpha;
      dst++;
    }
  }
  else
  {
    const uint64_t *src = (uint64_t*) src_img->Data;
    for(int i=0;i<iend;i++)
    {
      dst->InitMRGB16(*src++);
      dst->w = alpha;
      dst++;
    }
  }
}

void sCompressMRGB(sImageData *img, int format)
{
  sVERIFY((img->Format&sTEX_FORMAT)==sTEX_ARGB32F);
  sVERIFY((format&sTEX_FORMAT)==sTEX_MRGB8 || (format&sTEX_FORMAT)==sTEX_MRGB16);
  sVERIFY(img->CodecType==sICT_RAW);

  uint8_t* temp = img->Data;
  img->Data = 0;
  img->Init2((img->Format&~sTEX_FORMAT)|(format&sTEX_FORMAT), img->Mipmaps, img->SizeX, img->SizeY, img->SizeZ);
  
  const sVector4 *src = (sVector4*)temp;
  int iend = int(8*img->GetByteSize()/img->BitsPerPixel);

  if((format&sTEX_FORMAT)==sTEX_MRGB8)
  {
    uint32_t *dst = (uint32_t*)img->Data;
    for(int i=0;i<iend;i++,src++)
      *dst++ = src->GetMRGB8();
  }
  else
  {
    uint64_t *dst = (uint64_t*)img->Data;
    for(int i=0;i<iend;i++,src++)
      *dst++ = src->GetMRGB16();
  }
  sDeleteArray(temp);
}

void sDecompressMRGB(sImageData *img, float alpha/*=1.0f*/)
{
  sVERIFY((img->Format&sTEX_FORMAT)==sTEX_MRGB8 || (img->Format&sTEX_FORMAT)==sTEX_MRGB16);
  sVERIFY(img->CodecType==sICT_RAW);
  uint8_t* temp = img->Data;
  int format = img->Format&sTEX_FORMAT;
  img->Data = 0;
  img->Init2((img->Format&~sTEX_FORMAT)|sTEX_ARGB32F, img->Mipmaps, img->SizeX, img->SizeY, img->SizeZ);
  
  sVector4 *dst = (sVector4*)img->Data;
  int iend = int(8*img->GetByteSize()/img->BitsPerPixel);

  if(format==sTEX_MRGB8)
  {
    const uint32_t *src = (uint32_t*)temp;
    for(int i=0;i<iend;i++,dst++)
    {
      dst->InitMRGB8(*src++);
      dst->w = alpha;
    }
  }
  else
  {
    sVERIFY(format==sTEX_MRGB16);
    const uint64_t *src = (uint64_t*)temp;
    for(int i=0;i<iend;i++,dst++)
    {
      dst->InitMRGB16(*src++);
      dst->w = alpha;
    }
  }
  sDeleteArray(temp);
}


void sClampToARGB8(sImageData *img, int mm/*=0*/)
{
  sVERIFY((img->Format&sTEX_TYPE_MASK)==sTEX_2D);
  sVERIFY(img->CodecType==sICT_RAW);

  int check = 1;
  while((img->SizeX>>check)>=1 && (img->SizeY>>check)>=1) check++;
  if(!mm || check<mm) mm = check;

  sImageData *tmp = 0;
  if((img->Format&sTEX_FORMAT)!=sTEX_ARGB8888 || img->Mipmaps!=mm)
  {
    tmp = new sImageData;
    tmp->Init2(sTEX_2D|sTEX_ARGB8888,mm,img->SizeX,img->SizeY,img->SizeZ);
    tmp->Swap(img);
  }
  uint32_t *dst = (uint32_t*) img->Data;
  int iend = img->GetByteSize()/sizeof(uint32_t);

  if(tmp)
  {
    switch(tmp->Format&sTEX_FORMAT)
    {
    case sTEX_ARGB32F:
      {
        if(tmp->Mipmaps<img->Mipmaps)
          iend = tmp->GetByteSize()/sizeof(sVector4);
        sVector4 *src = (sVector4*) tmp->Data;
        for(int i=0;i<iend;i++)
          dst[i] = src[i].GetColor();
      }
      break;
    case sTEX_MRGB16:
      {
        if(tmp->Mipmaps<img->Mipmaps)
          iend = tmp->GetByteSize()/sizeof(uint64_t);
        uint64_t *src = (uint64_t*) tmp->Data;
        for(int i=0;i<iend;i++)
        {
          sVector4 col; col.InitMRGB16(src[i]);
          dst[i] = col.GetColor();
        }
      }
      break;
    case sTEX_MRGB8:
      {
        if(tmp->Mipmaps<img->Mipmaps)
          iend = tmp->GetByteSize()/sizeof(uint32_t);
        uint32_t *src = (uint32_t*) tmp->Data;
        for(int i=0;i<iend;i++)
        {
          sVector4 col; col.InitMRGB8(src[i]);
          dst[i] = col.GetColor();
        }
      }
      break;
    case sTEX_ARGB8888:
      {
        if(tmp->Mipmaps<img->Mipmaps)
          iend = tmp->GetByteSize()/sizeof(uint32_t);
        uint32_t *src = (uint32_t*) tmp->Data;
        for(int i=0;i<iend;i++)
          dst[i] = src[i];
      }
      break;
    default:
      sFatal(L"sClampToARGB8 invalid image format %d\n",img->Format&sTEX_FORMAT);
    }
  }

  sGenerateMipmaps(img);
  sDelete(tmp);
}


sImageData *sConvertARGB8ToHDR(const sImageData *img, int format)
{
  format &= sTEX_FORMAT;
  sVERIFY((img->Format&sTEX_FORMAT) == sTEX_ARGB8888);
  sVERIFY(img->CodecType == sICT_RAW);

  sVERIFY(format==sTEX_ARGB32F || format==sTEX_MRGB16 || format==sTEX_MRGB8);

  sImageData *img_dst = new sImageData;
  img_dst->Init2(sTEX_2D|format,img->Mipmaps,img->SizeX,img->SizeY,img->SizeZ);
  int iend = img->GetByteSize()/sizeof(uint32_t);
  uint32_t *src = (uint32_t*) img->Data;

  switch(format&sTEX_FORMAT)
  {
  case sTEX_ARGB32F:
    {
      sVector4 *dst = (sVector4*) img_dst->Data;
      for(int i=0;i<iend;i++)
        dst[i].InitColor(src[i]);
    }
    break;
  case sTEX_MRGB16:
    {
      uint64_t *dst = (uint64_t*) img_dst->Data;
      for(int i=0;i<iend;i++)
      {
        sVector4 tmp; tmp.InitColor(src[i]);
        dst[i] = tmp.GetMRGB16();
      }
    }
    break;
  case sTEX_MRGB8:
    {
      uint32_t *dst = (uint32_t*) img_dst->Data;
      for(int i=0;i<iend;i++)
      {
        sVector4 tmp; tmp.InitColor(src[i]);
        dst[i] = tmp.GetMRGB8();
      }
    }
    break;
  default:
    sFatal(L"sClampToARGB8 invalid image format %d\n",img->Format&sTEX_FORMAT);
  }

  return img_dst;
}

/****************************************************************************/
/***                                                                      ***/
/***   DXT Compression                                                    ***/
/***                                                                      ***/
/****************************************************************************/

static void makecol(uint32_t *c32,uint16_t c0,uint16_t c1,sBool opaque)
{
  uint8_t *c8 = (uint8_t *) c32;

  int r0,g0,b0;
  int r1,g1,b1;

  r0 = (c0&0xf800)>>11;
  g0 = (c0&0x07e0)>>5; 
  b0 = (c0&0x001f);    
  r1 = (c1&0xf800)>>11;
  g1 = (c1&0x07e0)>>5; 
  b1 = (c1&0x001f);    

  if(1)
  {
    // integer implementation (almost perfect)

    c8[ 0] = (b0*255+15)/31;
    c8[ 1] = (g0*255+31)/63;
    c8[ 2] = (r0*255+15)/31;
    c8[ 3] = 255;
    c8[ 4] = (b1*255+15)/31;
    c8[ 5] = (g1*255+31)/63;
    c8[ 6] = (r1*255+15)/31;
    c8[ 7] = 255;
    if(opaque)
    {
      c8[ 8] = ((b0+b0+b1)*255+45)/(31*3);
      c8[ 9] = ((g0+g0+g1)*255+93)/(63*3);
      c8[10] = ((r0+r0+r1)*255+45)/(31*3);
      c8[11] = 255;
      c8[12] = ((b0+b1+b1)*255+45)/(31*3);
      c8[13] = ((g0+g1+g1)*255+93)/(63*3);
      c8[14] = ((r0+r1+r1)*255+45)/(31*3);
      c8[15] = 255;
    }
    else
    {
      c8[ 8] = ((b0+b1)*255+30)/(31*2);     // INACURATE! (compared to ms)
      c8[ 9] = ((g0+g1)*255+62)/(63*2);     // INACURATE! (compared to ms)
      c8[10] = ((r0+r1)*255+30)/(31*2);     // INACURATE! (compared to ms)
      c8[11] = 255;
      c8[12] = 0;
      c8[13] = 0;
      c8[14] = 0;
      c8[15] = 0;
    }
  }
  else
  {
    // float implementation (completly inaccuate)
    c8[ 0] = (b0*255+15)/31;
    c8[ 1] = (g0*255+31)/63;
    c8[ 2] = (r0*255+15)/31;
    c8[ 3] = 255;
    c8[ 4] = (b1*255+15)/31;
    c8[ 5] = (g1*255+31)/63;
    c8[ 6] = (r1*255+15)/31;
    c8[ 7] = 255;
    if(opaque)
    {
      c8[ 8] = (uint8_t)(255 * (b0/31.0f + b0/31.0f + b1/31.0f) / 3 + 0.5f);
      c8[ 9] = (uint8_t)(255 * (g0/63.0f + g0/63.0f + g1/63.0f) / 3 + 0.5f);
      c8[10] = (uint8_t)(255 * (r0/31.0f + r0/31.0f + r1/31.0f) / 3 + 0.5f);
      c8[11] = (uint8_t)(255);
      c8[12] = (uint8_t)(255 * (b0/31.0f + b1/31.0f + b1/31.0f) / 3 + 0.5f);
      c8[13] = (uint8_t)(255 * (g0/63.0f + g1/63.0f + g1/63.0f) / 3 + 0.5f);
      c8[14] = (uint8_t)(255 * (r0/31.0f + r1/31.0f + r1/31.0f) / 3 + 0.5f);
      c8[15] = (uint8_t)(255);
    }
    else
    {
      c8[ 8] = (uint8_t)(255 * (b0+b1)/31.0f  / 2 + 0.5f);
      c8[ 9] = (uint8_t)(255 * (g0+g1)/63.0f  / 2 + 0.5f);
      c8[10] = (uint8_t)(255 * (r0+r1)/31.0f  / 2 + 0.5f);
      c8[11] = (uint8_t)(255);
      c8[12] = 0;
      c8[13] = 0;
      c8[14] = 0;
      c8[15] = 0;
    }
  }
}
/*
void UnpackDXT(uint32_t *d32,uint8_t *s,int level,int xs,int ys)
{
  uint16_t c0,c1;
  uint32_t c[4];
  uint32_t map;
  uint64_t a64;
  int a0,a1;
  uint8_t a[8];
  uint32_t *dsave = d32;

  for(int yy=0;yy<ys;yy+=4)
  {
    for(int xx=0;xx<xs;xx+=4)
    {
      switch(level)
      {
      case sTEX_DXT1:
      case sTEX_DXT1A:
        c0 = *(uint16_t *) (s+0);
        c1 = *(uint16_t *) (s+2);
        map = *(uint32_t *) (s+4);
        s+=8;
        makecol(&c[0],c0,c1,c0>c1);
        for(int y=0;y<4;y++)
        {
          for(int x=0;x<4;x++)
          {
            d32[y*xs+xx+x] = c[map&3];
            map = map>>2;
          }
        }
        break;
      case sTEX_DXT3:
        a64 = *(uint64_t *) (s+0);
        c0 = *(uint16_t *) (s+8);
        c1 = *(uint16_t *) (s+10);
        map = *(uint32_t *) (s+12);
        s+=16;
        makecol(&c[0],c0,c1,1);
        for(int y=0;y<4;y++)
        {
          for(int x=0;x<4;x++)
          {
            a0 = (a64&15); a0=a0*255/15;
            d32[y*xs+xx+x] = (c[map&3]&0x00ffffff) | (a0<<24);
            map = map>>2;
            a64 = a64>>4;
          }
        }
        break;
      case sTEX_DXT5:
      case sTEX_DXT5N:
      case sTEX_DXT5_AYCOCG:
        a0 = *(uint8_t *) (s+0);
        a1 = *(uint8_t *) (s+1);
        a64 = (*(uint64_t *) (s+0))>>16;
        c0 = *(uint16_t *) (s+8);
        c1 = *(uint16_t *) (s+10);
        map = *(uint32_t *) (s+12);
        s+=16;
        if(a0>a1)
        {
          a[0] = a0;
          a[1] = a1;
          a[2]= (6*a0+1*a1+3)/7;
          a[3]= (5*a0+2*a1+3)/7;
          a[4]= (4*a0+3*a1+3)/7;
          a[5]= (3*a0+4*a1+3)/7;
          a[6]= (2*a0+5*a1+3)/7;
          a[7]= (1*a0+6*a1+3)/7;
        }
        else
        {
          a[0] = a0;
          a[1] = a1;
          a[2]= (4*a0+1*a1+2)/5;
          a[3]= (3*a0+2*a1+2)/5;
          a[4]= (2*a0+3*a1+2)/5;
          a[5]= (1*a0+4*a1+2)/5;
          a[6]= 0;
          a[7]= 255;
        }
        makecol(&c[0],c0,c1,1);
        for(int y=0;y<4;y++)
        {
          for(int x=0;x<4;x++)
          {
            d32[y*xs+xx+x] = (c[map&3]&0x00ffffff)|(a[a64&7]<<24);
            map = map>>2;
            a64 = a64>>3;
          }
        }
        break;
      }
    }

    d32 += xs*4;
  }

  if(level==sTEX_DXT5N)
  {
    for(int i=0;i<xs*ys;i++)
    {
      uint32_t val = dsave[i];
      float r = ((val&0xff000000)>>24)-127.5f;
      float g = ((val&0x0000ff00)>> 8)-127.5f;
      float b = sFSqrt(127.5f*127.5f - r*r - g*g);
      dsave[i] = 0xff000000 
               | (0x00ff0000&(val>>8))
               | (0x0000ff00&val)
               | sClamp(int(b+127.5f),0,255);
    }
  }
  if(level==sTEX_DXT5_AYCOCG)
  {
    for(int i=0;i<xs*ys;i++)
      dsave[i] = sAYCoCgtoARGB(dsave[i]);
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   Font Generation                                                    ***/
/***                                                                      ***/
/****************************************************************************/

#if !sCONFIG_SYSTEM_WINDOWS

sFontMap *sImage::CreateFontPage(sFontMapInputParameter &inParam, int &outLetterCount, int *outBaseLine)
{
  sFatal(L"sImage::CreateFontPage() not implemented on this platform");
  return 0;
}

#else

// Find the free rect fitting the current char best
int FindBestFit(sArray<sRect>& freeRect, int cx, int cy)
{
  int result = -1;
  float minWaste = 10E10;

  for (int j = 0; j < freeRect.GetCount(); ++j)
  {
    const int diffX = freeRect[j].SizeX() - cx;
    const int diffY = freeRect[j].SizeY() - cy;

    if (0 <= diffX && 0 <= diffY) // fits
    {
      const float wasted = freeRect[j].SizeX() - (cx * cy) / (float) freeRect[j].SizeY(); // actual area / freeRect[j].SizeY()

      if (wasted < minWaste) // fits better
      {
        result = j;
        minWaste = wasted;
      }
    }
  }

  return result;
}

sFontMap *sImage::CreateFontPage(sFontMapInputParameter &inParam, int &outLetterCount, int *outBaseLine)
{
  sFontMap *characterMap = sNULL;

  int alias = inParam.AntiAliasingFactor;
  int borderPixels = sMax((int)sMax(inParam.Outline, inParam.Blur), 4);
  int effectPixels = borderPixels*alias;
  const int filterGapY = 4; // vertical space between glyphs to account for mipmapping

  // prepare result
  outLetterCount   = sGetStringLen(inParam.Letters);
  characterMap     = new sFontMap[outLetterCount];
  
  // clear characterMap
  for (int i = 0; i<outLetterCount; i++)
    sClear(characterMap[i]);
     
// compute size of mainbitmap
    
  // initialize size of letterbitmap
  int lbmpSizeX = 1;
  int lbmpSizeY = 1;
  int mbmpSizeX = 1;
  int mbmpSizeY = 1;

  sRender2DBegin(64,64);      // we don't really want to render, but font queries are forbidden outside begin/end!

  // for all fonts
  sFontMapFontDesc *ifd;
  sFORALL(inParam.FontsToSearchIn,ifd)
  {
    sFont2D *font;
    font = new sFont2D(ifd->Name, ifd->Size * alias,  (ifd->Bold   ? sF2C_BOLD    : 0)
                                                    | (ifd->Italic ? sF2C_ITALICS : 0));
   
    if (!_i && outBaseLine) *outBaseLine = font->GetBaseline();

    // for all letters
    const sChar     *cp = inParam.Letters;
    sFontMap  *cm = characterMap;

    int charcount = 0;

    while (cp && *cp)
    {
      if (!cm->Letter)
      {

        if (font->LetterExists(*cp))
        {
          cm->Letter = *cp;
          charcount++;
          
          // get dimension of current letter
          sFont2D::sLetterDimensions dimension = font->sGetLetterDimensions(cm->Letter);

          dimension.Pre   -= effectPixels;
          dimension.Cell  += effectPixels*2;
          dimension.Post  -= effectPixels;

          cm->Cell    = (dimension.Cell+alias-1)&(~(alias-1));  // this is copied from the predecessor-engine
          cm->Pre     = (dimension.Pre+alias-1)&(~(alias-1));
          cm->Post    = (dimension.Post+alias-1)&(~(alias-1));
          cm->Advance = dimension.Advance;
          
          cm->X = ( ( //sAbs(cm->pre) + sAbs(cm->post) + 
                    cm->Cell + alias*2)&(~(alias-1) ) ) +alias;
          cm->Y = dimension.Height + effectPixels+2*alias;
          cm->OffsetY = (font->GetBaseline() - dimension.OriginY) / alias;
          cm->Height = dimension.Height;
                   
          // extend the letterbitmapsize if neccessary
          lbmpSizeX = sMax<int>(lbmpSizeX, cm->X);
          lbmpSizeY = sMax<int>(lbmpSizeY, cm->Y+cm->OffsetY*alias);
        }
      }

      // next step
      cm++;
      cp++;
    }

    sDelete(font);
  }

  sRender2DEnd();

  // sort characters by descending width
  for(int i=0;i<outLetterCount-1;i++)
    for(int j=i+1;j<outLetterCount;j++)
      if(characterMap[i].Cell < characterMap[j].Cell)
        sSwap(characterMap[i],characterMap[j]);

  // extend the size of the mainbitmap until all letters fit
  sBool doesNotFit = sTRUE;
  sBool doubleVertical = sTRUE;
  while (doesNotFit)
  {
    int i;

    // double the size of the mainbitmap in one direction (changing)
    if (doubleVertical)
      mbmpSizeY = mbmpSizeY << 1;
    else
      mbmpSizeX = mbmpSizeX << 1;

    doubleVertical = !doubleVertical;


    // for all letters
    doesNotFit = sFALSE;

    sArray<sRect> freeRects;
    freeRects.AddMany(1)->Init(0, 0, mbmpSizeX, mbmpSizeY);

    for (i = 0; i < outLetterCount && !doesNotFit; ++i)
    {
      const sFontMap* cm = characterMap + i;

      if (!cm->Letter)
        continue;

      const int cx = cm->X / alias;
      const int cy = cm->Y / alias;

      const int bestFreeIndex = FindBestFit(freeRects, cx, cy + filterGapY);

      if (-1 != bestFreeIndex)
      {
        sRect* const newFree = freeRects.AddMany(1);
        sRect& rect = freeRects[bestFreeIndex];
        newFree->Init(rect.x0 + cx, rect.y0, rect.x1, rect.y1);

        rect.x1 = rect.x0 + cx;
        rect.y0 += cy + filterGapY;
      }
      else
      {
        doesNotFit = sTRUE;
      }
    }
  }

  // mark the letters which are not in the mainbitmap with height zero
  sBool *isInMainBitmap = new sBool[outLetterCount];
  sSetMem(isInMainBitmap,0,sizeof(sBool) * outLetterCount);

  // initialise the mainbitmap (this)
  Init(mbmpSizeX, mbmpSizeY);
  Fill(0x00000000);

  // initialise the letterbitmap
  sImage *letterImage = new sImage;
  lbmpSizeX=sAlign(lbmpSizeX,alias);
  lbmpSizeY=sAlign(lbmpSizeY,alias);
  letterImage->Init(lbmpSizeX, lbmpSizeY);

  // initialise the guibitmap (the bitmap given to the OS) [we need this because, we can't change the size of this on the fly]
  sImage *guiImage = new sImage;
  guiImage->Init(lbmpSizeX, lbmpSizeY);
 
  sRender2DBegin(guiImage->SizeX,guiImage->SizeY);    

  // write all letters into the mainbitmap
  uint32_t oldcol1 = sGetColor2D(1);
  uint32_t oldcol2 = sGetColor2D(2);
  sSetColor2D(1,0xff000000);    // black obviously _made_ a problem here, but now it works. if this fails for anyone, tell shamada
                                  // deprecated: for some reason, total black does not work. Since we use only the red channel, slight green is a safe color
  sSetColor2D(2,0xffffffff);

  sArray<sRect> freeRects;
  freeRects.AddMany(1)->Init(0, 0, mbmpSizeX, mbmpSizeY);

  // for all fonts
  sFORALL(inParam.FontsToSearchIn,ifd)
  {
    sFont2D *font;
    font = new sFont2D(ifd->Name, ifd->Size * alias, (ifd->Bold   ? sF2C_BOLD    : 0)
                                                   | (ifd->Italic ? sF2C_ITALICS : 0));

    for (int i=0; i<outLetterCount; i++)
    {
      // continue if the letter was already build with a previous font
      if (isInMainBitmap[i])
        continue;

      
      sFontMap  *cm = characterMap + i;

      if (!cm->Letter)
        continue;

      if (!font->LetterExists(cm->Letter))
        continue;

      isInMainBitmap[i] = sTRUE;

      int cx = cm->X / alias;
      int cy = cm->Y / alias;

      // init guibitmap

      sRect2D(0,0,lbmpSizeX,lbmpSizeY,1);

      // draw character
      sChar buffer[2];
      buffer[0] = cm->Letter;
      buffer[1] = 0;
      font->SetColor(2,2);
      font->Print(0,-cm->Pre, effectPixels/2,buffer);
      sRender2DGet(guiImage->Data);

      guiImage->Outline(inParam.Outline);
      guiImage->Blur(inParam.Blur);

      // init letterbitmap
      letterImage->SizeX = lbmpSizeX / alias;
      letterImage->SizeY = lbmpSizeY / alias;
      
      // copy (and scale) the letter
      letterImage->Scale(guiImage,sTRUE);

      const int bestFreeIndex = FindBestFit(freeRects, cx, cy + filterGapY);
      sVERIFY(-1 != bestFreeIndex); // Assume all the glyphs fit, as the bitmap size has been chosen big enough (see above)

      // put letter into the mainbitmap
      BlitFrom(letterImage, 0, cm->OffsetY, freeRects[bestFreeIndex].x0, freeRects[bestFreeIndex].y0, cx, cy);
      cm->X = freeRects[bestFreeIndex].x0;
      cm->Y = freeRects[bestFreeIndex].y0;
      cm->Height = cy;



      sRect* const newFree = freeRects.AddMany(1);
      sRect& rect = freeRects[bestFreeIndex];
      newFree->Init(rect.x0 + cx, rect.y0, rect.x1, rect.y1);

      rect.x1 = rect.x0 + cx;
      rect.y0 += cy + filterGapY;
      
      cm->Cell      /= alias;
      cm->Post      /= alias;
      cm->Pre       /= alias;
      cm->Advance   /= alias;

      // we did loose pecision here but we need to rely on the following! (for sFF_JUSTIFIED)

      cm->Advance = cm->Pre+cm->Post+cm->Cell;
    }

    sDelete(font);
  }

  sRender2DEnd();
  sSetColor2D(1,oldcol1);
  sSetColor2D(2,oldcol2);
  
  sDelete(letterImage);
  sDelete(guiImage);
  sDeleteArray(isInMainBitmap);

  if (inParam.PMAlpha)
    PMAlpha(); // turn the font image into a premultiplied alpha font image

  // the following block is deprecated, because the text is rendered white anyway
  // and alpha is set by the sImage::Outline() method.

  // copy red to alpha and set color to white
  //for(int i=0;i<SizeX*SizeY;i++)
  //  Data[i] = ((Data[i]&0x000000ff)<<24) | 0x00ffffff;
  return characterMap;
}

#endif

sINLINE void U32ToRGBA(int c, int &r, int &g, int &b, int &a)
{ 
  c = sSwapIfBE(c);
  r = c & 0xff;
  g = (c >> 8) & 0xff;
  b = (c >> 16) & 0xff;
  a = (c >> 24) & 0xff;
}

sINLINE uint32_t RGBAToU32(int r, int g, int b, int a)
{
  return (r)|((g)<<8)|((b)<<16)|((a)<<24);
}

sINLINE void U32ToRGBAAdd(int c, int &r, int &g, int &b, int &a)
{
  c = sSwapIfBE(c);
  r += c & 0xff;
  g += (c >> 8) & 0xff;
  b += (c >> 16) & 0xff;
  a += (c >> 24) & 0xff;
}

sINLINE void U32ToRGBASub(int c, int &r, int &g, int &b, int &a)
{
  c = sSwapIfBE(c);
  r -= c & 0xff;
  g -= (c >> 8) & 0xff;
  b -= (c >> 16) & 0xff;
  a -= (c >> 24) & 0xff;
}

sINLINE uint32_t RGBAToU32Div(int r, int g, int b, int a, int div)
{
  return (r/div)|((g/div)<<8)|((b/div)<<16)|((a/div)<<24);
}

void sImage::BlurX(int R)
{
  int r,g,b,a;
  sImage old;
  old.Copy(this);
  uint32_t *in = old.Data;
  uint32_t *out = Data;
  for (int y = 0; y != SizeY; ++y)
  {
    r=g=b=a=0;
    for (int ofs = -R; ofs <= R; ofs++)
    {
      U32ToRGBAAdd(in[sMax(ofs,0)], r, g, b, a);
    }
    *out++ = RGBAToU32Div(r,g,b,a,R*2+1);
    for (int x = 1; x != SizeX; ++x)
    {
      U32ToRGBAAdd(in[sMin(x+R,SizeX-1)], r, g, b, a);
      U32ToRGBASub(in[sMax(x-R-1,0)], r, g, b, a);
      *out++ = RGBAToU32Div(r,g,b,a,R*2+1);
    }
    in += SizeX;
  }
}

void sImage::BlurY(int R)
{
  FlipXY();
  BlurX(R);
  FlipXY();
}

void sImage::Blur(int R, int passes)
{
  if (!R)
    return;
  for (int i=0; i!=passes; ++i)
  {
    BlurX(R);
  }
  for (int i=0; i!=passes; ++i)
  {
    BlurY(R);
  }
}

void sImage::FlipXY()
{
  sImage old;
  old.Copy(this);
  Init(old.SizeY,old.SizeX);
  for (int y = 0; y != SizeY; ++y)
  {
    for (int x = 0; x != SizeX; ++x)
    {
      Data[x+y*SizeX] = old.Data[y+x*old.SizeX];
    }    
  }
}

void sImage::PMAlpha()
{
  int r,g,b,a;
  uint32_t *out = Data;
  for (int p = 0; p < (SizeX*SizeY); ++p)
  {
    r=g=b=a=0;
    U32ToRGBA(*out, r, g, b, a);
    *out = RGBAToU32((r*(a+1))>>8,(g*(a+1))>>8,(b*(a+1))>>8,a);
    out++;
  }
}

void sImage::ClearRGB()
{
  uint32_t *out = Data;
  for (int p = 0; p < (SizeX*SizeY); ++p)
  {
    *out &= 0xff000000;
    out++;
  }
}

void sImage::ClearAlpha()
{
  uint32_t *out = Data;
  for (int p = 0; p < (SizeX*SizeY); ++p)
  {
    *out |= 0xff000000;
    out++;
  }
}

void sImage::PinkAlpha()
{
  for(int i=0;i<SizeX*SizeY;i++)
  {
    Data[i] |= 0xff000000;
    if(Data[i]==0xffff00ff)
      Data[i] = 0;
  }
}

void sImage::MonoToAll()
{
  if (SizeX<1 && SizeY<1) return;

  sBool inalpha=sFALSE, incolor=sFALSE;
  uint32_t refa=Data[0]&0xff000000;
  uint32_t refc=Data[0]&0x00ffffff;

  for(int i=0;i<SizeX*SizeY && (!incolor || !inalpha);i++)
  {
    if ((Data[i]&0x00ffffff)!=refc) incolor=sTRUE;
    if ((Data[i]&0xff000000)!=refa) inalpha=sTRUE;
  }

  if (inalpha)
  {
    if (incolor) sLogF(L"gfx",L"MonoToAll: texture has content in both channels, using alpha\n");
    for(int i=0;i<SizeX*SizeY;i++)
      Data[i]=0x1010101*(Data[i]>>24);
  }
  else
  {
    for(int i=0;i<SizeX*SizeY;i++)
    {
      uint32_t pixel = Data[i];
      int lum = (59*((pixel>>8)&0xff)+30*((pixel>>16)&0xff)+11*(pixel&0xff)+50)/100;
      Data[i]=0x1010101*lum;
    }
  }

}

void sImage::HalfTransparentRect(int x0,int y0,int x1,int y1,uint32_t color)
{
  // clip
  x0 = sMax(x0,0);
  y0 = sMax(y0,0);
  x1 = sMin(x1,SizeX);
  y1 = sMin(y1,SizeY);

  for(int y=y0;y<y1;y++)
  {
    uint32_t *data = Data + y*SizeX;
    
    // von neumann adder: ((x&y) << 1) + (x^y) => nice fast pseudo-SIMD average
    for(int x=x0;x<x1;x++)
      data[x] = (data[x] & color) + (((data[x] ^ color) >> 1) & 0x7f7f7f7f);
  }
}

void sImage::HalfTransparentRectHole(const sRect &outer,const sRect &hole,uint32_t color)
{
  HalfTransparentRect(outer.x0,outer.y0,outer.x1,hole.y0, color);
  HalfTransparentRect(outer.x0,hole.y0, hole.x0, hole.y1, color);
  HalfTransparentRect(hole.x1, hole.y0, outer.x1,hole.y1, color);
  HalfTransparentRect(outer.x0,hole.y1, outer.x1,outer.y1,color);
}

void sImage::AlphaFromLuminance(sImage *img)
{
  sVERIFY(SizeX==img->SizeX && SizeY==img->SizeY);
  uint8_t *s = (uint8_t *) img->Data;
  uint8_t *d = (uint8_t *) Data;

  for(int i=0;i<SizeX*SizeY;i++)
    d[i*4+3] = (s[i*4+0] + 2*s[i*4+1] + s[i*4+2] + 2) >> 2;
}

void sImage::Outline(int pixelCount)
{
  int x,y,x0,x1,y0,y1,j,k;
  int xs,ys,bpr;
  uint32_t *data;
  int val;

  xs = SizeX;
  ys = SizeY;
  bpr = SizeX;
   
  data = Data;
  for(y=0;y<ys;y++)
  {
    y0 = y-pixelCount; if(y0<0) y0=0;
    y1 = y+pixelCount; if(y1>=ys) y1=ys-1;
    for(x=0;x<xs;x++)
    {
      x0 = x-pixelCount; if(x0<0) x0=0;
      x1 = x+pixelCount; if(x1>=ys) x1=xs-1;

      val = data[x +y *bpr]&255;

      for(j=y0;j<=y1;j++)
        for(k=x0;k<=x1;k++)
          if((k-x)*(k-x)+(j-y)*(j-y)<pixelCount*pixelCount)
            val = sMax(val,(int)data[k+j*bpr]&255);

      // store:
      // red channel     : antialiased font pixel
      // green channel   : 0xff if no outline, otherwise 0
      // blue channel    : unused
      // alpha channel   : final alpha
      data[x+y*bpr] = ((data[x+y*bpr]&0x000000ff) | ((pixelCount>0?0:0xff)<<8) | (sMin(val,255)<<24));
    }
  }

  uint32_t fincolor;
  for(y=0;y<ys;y++)
  {
    for(x=0;x<xs;x++)
    {
      // composite the layers created before to get a smooth font bitmap
      fincolor=sMin( ((data[x+y*bpr]>>8)&0xff) + ((data[x+y*bpr]>>0)&0xff) , (uint32_t)255);
      data[x+y*bpr] = (data[x+y*bpr]&0xff000000) | (fincolor<<16) | (fincolor<<8) | (fincolor<<0);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Misc                                                               ***/
/***                                                                      ***/
/****************************************************************************/

/*void sSaveRT(const sChar *filename,sTexture2D *rt)
{
  const uint8_t *data;
  int32_t pitch;
  sTextureFlags flags;
  sImage img;
  int xs,ys;
  sRect r;

  xs = rt->SizeX;
  ys = rt->SizeY;
  img.Init(xs,ys);
  img.Fill(0xffff0000);
  r.Init(0,0,xs,ys);

  sBeginReadTexture(data,pitch,flags,rt);
  if(flags==sTEX_ARGB8888)
  {
    data += r.x0*4 + r.y0*pitch;
    uint32_t *dest = img.Data;
    for(int y=r.y0;y<r.y1;y++)
    {
      sCopyMem(dest,data,r.SizeX()*4);
      dest += img.SizeX;
      data += pitch;
    }
  }
  sEndReadTexture();

  if(flags==sTEX_ARGB8888)
  {
    img.SaveBMP(filename);
  }
}
*/

/****************************************************************************/
/***                                                                      ***/
/***   16 bit grayscale image                                             ***/
/***                                                                      ***/
/****************************************************************************/

sImageI16::sImageI16()
{
  SizeX = 0;
  SizeY = 0;
  Data = 0;
}

sImageI16::sImageI16(int xs,int ys)
{
  SizeX = xs;
  SizeY = ys;
  Data = new uint16_t[xs*ys];
}

sImageI16::~sImageI16()
{
  delete[] Data;
}

void sImageI16::Init(int xs,int ys)
{
  if(xs!=SizeX || ys!=SizeY)
  {
    delete[] Data;
    SizeX = xs;
    SizeY = ys;
    Data = new uint16_t[xs*ys];
  }
}

sImage *sImageI16::Copy() const
{
  sImage *dest = new sImage(SizeX,SizeY);
  sCopyMem(dest->Data,Data,SizeX*SizeY*sizeof(uint16_t));
  return dest;
}

void sImageI16::CopyFrom(const sImageI16 *src)
{
  Init(src->SizeX,src->SizeY);
  sCopyMem(Data,src->Data,SizeX*SizeY*sizeof(uint16_t));
}

void sImageI16::CopyFrom(const sImage *src)
{
  Init(src->SizeX,src->SizeY);
  int max = src->SizeX*src->SizeY;
  for(int i=0;i<max;i++)
  {
    uint32_t col = src->Data[i];
    int r = (col>>16)&0xff;
    int g = (col>>8)&0xff;
    int b = (col>>0)&0xff;
    int n = sMax(r,sMax(g,b));
    Data[i] = (n<<8)|n;
  }
}

/****************************************************************************/

template <class streamer> void sImageI16::Serialize_(streamer &s)
{
  int version = s.Header(sSerId::sImage,1);
  if(version>0)
  {
    s | SizeX | SizeY;

    if(s.IsReading())
    {
      int xs=SizeX; SizeX=0;
      int ys=SizeY; SizeY=0;
      if(xs*ys==0) s.Fail();
      Init(xs,ys);
      if(xs!=SizeX || ys!=SizeY) s.Fail();
    }

    s.ArrayU16(Data,SizeX*SizeY);
    s.Footer();
  }
}

void sImageI16::Serialize(sWriter &s) { Serialize_(s); }
void sImageI16::Serialize(sReader &s) { Serialize_(s); }

/****************************************************************************/

void sImageI16::Fill(uint16_t value)
{
  for(int i=0;i<SizeX*SizeY;i++)
    Data[i] = value;
}

void sImageI16::SwapEndian()
{
  for(int i=0;i<SizeX*SizeY;i++)
    sSwapEndianI(Data[i]);
}

void sImageI16::Add(sImageI16 *img)
{
  for(int i=0;i<SizeX*SizeY;i++)
    Data[i] = sMin(0xffff,Data[i]+img->Data[i]);
}

void sImageI16::Mul(sImageI16 *img)
{
  for(int i=0;i<SizeX*SizeY;i++)
    Data[i] = ((uint32_t(Data[i]))*img->Data[i])/65535;
}

void sImageI16::Mul(uint16_t value)
{
  for(int i=0;i<SizeX*SizeY;i++)
    Data[i] = ((uint32_t(Data[i]))*value)/65535;
}

void sImageI16::FlipXY()
{
  sImageI16 old;
  old.CopyFrom(this);
  Init(old.SizeY,old.SizeX);
  for (int y = 0; y != SizeY; ++y)
  {
    for (int x = 0; x != SizeX; ++x)
    {
      Data[x+y*SizeX] = old.Data[y+x*old.SizeX];
    }    
  }
}

int sImageI16::Filter(int x,int y,sBool colwrap) const
{
  int x0 = (x>>8)&(SizeX-1);
  int y0 = (y>>8)&(SizeY-1);
  int x1 = (x0+1)&(SizeX-1);
  int y1 = (y0+1)&(SizeY-1);

  uint32_t col00 = Data[x0+y0*SizeX];
  uint32_t col01 = Data[x1+y0*SizeX];
  uint32_t col10 = Data[x0+y1*SizeX];
  uint32_t col11 = Data[x1+y1*SizeX];

  if(colwrap)   // this will not interpolate if all the pixel are 0xffff or 0x0000
  {
    int i0 = 0;
    int i1 = 0;
    if(col00>0xfff0) i1 |= 1;
    if(col01>0xfff0) i1 |= 2;
    if(col10>0xfff0) i1 |= 4;
    if(col11>0xfff0) i1 |= 8;
    if(col00<0x000f) i0 |= 1;
    if(col01<0x000f) i0 |= 2;
    if(col10<0x000f) i0 |= 4;
    if(col11<0x000f) i0 |= 8;
    if(i1==15) 
      return 0xffff;
    if(i0==15) 
      return 0x0000;
    if((i1|i0)==15)
      return 0x0000;
  }
  
  x &= 0xff;
  y &= 0xff;
  uint32_t f00 = (256-x)*(256-y);
  uint32_t f01 =      x *(256-y);
  uint32_t f10 = (256-x)*     y ;
  uint32_t f11 =      x *     y ;

  return (col00*f00 + col01*f01 + col10*f10 + col11*f11)>>16;
}

/****************************************************************************/
/***                                                                      ***/
/***   32 bit float image                                                 ***/
/***                                                                      ***/
/****************************************************************************/

sFloatImage::sFloatImage()
{
  SizeX = 0;
  SizeY = 0;
  SizeZ = 0;
  Data = 0;
  Cubemap = 0;
}

sFloatImage::sFloatImage(int xs,int ys,int zs)
{
  SizeX = xs;
  SizeY = ys;
  SizeZ = zs;
  Cubemap = 0;
  Data = new float[xs*ys*zs*4];
}

sFloatImage::~sFloatImage()
{
  delete[] Data;
}

void sFloatImage::Init(int xs,int ys,int zs)
{
  if(xs*ys*zs != SizeX*SizeY*SizeZ)
  {
    delete[] Data;
    Data = new float[xs*ys*zs*4];
  }
  SizeX = xs;
  SizeY = ys;
  SizeZ = zs;
  Cubemap = 0;
}

void sFloatImage::CopyFrom(const sFloatImage *src)
{
  Init(src->SizeX,src->SizeY,1);
  sCopyMem(Data,src->Data,4*sizeof(float)*SizeX*SizeY*SizeZ);
}

void sFloatImage::CopyFrom(const sImage *src)
{
  Init(src->SizeX,src->SizeY,1);
  for(int i=0;i<SizeX*SizeY*SizeZ;i++)
  {
    uint32_t col = src->Data[i];
    Data[i*4+0] = ((col && 0x00ff0000)>>16)/255.0f;
    Data[i*4+1] = ((col && 0x0000ff00)>> 8)/255.0f;
    Data[i*4+2] = ((col && 0x000000ff)>> 0)/255.0f;
    Data[i*4+3] = ((col && 0xff000000)>>24)/255.0f;
  }
}

void sFloatImage::CopyFrom(const sImageData *src)
{
  sVERIFY((src->Format&sTEX_FORMAT)==sTEX_ARGB8888);
  sVERIFY(src->BitsPerPixel == 32);
  switch(src->Format&sTEX_TYPE_MASK)
  {
  case sTEX_2D:
    Init(src->SizeX,src->SizeY,1);
    break;
  case sTEX_CUBE:
    Init(src->SizeX,src->SizeY,6);
    Cubemap = 1;
    break;
  case sTEX_3D:
    sVERIFY(src->SizeZ>0);
    Init(src->SizeX,src->SizeY,src->SizeZ);
    break;
  }

  float *d = Data;
  for(int z=0;z<SizeZ;z++)
  {
    uint8_t *s = (uint8_t *) src->Data;
    s += src->GetFaceSize()*z;

    for(int i=0;i<SizeX*SizeY;i++)
    {
      d[0] = s[2]/255.0f;
      d[1] = s[1]/255.0f;
      d[2] = s[0]/255.0f;
      d[3] = s[3]/255.0f;
      d+=4;
      s+=4;
    }
  }
}

void sFloatImage::CopyTo(sImage *dest) const
{
  CopyTo(dest,1.0f);
}

void sFloatImage::CopyTo(sImage *dest,float power) const
{
  sVERIFY(SizeZ==1);
  dest->Init(SizeX,SizeY);
  CopyTo(dest->Data,power);
}

void sFloatImage::CopyTo(uint32_t *d,float power) const
{
  if(power==1.0f)
  {
    for(int i=0;i<SizeX*SizeY*SizeZ;i++)
    {
      int r = sClamp(int(Data[i*4+0]*255.0f),0,255);
      int g = sClamp(int(Data[i*4+1]*255.0f),0,255);
      int b = sClamp(int(Data[i*4+2]*255.0f),0,255);
      int a = sClamp(int(Data[i*4+3]*255.0f),0,255);

      d[i] = (a<<24)|(r<<16)|(g<<8)|b;
    }
  }
  else if(power==2.0f)
  {
    for(int i=0;i<SizeX*SizeY*SizeZ;i++)
    {
      int r = sClamp(int(Data[i*4+0]*Data[i*4+0]*255.0f),0,255);
      int g = sClamp(int(Data[i*4+1]*Data[i*4+1]*255.0f),0,255);
      int b = sClamp(int(Data[i*4+2]*Data[i*4+2]*255.0f),0,255);
      int a = sClamp(int(Data[i*4+3]            *255.0f),0,255);

      d[i] = (a<<24)|(r<<16)|(g<<8)|b;
    }
  }
  else if(power==0.5f)
  {
    for(int i=0;i<SizeX*SizeY*SizeZ;i++)
    {
      int r = sClamp(int(sSqrt(Data[i*4+0])*255.0f),0,255);
      int g = sClamp(int(sSqrt(Data[i*4+1])*255.0f),0,255);
      int b = sClamp(int(sSqrt(Data[i*4+2])*255.0f),0,255);
      int a = sClamp(int(      Data[i*4+3] *255.0f),0,255);

      d[i] = (a<<24)|(r<<16)|(g<<8)|b;
    }
  }
  else
  {
    for(int i=0;i<SizeX*SizeY*SizeZ;i++)
    {
      int r = sClamp(int(sPow(Data[i*4+0],power)*255.0f),0,255);
      int g = sClamp(int(sPow(Data[i*4+1],power)*255.0f),0,255);
      int b = sClamp(int(sPow(Data[i*4+2],power)*255.0f),0,255);
      int a = sClamp(int(     Data[i*4+3]       *255.0f),0,255);

      d[i] = (a<<24)|(r<<16)|(g<<8)|b;
    }
  }
}

void sFloatImage::Fill(float a,float r,float g,float b)
{
  for(int i=0;i<SizeX*SizeY*SizeZ;i++)
  {
    Data[i*4+0] = r;
    Data[i*4+1] = g;
    Data[i*4+2] = b;
    Data[i*4+3] = a;
  }
}

/****************************************************************************/

void sFloatImage::Scale(const sFloatImage *src,int xs_,int ys_)
{
  Init(xs_,ys_,src->SizeZ);

  int sx = src->SizeX/SizeX;
  int sy = src->SizeY/SizeY;
  sVERIFY(sx*SizeX == src->SizeX);
  sVERIFY(sy*SizeY == src->SizeY);

  const float *s = src->Data;
  float *d = Data;
  for(int z=0;z<SizeZ;z++)
  {
    for(int y=0;y<SizeY;y++)
    {
      const float *ss = s;
      for(int x=0;x<SizeX;x++)
      {
        float accu[4];
        accu[0] = 0;
        accu[1] = 0;
        accu[2] = 0;
        accu[3] = 0;
        for(int yy=0;yy<sy;yy++)
        {
          for(int xx=0;xx<sx;xx++)
          {
            accu[0] += ss[0+(xx+yy*src->SizeX)*4];
            accu[1] += ss[1+(xx+yy*src->SizeX)*4];
            accu[2] += ss[2+(xx+yy*src->SizeX)*4];
            accu[3] += ss[3+(xx+yy*src->SizeX)*4];
          }
        }
        ss+=sx*4;
        d[0] = accu[0]/(sx*sy);
        d[1] = accu[1]/(sx*sy);
        d[2] = accu[2]/(sx*sy);
        d[3] = accu[3]/(sx*sy);
        d+=4;
      }
      s+=sy*src->SizeX*4;
    }
  }
}

void sFloatImage::Power(float p)
{
  if(p==2.0f)
  {
    for(int i=0;i<SizeX*SizeY*SizeZ*4;i+=4)
    {
      Data[i+0] = Data[i+0]*Data[i+0];
      Data[i+1] = Data[i+1]*Data[i+1];
      Data[i+2] = Data[i+2]*Data[i+2];
    }
  }
  else if(p==0.5f)
  {
    for(int i=0;i<SizeX*SizeY*SizeZ*4;i+=4)
    {
      Data[i+0] = sFSqrt(Data[i+0]);
      Data[i+1] = sFSqrt(Data[i+1]);
      Data[i+2] = sFSqrt(Data[i+2]);
    }
  }
  else
  {
    for(int i=0;i<SizeX*SizeY*SizeZ*4;i+=4)
    {
      Data[i+0] = sPow(Data[i+0],p);
      Data[i+1] = sPow(Data[i+1],p);
      Data[i+2] = sPow(Data[i+2],p);
    }
  }
}

void sFloatImage::Half(sBool linear)
{
  sVERIFY((SizeX&1)==0);
  sVERIFY((SizeY&1)==0);

  int xp = 4;
  int yp = SizeX*xp;
  int zp = SizeY*yp;

  float *d = Data;
  if(SizeZ==1 || Cubemap)
  {
    if(linear)
    {
      for(int z=0;z<SizeZ;z++)
      {
        for(int y=0;y<SizeY;y+=2)
        {
          for(int x=0;x<SizeX;x+=2)
          {
            for(int i=0;i<4;i++)
            {
              d[i] = ( Data[(z)*zp + (y+0)*yp + (x+0)*xp + i]
                     + Data[(z)*zp + (y+0)*yp + (x+1)*xp + i]
                     + Data[(z)*zp + (y+1)*yp + (x+0)*xp + i]
                     + Data[(z)*zp + (y+1)*yp + (x+1)*xp + i] ) * 0.25f;
            }
            d+=4;
          }
        }
      }
    }
    else
    {
      for(int z=0;z<SizeZ;z++)
      {
        for(int y=0;y<SizeY;y+=2)
        {
          for(int x=0;x<SizeX;x+=2)
          {
            d[0] = Data[z*zp + y*yp + x*xp + 0];
            d[1] = Data[z*zp + y*yp + x*xp + 1];
            d[2] = Data[z*zp + y*yp + x*xp + 2];
            d[3] = Data[z*zp + y*yp + x*xp + 3];
            d+=4;
          }
        }
      }
    }
  }
  else  // 3d texture
  {
    if(linear)
    {
      for(int z=0;z<SizeZ;z+=2)
      {
        for(int y=0;y<SizeY;y+=2)
        {
          for(int x=0;x<SizeX;x+=2)
          {
            for(int i=0;i<4;i++)
            {
              d[i] = ( Data[(z+0)*zp + (y+0)*yp + (x+0)*xp + i]
                     + Data[(z+0)*zp + (y+0)*yp + (x+1)*xp + i]
                     + Data[(z+0)*zp + (y+1)*yp + (x+0)*xp + i]
                     + Data[(z+0)*zp + (y+1)*yp + (x+1)*xp + i]
                     + Data[(z+1)*zp + (y+0)*yp + (x+0)*xp + i]
                     + Data[(z+1)*zp + (y+0)*yp + (x+1)*xp + i]
                     + Data[(z+1)*zp + (y+1)*yp + (x+0)*xp + i]
                     + Data[(z+1)*zp + (y+1)*yp + (x+1)*xp + i] ) * 0.25f;
            }
            d+=4;
          }
        }
      }
    }
    else
    {
      for(int z=0;z<SizeZ;z+=2)
      {
        for(int y=0;y<SizeY;y+=2)
        {
          for(int x=0;x<SizeX;x+=2)
          {
            d[0] = Data[z*zp + y*yp + x*xp + 0];
            d[1] = Data[z*zp + y*yp + x*xp + 1];
            d[2] = Data[z*zp + y*yp + x*xp + 2];
            d[3] = Data[z*zp + y*yp + x*xp + 3];
            d+=4;
          }
        }
      }
    }
    SizeZ = SizeZ/2;
  }
  SizeY = SizeY/2;
  SizeX = SizeX/2;
}

void sFloatImage::Downsample(int mip,const sFloatImage *src)
{
  int step = 1<<mip;
  sVERIFY(src->SizeZ==1);
  sVERIFY((src->SizeX & (step-1))==0);
  sVERIFY((src->SizeY & (step-1))==0);
  sVERIFY(src->SizeX>=step);
  sVERIFY(src->SizeY>=step);
  sVERIFY(sIsPower2(src->SizeX));
  sVERIFY(sIsPower2(src->SizeY));

  int sx = src->SizeX/step;
  int sy = src->SizeY/step;
  Init(sx,sy,src->SizeZ);

  int xp = 4;
  int yp = src->SizeX*xp;
  int myp = sx*xp;
  int mx = src->SizeX-1;
  int my = src->SizeY-1;

  int kernelsize = 2+(6<<mip);
  float *kernel = new float[kernelsize];
  float ksum = 0;
  int kshift = -3<<mip;
  for(int i=0;i<kernelsize;i++)
  {
    float a = 3;
    float x = (kshift+i-0.5f)/float(step);
    kernel[i] = float(sSin(sPIF*x)*sSin((sPIF/a)*x)/(sPIF*sPIF*(x*x)));
    ksum += kernel[i];
  }
  for(int i=0;i<kernelsize;i++)      // adjust kernel to 1.
    kernel[i] *= 1.0f/ksum;
  kshift += step/2-1;                   // remove offset completely. 

  float *mdata = new float[sx*src->SizeY*4];

  const float * __restrict s = src->Data;
  float * __restrict d = mdata;

  for(int y=0;y<src->SizeY;y++)
  {
    for(int x=0;x<src->SizeX;x+=step)
    {
      for(int i=0;i<4;i++)
      {
        float sum = 0;
        for(int j=0;j<kernelsize;j++)
          sum += s[y*yp + sClamp(x+j+kshift,0,mx)*xp + i] * kernel[j];
        d[i] = sum;
      }
      d+=4;
    }
  }

  s = mdata;
  d = Data;

  for(int y=0;y<src->SizeY;y+=step)
  {
    for(int x=0;x<sx;x+=1)
    {
      for(int i=0;i<4;i++)
      {
        float sum = 0;
        for(int j=0;j<kernelsize;j++)
          sum += s[sClamp(y+j+kshift,0,my)*myp + x*xp + i] * kernel[j];
        d[i] = sum;
      }
      d+=4;
    }
  }

  delete[] kernel;
  delete[] mdata;
}


void sFloatImage::Normalize()
{
  float *d = Data;
  for(int i=0;i<SizeX*SizeY*SizeZ;i++)
  {
    float x = d[0]*2-1;
    float y = d[1]*2-1;
    float z = d[2]*2-1;
    float e = sRSqrt(x*x + y*y + z*z);
    x *= e;
    y *= e;
    z *= e;
    d[0] = x*0.5f+0.5f;
    d[1] = y*0.5f+0.5f;
    d[2] = z*0.5f+0.5f;
    d+=4;
  }
}

void sFloatImage::ScaleAlpha(float scale)
{
  for(int i=0;i<SizeX*SizeY*SizeZ;i++)
    Data[i*4+3] *= scale;
}

float sFloatImage::GetAlphaCoverage(float tresh)
{
  int hit = 0;
  int max = SizeX*SizeY*SizeZ;
  for(int i=0;i<max;i++)
    if(Data[i*4+3]>=tresh)
      hit++;
  return float(hit)/max;
}

void sFloatImage::AdjustAlphaCoverage(float tresh,float oldcov,float maxerror)
{
  float imin = 0.01f;              // don't set this to 0.0, divide by zero looms!
  float imax = 1.0f;
  float inow = tresh;
  int n = 0;
  float error = 1.0f;
  float scale = 1.0f;
  do
  {
    float cov = GetAlphaCoverage(inow);
    scale = tresh/inow;
    error = sFAbs(cov-oldcov)*scale;

    if(cov<oldcov)                // coverage too small
    {
      imax = inow;                // lower treshold to increase coverage
      inow = (inow+imin)/2;
    }
    else                          // coverage too big
    {
      imin = inow;                // increase threshold to lower coverage
      inow = (inow+imax)/2;
    }

    n++;
  }
  while(error>maxerror && n<16);
  
  ScaleAlpha(scale);
  if(0) sDPrintF(L"covscale %f\n",scale);
}

/****************************************************************************/

