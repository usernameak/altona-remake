/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_FASTCOMPRESS_HPP
#define FILE_UTIL_FASTCOMPRESS_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/system.hpp"

/****************************************************************************/

// Very fast LZP-style compressor. Pack ratio is relatively low, but it's
// *really* quick. Right now, intended for PC only: uses a few hundred
// kilobytes of memory.
// Primary for large files (at least a few hundred kilobytes) because of
// relatively high setup overhead (both compressor+decompressor).
//
// "Very fast" means: 100-200 MB/sec (depending on how compressible the data
// is) on a Core2 2.83GHz for both compression and decompression.
class sFastLzpCompressor
{
  uint8_t *ChunkBuffer;
  uint8_t *OutBuffer;
  uint32_t *HashTable;
  uint32_t CurrentPos;

  uint8_t *RawPos;
  uint8_t *BitPos1,*BitPos2;
  uint32_t BitBuffer;
  int BitShift;

  uint32_t PWritePos;
  sBool PFirstBlock;

  void StartWrite(uint8_t *ptr);
  void EndWrite();

  void Shift();

  // The BitPos1/BitPos2 stuff looks strange, I know. But it's all too make the
  // depacker very simple+fast.
  sINLINE void PutBits(uint32_t value,int nBits) // nBits must be <=16!!
  {
    BitShift -= nBits;
    BitBuffer |= value << BitShift;

    if(BitShift <= 16)
      Shift();
  }

  sINLINE void PutBitsLong(uint32_t value,int nBits) // nBits may be up to 32
  {
    if(nBits <= 16)
      PutBits(value,nBits);
    else
    {
      PutBits(value>>16,nBits-16);
      PutBits(value&0xffff,16);
    }
  }

  sINLINE void PutByte(uint8_t value) { *RawPos++ = value; }
  void PutExpGolomb(uint32_t value);

  int CompressChunk(uint32_t nBytes,sBool isLast);
  void Reset();

public:
  sFastLzpCompressor();
  ~sFastLzpCompressor();

  // EITHER: Everything at once
  sBool Compress(sFile *dest,sFile *src);

  // OR: Piecewise I/O (normal write operations)
  void StartPiecewise();
  sBool WritePiecewise(sFile *dest,const void *buffer,ptrdiff_t size);
  sBool EndPiecewise(sFile *dest);
};

// The corresponding decompressor.
class sFastLzpDecompressor
{
  uint8_t *ChunkBuffer;
  uint8_t *InBuffer;
  uint32_t *HashTable;
  uint32_t CurrentPos;

  uint32_t PReadPos;
  uint32_t PBlockEnd;
  sBool PIsLast;

  const uint8_t *RawPos;
  uint32_t BitBuffer;
  int BitFill;

  void StartRead(const uint8_t *ptr);

  void Shift();

  sINLINE uint32_t PeekBits(int nBits) // nBits must be <=16!!
  {
    return BitBuffer >> (32 - nBits);
  }

  sINLINE void SkipBits(int nBits) // nBist must be <=16!!
  {
    BitBuffer <<= nBits;
    BitFill -= nBits;
    if(BitFill <= 16)
      Shift();
  }

  sINLINE uint32_t GetBits(int nBits) // nBits must be <=16!!
  {
    uint32_t v = PeekBits(nBits);
    SkipBits(nBits);
    return v;
  }

  sINLINE uint8_t GetByte()
  {
    return *RawPos++;
  }

  uint32_t GetExpGolomb();

  int DecompressChunk(uint32_t blockLen,sBool &last,uint8_t *&ptr);
  void Reset();

  uint32_t ReadLen(const uint8_t* ptr);

public:
  sFastLzpDecompressor();
  ~sFastLzpDecompressor();

  // EITHER: Everything at once
  sBool Decompress(sFile *dest,sFile *src);

  // OR: Piecewise I/O (normal read operations)
  void StartPiecewise();
  sBool ReadPiecewise(sFile *src,void *buffer,ptrdiff_t size);
  void EndPiecewise();
};

/****************************************************************************/

// File wrappers (intended to be used with serialization)
class sFastLzpFile : public sFile
{
  sFastLzpCompressor *Comp;
  sFastLzpDecompressor *Decomp;
  sFile *Host;
  int64_t Size;

public:
  sFastLzpFile();
  virtual ~sFastLzpFile();

  // either open for reading or writing, not both. sFastLzpFile owns host.
  // it's freed immediately if Open fails!
  sBool Open(sFile *host,sBool writing); 

  static sFile *OpenRead(sFile *host);
  static sFile *OpenWrite(sFile *host);

  virtual sBool Close();
  virtual sBool Read(void *data,ptrdiff_t size);
  virtual sBool Write(const void *data,ptrdiff_t size);
  virtual int64_t GetSize();
};

/****************************************************************************/

#endif // FILE_UTIL_FASTCOMPRESS_HPP

