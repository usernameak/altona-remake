/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_BITIO_HPP
#define FILE_UTIL_BITIO_HPP

#include "base/types.hpp"

/****************************************************************************/

class sFile;

class sBitWriter
{
  sFile *File;

  uint8_t *Buffer;
  uint8_t *BufferPtr,*BufferEnd;
  uint32_t BitBuffer;
  int BitsLeft;

  ptrdiff_t Written;
  sBool Error;

  void FlushBuffer(sBool finish=sFALSE);

  sINLINE void PutByte(uint8_t byte)
  {
    if(BufferPtr == BufferEnd)
      FlushBuffer();

    *BufferPtr++ = byte;
    Written++;
  }

public:
  sBitWriter();
  ~sBitWriter();

  void Start(uint8_t *outBuffer,ptrdiff_t bufferSize);  // write to memory
  void Start(sFile *outFile);                   // write to file
  ptrdiff_t Finish(); // returns number of bytes written

  sBool IsOk() const { return !Error; }

  sINLINE void PutBits(uint32_t bits,int count) // never write more than 24 bits at once!
  {
#if sCONFIG_BUILD_DEBUG // yes, not sDEBUG - only check in actual debug builds.
    sVERIFY(count <= 24);
    sVERIFY(bits < (1u << count));
#endif

    BitsLeft -= count;
    BitBuffer |= bits << BitsLeft;

    while(BitsLeft <= 24)
    {
      PutByte((uint8_t) (BitBuffer >> 24));
      BitBuffer <<= 8;
      BitsLeft += 8;
    }
  }
};

/****************************************************************************/

class sBitReader
{
  friend class sLocalBitReader;

  sFile *File;
  int64_t FileSize;

  uint8_t *Buffer;
  uint8_t *BufferPtr,*BufferEnd;
  uint32_t BitBuffer;
  int BitsLeft;

  int ExtraBytes;
  sBool Error;

  void RefillBuffer();

  sINLINE uint8_t GetByte()
  {
    if(BufferPtr == BufferEnd)
      RefillBuffer();

    return *BufferPtr++;
  }

public:
  sBitReader();
  ~sBitReader();

  void Start(const uint8_t *buffer,ptrdiff_t size);   // read from memory
  void Start(sFile *file);                    // read from file
  sBool Finish();

  sBool IsOk() { return !Error && (ExtraBytes < 4 || BitsLeft == 32); }

  sINLINE void SkipBits(int count)
  {
#if sCONFIG_BUILD_DEBUG
    sVERIFY(count <= 24);
#endif

    BitBuffer <<= count;
    BitsLeft -= count;

    while(BitsLeft <= 24)
    {
      BitsLeft += 8;
      BitBuffer |= GetByte() << (32 - BitsLeft);
    }
  }

  // In case of error, PeekBits/GetBits always pad with zero bits - design your codes accordingly!
  sINLINE uint32_t PeekBits(int count)   { return BitBuffer >> (32 - count); }
  sINLINE int32_t PeekBitsS(int count)  { return int32_t(BitBuffer) >> (32 - count); }

  sINLINE uint32_t GetBits(int count)    { uint32_t r = PeekBits(count); SkipBits(count); return r; }
  sINLINE int32_t GetBitsS(int count)   { int32_t r = PeekBitsS(count); SkipBits(count); return r; }
};

// sLocalBitReader is the same as sBitReader; use for "local" copies in inner loops, only
// create instances as local variables, and never take any pointers or references to them.
// this should help a lot on platforms with high load-store forwarding penalties (e.g. powerpc).
class sLocalBitReader
{
  sBitReader &Parent;
  uint8_t *BufferPtr,*BufferEnd;
  uint32_t BitBuffer;
  int BitsLeft;

