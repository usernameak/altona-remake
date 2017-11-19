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
/***   This file contains classes for working with images                 ***/
/***                                                                      ***/
/***   The sImageData class is used to store images that are ready for    ***/
/***   usage as texture.                                                  ***/
/***   - all texture formats are supported                                ***/
/***   - no functions for manipulating are supported                      ***/
/***   - you can only load and save the altona file format                ***/
/***   - support for mipmaps and cubemaps                                 ***/
/***                                                                      ***/
/***   The sImage class provides functionality to work with images        ***/
/***   - only one pixel format is supported: ARGB888                      ***/
/***   - it's simple to work with the images                              ***/
/***   - you can convert to sImageData for a specific texture             ***/
/***   - you can load and save different file formats                     ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_ALTONA_UTIL_IMAGE
#define HEADER_ALTONA_UTIL_IMAGE

#ifndef __GNUC__
#pragma once
#endif


#include "base/types.hpp"
//#include "base/graphics.hpp"

enum sTextureFlags
{
  sTEX_UNKNOWN = 0,  // choose a format.
  sTEX_ARGB8888 = 1, // standard 32 bit texture
  sTEX_QWVU8888 = 2, // interpreted as signed number, for bumpmaps

  sTEX_GR16 = 3,
  sTEX_ARGB16 = 4,

  sTEX_R32F = 5, // floating point textures
  sTEX_GR32F = 6,
  sTEX_ARGB32F = 7,
  sTEX_R16F = 8,
  sTEX_GR16F = 9,
  sTEX_ARGB16F = 10,

  sTEX_A8 = 11, // alpha only
  sTEX_I8 = 12, // Intensity only (red=green=blue= 8bit)

  sTEX_DXT1 = 13,  // dxt1 without alpha
  sTEX_DXT1A = 14, // dxt1 with one bit alpha
  sTEX_DXT3 = 15,  // dxt3: direct 4 bit alpha
  sTEX_DXT5 = 16,  // dxt5: interpolated 3 bit alpha

  sTEX_INDEX4 = 17,
  sTEX_INDEX8 = 18,

  sTEX_MRGB8 = 19,  // 8bit scaled: HDR=M*RGB
  sTEX_MRGB16 = 20, // 16bit scaled: HDR=M*RGB

  sTEX_ARGB2101010 = 21,
  sTEX_D24S8 = 22, // depth stencil texture

  sTEX_I4 = 23,       // intensity only
  sTEX_IA4 = 24,      // intensity+alpha
  sTEX_IA8 = 25,      // intensity+alpha
  sTEX_RGB5A3 = 26,   // ARGB, depending on MSB, either 0:5:5:5 or 3:4:4:4
  sTEX_ARGB1555 = 27, // 16 bit format
  sTEX_RGB565 = 28,
  sTEX_DXT5N = 29,    // DXT5 for normal compression: rgba = 0g0r
  sTEX_GR8 = 30,      // green/red as 8bit (for delta-st maps)
  sTEX_ARGB4444 = 31, // AMIGA!

  sTEX_DEPTH16NOREAD = 32, // depth buffer, can not be read
  sTEX_DEPTH24NOREAD = 33,
  sTEX_PCF16 = 34, // depth buffer, texture read as PCF filtered (directx9 only)
  sTEX_PCF24 = 35,
  sTEX_DXT5_AYCOCG = 36, // YCoCg with additional alpha DXT5 compressed (with nasty color shifts which could be reduced with YCoCg without additional alpha)
  sTEX_DEPTH16 = 37,     // depth buffer, can be bound as texture to read
  sTEX_DEPTH24 = 38,

  sTEX_8TOIA = 39, // single 8bit channel that gets replicated to RGBA

  sTEX_STRUCTURED = 40, // compute shader: structured buffer.
  sTEX_RAW = 41,        // compute shader: raw (byte addressed) buffer
  sTEX_UINT32 = 42,     // another format useful for compute shaders.

  // don't use more than 64 texture formats, some tools use an sU64 bitmask to store sets of texture formats.

  sTEX_FORMAT = 0x000000ff,       // mask for format field
  sTEX_DYNAMIC = 0x00000100,      // you want to update the texture regulary by BeginLoad() / EndLoad()
                                  // may need more memory on consoles, use sTEX_STREAM for textures which are updated every frame
  sTEX_RENDERTARGET = 0x00000200, // you want to use the texture as a rendertarget.
  sTEX_NOMIPMAPS = 0x00000400,    // no mipmaps
  sTEX_MAIN_DEPTH = 0x00000800,   // use main render target depth stencil buffer
  sTEX_NORMALIZE = 0x00010000,    // enable renomalization durng mipmap generation - useful for bumpmaps!
  sTEX_SCREENSIZE = 0x00020000,   // for rendertargets: use size of screen, update when resizing, use SizeX & SizeY as shift to create half / qarter screens.
  sTEX_SOFTWARE = 0x00040000,     // flagging sTextureSoftware type
  sTEX_SWIZZLED = 0x00080000,     // hardware dependent swizzled texture
  sTEX_PROXY = 0x00100000,        // flagging sTextureProxy type
  sTEX_AUTOMIPMAP = 0x00200000,   // generate mipmaps for rendertarget
  sTEX_STREAM = 0x00400000,       // texture is update every frame
  sTEX_FASTDXTC = 0x00800000,     // texture uses fast dxt compression

  sTEX_TILED_RT = 0x01000000, // only render/resolve the target rectangle not the complete texture

  sTEX_INTERNAL = 0x10000000, // used by system to initialize texture structures for backbuffer and zbuffers

  sTEX_MSAA = 0x20000000,             // multisample anti aliasing (for rendertargets)
  sTEX_NORESOLVE = 0x40000000,        // have multisampled and non multisampled buffers, but do not actually resolve (used for zbuffers)
  sTEX_AUTOMIPMAP_POINT = 0x80000000, // in addition to the automipmap flag: use point sampling, not best sampling

  // texture types

  sTEX_2D = 0x00001000, // texture type.
  sTEX_CUBE = 0x00002000,
  sTEX_3D = 0x00003000,
  sTEX_BUFFER = 0x00004000,    // compute shader: buffer
  sTEX_TYPE_MASK = 0x0000f000, // must be within lower 16 bit for sImgBlobExtract::Serialize_

  // running out of bits - recycling console specific flags for DX11 compute shader features

  sTEX_CS_INDEX = 0x00100000,    // Will be used as an index buffer
  sTEX_CS_VERTEX = 0x01000000,   // Will be used as a vertex buffer
  sTEX_CS_WRITE = 0x02000000,    // create compute shader unordered access view for this texture
  sTEX_CS_COUNT = 0x04000000,    // add counter
  sTEX_CS_STACK = 0x08000000,    // allow append / consume
  sTEX_CS_INDIRECT = 0x00080000, // can be used as draw indirect buffer
};

class sImage;
class sTextureBase;

extern sU64 sTotalImageDataMem;  // total memory in sImageData bitmaps

/****************************************************************************/

enum sImageCodecType
{
  sICT_RAW    = 0x00,             // raw data
  sICT_IM49   = 0x01,             // IM49 codec

  sICT_COUNT  = 0x10,             // current max supported # of image codecs (increase if you need more)
};

/****************************************************************************/

class sImageData
{
  int DataSize;                  // size for all data, including cubemaps (allocated size)
  int UnpackedSize;              // size of unpacked data (==DataSize if CodecType==sICT_RAW)
  int CubeFaceSize;              // cubemaps are stored: face0 mm0 face0 mm1 face0 mm2 face1 mm0 case1 mm1 ...
public:
  int Format;                    // sTEX_XXX format
  int SizeX;                     // Width in pixels
  int SizeY;                     // Height in pixels
  int SizeZ;                     // for volumemaps. 0 otherwise
  int Mipmaps;                   // number of mipmaps
  int BitsPerPixel;              // for size calculation
  sU8 *Data;                      // the Data itself
  int PaletteCount;              // in color entries (32 bit units)
  sU32 *Palette;                  // if a palette is required, here it is!
  int NameId;                    // Texture serialization nameid
  int Quality;                   // dither quality for DXT conversions
  int CodecType;                 // sICT_***

  sImageData();
  sImageData(sImage *,int flagsandformat=sTEX_2D|sTEX_ARGB8888);
  sImageData(sImage *,int flagsandformat,int ditherquality);
  ~sImageData();

//  void Init(int format,int xs,int ys,int mipmaps=1);    // obsolete
  void Init2(int format,int mipmaps,int xs,int ys,int zs); // new version, with volume maps
  void InitCoded(int codecType,int format,int mipmaps,int xs,int ys,int zs,const sU8 *data,int dataSize);