  sINLINE uint8_t GetByte()
  {
    if(BufferPtr == BufferEnd)
    {
      Parent.BufferPtr = BufferPtr;
      Parent.RefillBuffer();
      BufferPtr = Parent.BufferPtr;
      BufferEnd = Parent.BufferEnd;
    }

    return *BufferPtr++;
  }

public:
  sLocalBitReader(sBitReader &dad) : Parent(dad)
  {
    BufferPtr = dad.BufferPtr;
    BufferEnd = dad.BufferEnd;
    BitBuffer = dad.BitBuffer;
    BitsLeft = dad.BitsLeft;
  }

  ~sLocalBitReader()
  {
    Parent.BufferPtr = BufferPtr;
    Parent.BitBuffer = BitBuffer;
    Parent.BitsLeft = BitsLeft;
  }

  sINLINE void SkipBits(int count)
  {
#if sCONFIG_BUILD_DEBUG
    sVERIFY(count <= 24);
#endif

    BitBuffer <<= count;
    BitsLeft -= count;

    while(BitsLeft <= 24)
    {
      BitsLeft += 8;
      BitBuffer |= GetByte() << (32 - BitsLeft);
    }
  }

  // In case of error, PeekBits/GetBits always pad with zero bits - design your codes accordingly!
  sINLINE uint32_t PeekBits(int count)   { return BitBuffer >> (32 - count); }
  sINLINE int32_t PeekBitsS(int count)  { return int32_t(BitBuffer) >> (32 - count); }

  sINLINE uint32_t GetBits(int count)    { uint32_t r = PeekBits(count); SkipBits(count); return r; }
  sINLINE int32_t GetBitsS(int count)   { int32_t r = PeekBitsS(count); SkipBits(count); return r; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Huffman coding                                                     ***/
/***                                                                      ***/
/****************************************************************************/

// Input:   List of frequencies for <count> symbols (0=unused).
// Output:  Length of huffman code for each symbol; all lengths are <=maxLen.
void sBuildHuffmanCodeLens(int *lens,const uint32_t *freq,int count,int maxLen);

// Turns the lengths from sBuildHuffmanCodeLens into the actual codes (for encoding).
void sBuildHuffmanCodeValues(uint32_t *codes,const int *lens,int count);

// Full service
void sBuildHuffmanCodes(uint32_t *codes,int *lens,const uint32_t *freq,int count,int maxLen);

// Encode huffman code lengths (when you need to store them)
sBool sWriteHuffmanCodeLens(sBitWriter &writer,const int *lens,int count);

// Decode huffman code lenghts written with above function
sBool sReadHuffmanCodeLens(sBitReader &reader,int *lens,int count);

class sFastHuffmanDecoder
{
  static const int FastBits = 8; // MUST be <16!

  uint16_t FastPath[1<<FastBits];
  uint32_t MaxCode[26];
  int Delta[25];
  uint16_t *CodeMap;

public:
  sFastHuffmanDecoder();
  ~sFastHuffmanDecoder();

  sBool Init(const int *lens,int count);

  // DecodeSymbol for sBitReader and sLocalBitReader do exactly the same thing.
  // Use the second variant where speed is critical (and where you're presumably using
  // sLocalBitReader anyway) and the second otherwise.
  int DecodeSymbol(sBitReader &reader);

  sINLINE int DecodeSymbol(sLocalBitReader &reader)
  {
    uint32_t peek = reader.PeekBits(FastBits);
    uint16_t fast = FastPath[peek];

    if(fast & 0xf000) // valid entry
    {
      reader.SkipBits(fast >> 12);
      return fast & 0xfff;
    }

    // can't use fast path (code is too long); first, find actual code length
    peek = reader.PeekBits(24);
    int len = FastBits+1;
    while(peek >= MaxCode[len])
      len++;

    if(len == 25) // code not found
      return -1;

    // return the coded symbol
    return CodeMap[reader.GetBits(len) + Delta[len]];
  }
};

/****************************************************************************/

#endif // FILE_UTIL_BITIO_HPP