  void Copy(const sImageData *source);
  void Swap(sImageData *img);
  int GetByteSize() const { return UnpackedSize; }
  int GetDiskSize() const { return DataSize; }
  int GetFaceSize() const { return CubeFaceSize; }
  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &s);
  void Serialize(sReader &s);

  class sTextureBase *CreateTexture() const;
  class sTextureBase *CreateCompressedTexture() const;
  void UpdateTexture(sTextureBase *) const; // this will recreate the texture if extends don't fit.
  void UpdateTexture2(sTextureBase *&) const; // this will create the texture for given null ptr and reinit given texture if extends don't fit.

  void ConvertFrom(const sImage *);
  void ConvertTo(sImage *,int mipmap=0) const;
  void ConvertFromCube(sImage **imgptr);
  void ConvertToCube(sImage **imgptr, int mipmap=0)const;

  sOBSOLETE int Size()const { return DataSize; } // use GetByteSize!
};

void sGenerateMipmaps(sImageData *img);
sImage *sDecompressImageData(const sImageData *src);
sImageData *sDecompressAndConvertImageData(const sImageData *src);
sTextureBase *sStreamImageAsTexture(sReader &s);

typedef sImage *(*sDecompressImageDataHandler)(const sImageData *src);
void sSetDecompressHandler(int codecType,sDecompressImageDataHandler handler);

/****************************************************************************/

struct sFontMapFontDesc
{
  sString<256> Name;      // name of font to search by the fontsystem of the OS
  sU32         Size;      // fontsize in pixel
  sS32         YOffset;   // offset in y-direction (positive values shift the font upwards)
  sBool        Italic;    // font in italic mode
  sBool        Bold;      // font in bold mode
};

class sFontMapInputParameter
{
  public:
    sStaticArray<sFontMapFontDesc> FontsToSearchIn;
    sU32         AntiAliasingFactor;
    sBool        Mipmaps;   // font has mitmaps
    sBool        Hinting;   // font has hinting enabled
    sBool        PMAlpha;   // font supports premultiplied alpha
    sU32         Outline;   // size of outline in pixels
    sU32         Blur;      // size of blur kernel in pixels
    const sChar *Letters;   // the letters to be contained by the fontmap
};


// there are two units for pixels
// bitmap-pixels: 
//   without multisampling factor
// multi-pixels: 
//   with multisampling multi-pixels = bitmap-pixels * Alias

struct sFontMap
{
  sU16 X,Y;                       ///< position in bitmap in bitmap-pixels
  sS16 Pre;                       ///< kerning before character in multi-pixels
  sS16 Cell;                      ///< size of bitmap-cell in bitmap-pixels
  sS16 Post;                      ///< kerning after character in multi-pixels
  sS16 Advance;                   ///< total advance in multi-pixels
  sU16 Letter;                    ///< 16-bit unicode of character
  sS16 OffsetY;                   // vertical distance between the top of the character cell and the glyph
  sU16 Height;                    // height of the glyph
  sU16 Pad;
};



class sImage
{
public:
  sBool LoadBMP(const sU8 *data,int size);
  sBool LoadPIC(const sU8 *data,int size);
  sBool LoadTGA(const sChar *name);
  sBool LoadJPG(const sU8 *data,int size);
  sBool LoadPNG(const sU8 *data,int size);

  sBool SaveBMP(const sChar *name);
  sBool SavePIC(const sChar *name);
  sBool SaveTGA(const sChar *name);
  sBool SavePNG(const sChar *name);

  int SaveBMP(sU8 *data, int size);

public:
/*
  enum IChannel     // IChannel functions removed because rarely needed 
  {                 // and not really better than shifting & masking by hand.
    ChBLUE=0,       // delete this when nobody complains...
    ChGREEN,
    ChRED,
    ChALPHA,
  };
*/
  int SizeX;
  int SizeY;
  sU32 *Data;

  sImage();
  sImage(int xs,int ys);
  ~sImage();
  void Init(int xs,int ys);
  void CopyRect(sImage *src,const sRect &destpos,int srcx,int srcy);
  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  sBool HasAlpha();                 // TRUE when at least one pixel is alpha!=255 -> use for DXT1/DXT5 check
  
  void Fill(sU32 color);
//  void Fill(sU32 srcBitmask,sU32 color);
  void SwapRedBlue();
  void SwapEndian();
#if sCONFIG_BE
  void SwapIfLE()       { }
  void SwapIfBE()       { SwapEndian(); }
#else
  void SwapIfLE()       { SwapEndian(); }
  void SwapIfBE()       { }
#endif

  void Checker(sU32 col0,sU32 col1,int maskx=8,int masky=8);
  void Glow(sF32 cx,sF32 cy,sF32 rx,sF32 ry,sU32 color,sF32 alpha=1.0f,sF32 power=0.5f);
  void Rect(sF32 x0,sF32 x1,sF32 y0,sF32 y1,sU32 color);
  void Perlin(int start,int octaves,sF32 falloff,int mode,int seed,sF32 amount);

  void MakeSigned();                    // convert from 0..255 unsigned to -128..127 signed
//  void ExchangeChannels(IChannel channelX, IChannel channelY);
//  void CopyChannelFrom(IChannel channelDst, IChannel channelSrc);
//  void SetChannel(IChannel channelX,sU8 val);
  void Copy(sImage *);
  void Add(sImage *);
  void Mul(sImage *);
  void Mul(sU32 color);
  void Blend(sImage *, sBool premultiplied);
  void ContrastBrightness(int contrast,int brightness); // x = (x-0.5)*c+0.5+b;
  void BlitFrom(const sImage *src, int sx, int sy, int dx, int dy, int width, int height);
  void Outline(int pixelCount);
  void BlurX(int R);
  void BlurY(int R);
  void Blur(int R, int passes=3);
  void FlipXY(); // flip x/y
  void PMAlpha();
  void ClearRGB();
  void ClearAlpha();
  void AlphaFromLuminance(sImage *);
  void PinkAlpha();
  void MonoToAll(); // min(alpha,luminance) -> all channels

  void HalfTransparentRect(int x0,int y0,int x1,int y1,sU32 color);
  void HalfTransparentRect(const sRect &r,sU32 color) { HalfTransparentRect(r.x0,r.y0,r.x1,r.y1,color); }
  void HalfTransparentRectHole(const sRect &outer,const sRect &hole,sU32 color);

  sFontMap *CreateFontPage(sFontMapInputParameter &inParam, int &outLetterCount, int *outBaseLine=0);
  

  sImage *Scale(int xs,int ys) const;
  void Scale(const sImage *src, sBool filter=sFALSE);
  sImage *Half(sBool gammacorrect=sFALSE) const;
  sImage *Copy() const;

  sU32 Filter(int x,int y) const;     // 24:8 pixel, requires power 2 texture!

//  sImageData *Convert(int tf,int mip=0) const;  // obsolete
//  void Convert(sImageData *);                     // obsolete

  sBool Load(const sChar *name);
  sBool Save(const sChar *name);

  void Diff(const sImage *img0, const sImage *img1);

  void sOBSOLETE CopyRenderTarget();
};

/****************************************************************************/
/***                                                                      ***/
/***   16 bit grayscale image                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sImageI16
{
public:
  int SizeX;
  int SizeY;
  sU16 *Data;

  sImageI16();
  sImageI16(int xs,int ys);
  ~sImageI16();
  void Init(int xs,int ys);
  sImage *Copy() const;
  void CopyFrom(const sImageI16 *src);
  void CopyFrom(const sImage *src);

  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  void Fill(sU16 value);
  void SwapEndian();
#if sCONFIG_BE
  void SwapIfLE()       { }
  void SwapIfBE()       { SwapEndian(); }
#else
  void SwapIfLE()       { SwapEndian(); }
  void SwapIfBE()       { }
#endif

  void Add(sImageI16 *);
  void Mul(sImageI16 *);
  void Mul(sU16 value);
  void FlipXY(); 
  int Filter(int x,int y,sBool colwrap=0) const;     // 24:8 pixel, requires power 2 texture!
};


/****************************************************************************/
/***                                                                      ***/
/***   32 bit float image (untested)                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sFloatImage
{
public:
  int SizeX;
  int SizeY;
  int SizeZ;                     // for cube or volume map
  sF32 *Data;
  sBool Cubemap;                  // Interpret SizeZ as cubemap, not volumemap

  sFloatImage();
  sFloatImage(int xs,int ys,int zs=1);
  ~sFloatImage();
  void Init(int xs,int ys,int zs=1);
  void CopyFrom(const sFloatImage *src);
  void CopyFrom(const sImage *src);
  void CopyFrom(const sImageData *src);
  void CopyTo(sImage *dest) const;
  void CopyTo(sImage *dest,sF32 power) const; // power optimized for 0.5f , 1.0f and 2.0f, only for RGB
  void CopyTo(sU32 *dest,sF32 power) const;   // power optimized for 0.5f , 1.0f and 2.0f, only for RGB
  

  void Fill(sF32 r,sF32 g,sF32 b,sF32 a);
  void Scale(const sFloatImage *src,int xs,int ys); // no interpolation
  void Power(sF32 p);             // power optimized for 0.5f and 2.0f, only for RGB
  void Half(sBool linear);
  void Downsample(int mip,const sFloatImage *src);
  void Normalize();

  void ScaleAlpha(sF32 scale);
  sF32 GetAlphaCoverage(sF32 tresh);
  void AdjustAlphaCoverage(sF32 tresh,sF32 cov,sF32 error);
};


/****************************************************************************/
/***                                                                      ***/
/***   Some sImage helpers                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sTexture2D *sLoadTexture2D(const sChar *name,int formatandflags);
class sTexture2D *sLoadTexture2D(const sImage *img,int formatandflags);
class sTexture2D *sLoadTexture2D(const sImageData *img);
class sTextureCube *sLoadTextureCube(const sChar *t0,const sChar *t1,const sChar *t2,const sChar *t3,const sChar *t4,const sChar *t5,int formatandflags);
class sTextureCube *sLoadTextureCube(const sImageData *img);
//sImage *sReadTexImage(sU32 *&data, sU32 &texflags);
//void sReadTexImage(sU32 *&data, sU32 &texflags, sImage *img);
void sSaveRT(const sChar *filename,sTexture2D *rt=0);

int sDiffImage(sImage *dest, const sImage *img0, const sImage *img1, sU32 mask=0xffffffff);
void Heightmap2Normalmap(sImage *dest, const sImage *src, sF32 scale /*= 1.0f*/);
sImageData *sMergeNormalMaps(const sImageData *img0, const sImageData *img1);

// HDR compression
inline sBool sCheckMRGB(int format) { format &= sTEX_FORMAT; return format==sTEX_MRGB8 || format==sTEX_MRGB16; }
void sCompressMRGB(sImageData *dst_img, int format, const sImageData *src_img);
void sDecompressMRGB(sImageData *dst_img, const sImageData *src_img, sF32 alpha=1.0f);
void sCompressMRGB(sImageData *img, int format);
void sDecompressMRGB(sImageData *img, sF32 alpha=1.0f);
void sClampToARGB8(sImageData *img, int mm=0);
sImageData *sConvertARGB8ToHDR(const sImageData *img, int format);

/****************************************************************************/

// HEADER_ALTONA_UTIL_IMAGE
#endif
