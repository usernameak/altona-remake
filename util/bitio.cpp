/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "bitio.hpp"
#include "base/system.hpp"

/****************************************************************************/

static uint8_t WriteScratch;  // scratch byte in case of buffer overruns

void sBitWriter::FlushBuffer(sBool finish)
{
  sVERIFY(Buffer != 0);
  if(BufferPtr == Buffer) // nothing to flush
    return;

  if(!File)
  {
    if(!finish) // not writing to a file and not finished => buffer overflow!
    {
      BufferPtr = &WriteScratch;
      BufferEnd = BufferPtr + 1;
      Error = sTRUE;
    }
  }
  else
  {
    ptrdiff_t size = BufferPtr - Buffer;
    if(!File->Write(Buffer,size))
      Error = sTRUE;

    BufferPtr = Buffer;
  }

  sVERIFY(finish || (BufferPtr < BufferEnd));
}

sBitWriter::sBitWriter()
{
  File = 0;
  Buffer = BufferPtr = BufferEnd = 0;
  BitBuffer = 0;
  BitsLeft = 32;

  Written = 0;
  Error = sFALSE;
}

sBitWriter::~sBitWriter()
{
  sVERIFY(Buffer == 0);
}

void sBitWriter::Start(uint8_t *outBuffer,ptrdiff_t bufferSize)
{
  sVERIFY(outBuffer != 0);
  sVERIFY(bufferSize > 0);

  File = 0;
  Buffer = BufferPtr = outBuffer;
  BufferEnd = Buffer + bufferSize;

  BitBuffer = 0;
  BitsLeft = 32;
  Written = 0;
  Error = sFALSE;
}

void sBitWriter::Start(sFile *outFile)
{
  static const int BufferSize = 4096;

  sVERIFY(outFile != 0);
  Start(new uint8_t[BufferSize],BufferSize);
  File = outFile;
}

ptrdiff_t sBitWriter::Finish()
{
  if(BitsLeft != 32) // might need to write last byte
    PutByte(BitBuffer >> 24);

  FlushBuffer(sTRUE);

  if(File)
  {
    File = 0;
    delete[] Buffer;
  }

  Buffer = BufferPtr = BufferEnd = 0;
  BitBuffer = 0;
  BitsLeft = 32;

  return Error ? -1 : Written;
}

/****************************************************************************/

static uint8_t ReadScratch = 0;

void sBitReader::RefillBuffer()
{
  sVERIFY(Buffer != 0);
  if(!File) // not reading from a file => we run past the buffer!
  {
    // we may read exactly 4 bytes past without it being an error (since that's how much we buffer)
    // 4 is edge-on (will cause another refill), so check for 5
    if(++ExtraBytes == 5)
      Error = sTRUE;

    BufferPtr = &ReadScratch; // read zeroes
    BufferEnd = BufferPtr + 1;
    return;
  }

  ptrdiff_t bufferSize = BufferEnd - Buffer;
  ptrdiff_t read = sMin((int64_t) bufferSize,FileSize - File->GetOffset());

  if(read == 0 || !File->Read(Buffer,read)) // io error or end of file
  {
    // we may read exactly 4 bytes past without it being an error (since that's how much we buffer)
    // 4 is edge-on (will cause another refill), so check for 5
    if(++ExtraBytes == 5)
      Error = sTRUE;

    BufferPtr = &ReadScratch; // read zero
    BufferEnd = BufferPtr + 1;
    return;
  }

  BufferPtr = Buffer;
  BufferEnd = Buffer + read;
}

sBitReader::sBitReader()
{
  File = 0;
  Buffer = BufferPtr = BufferEnd = 0;
  BitBuffer = 0;
  BitsLeft = 0;

  Error = sFALSE;
}

sBitReader::~sBitReader()
{
  if(Buffer)
    Finish();
}

void sBitReader::Start(const uint8_t *buffer,ptrdiff_t size)
{
  File = 0;
  FileSize = 0;

  Buffer = (uint8_t *) buffer;
  BufferPtr = Buffer;
  BufferEnd = Buffer + size;
  BitBuffer = 0;
  BitsLeft = 0;

  ExtraBytes = 0;
  Error = sFALSE;

  // fill bitbuffer (as far as possible)
  for(int i=0;i<sMin<ptrdiff_t>(size,4);i++)
  {
    BitsLeft += 8;
    BitBuffer |= *BufferPtr++ << (32 - BitsLeft);
  }
}

void sBitReader::Start(sFile *file)
{
  static const int bufferSize = 4096;

  sVERIFY(file != 0);
  File = file;
  FileSize = file->GetSize();

  Buffer = new uint8_t[bufferSize];
  BufferPtr = Buffer;
  BufferEnd = Buffer + bufferSize;
  BitBuffer = 0;
  BitsLeft = 0;

  ExtraBytes = 0;
  Error = sFALSE;

  // fill bitbuffer (as far as possible)
  RefillBuffer();

  for(int i=0;i<sMin<ptrdiff_t>(BufferEnd-Buffer,4);i++)
  {
    BitsLeft += 8;
    BitBuffer |= *BufferPtr++ << (32 - BitsLeft);
  }
}

sBool sBitReader::Finish()
{
  if(ExtraBytes == 4 && BitsLeft != 32) // if we read past the 4th extra byte, it's definitely an error.
    Error = sTRUE;

  sBool ok = !Error;

  if(File)
  {
    if(!ExtraBytes)
    {
      // seek back if we read too much because of buffering
      ptrdiff_t seekBack = (BufferEnd - BufferPtr) + (BitsLeft / 8);
      if(seekBack)
        File->SetOffset(File->GetOffset() - seekBack);
    }

    File = 0;
    FileSize = 0;

    delete[] Buffer;
  }

  Buffer = BufferPtr = BufferEnd = 0;
  BitBuffer = 0;
  BitsLeft = 0;
  Error = sFALSE;

  return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   Huffman coding                                                     ***/
/***                                                                      ***/
/****************************************************************************/

static uint32_t AddWeights(uint32_t w0,uint32_t w1)
{
  uint32_t wN = (w0 & ~0xff) + (w1 & ~0xff);    // weights
  uint32_t dN = 1u + sMax(w0 & 0xff,w1 & 0xff); // depths
  return wN | dN;
}

static void AddToHeap(sStaticArray<int> &heap,const sStaticArray<uint32_t> &weight,int item)
{
  heap.AddTail(item);

  // sift it up
  int child = heap.GetCount() - 1;
  while(child > 0)
  {
    int root = (child-1)/2;
    if(weight[heap[child]] < weight[heap[root]])
    {
      sSwap(heap[root],heap[child]);
      child=root;
    }
    else
      break;
  }
}

static int PopHeap(sStaticArray<int> &heap,const sStaticArray<uint32_t> &weight)
{
  sVERIFY(heap.GetCount() > 0);
  int ret = heap[0];

  int last = heap.RemTail();
  if(heap.GetCount())
  {
    heap[0] = last;

    // sift it down
    int count = heap.GetCount();
    int root = 0, child;
    while((child=root*2+1)<count)
    {
      if(child<count-1 && weight[heap[child]]>weight[heap[child+1]])
        child++;

      if(weight[heap[child]]<weight[heap[root]])
      {
        sSwap(heap[root],heap[child]);
        root = child;
      }
      else
        break;
    }
  }

  return ret;
}

void sBuildHuffmanCodeLens(int *lens,const uint32_t *freq,int count,int maxLen)
{
  sStaticArray<uint32_t> weight(count*2-1);
  sStaticArray<int> parent(count*2-1);
  sStaticArray<int> heap(count+1);

  weight.Resize(count*2-1);
  parent.Resize(count*2-1);

  int alphabetSize = 0;
  for(int i=0;i<count;i++)
  {
    sVERIFY(freq[i] < (1<<24));
    weight[i] = freq[i] << 8;
    alphabetSize += (freq[i] != 0);
  }

  if(alphabetSize == 1) // only one symbol? that sucks.
  {
    // add another one
    weight[freq[0] ? 1 : 0] = 1 << 8;
    alphabetSize++;
  }

  // build the tree
  sBool tooLong;
  do
  {
    // push all nonzero frequency symbols onto heap
    heap.Clear();
    int nNodes = count;
    for(int i=0;i<count;i++)
    {
      parent[i] = -1;
      if(weight[i])
        AddToHeap(heap,weight,i);
    }

    // huffman code building
    while(heap.GetCount() > 1)
    {
      // pop two smallest nodes
      int n0 = PopHeap(heap,weight);
      int n1 = PopHeap(heap,weight);

      // build new node
      parent[n0] = parent[n1] = nNodes;
      weight[nNodes] = AddWeights(weight[n0],weight[n1]);
      parent[nNodes] = -1;
      AddToHeap(heap,weight,nNodes++);
    }

    // calc code lengths
    tooLong = sFALSE;

    for(int i=0;i<count;i++)
    {
      int len = 0;
      for(int j=i;parent[j] != -1;j=parent[j])
        len++;

      lens[i] = len;
      tooLong |= (len > maxLen);
    }

    if(tooLong) // flatten the frequencies and try again
    {
      for(int i=0;i<count;i++)
        weight[i] = ((weight[i] + 256) >> 1) & ~0xff;
    }
  }
  while(tooLong);
}

void sBuildHuffmanCodeValues(uint32_t *value,const int *lens,int count)
{
  // find min/max len
  int minLen = 0x7fffffff, maxLen = 0;
  for(int i=0;i<count;i++)
  {
    if(!lens[i]) // unused code
      continue;

    minLen = sMin(minLen,lens[i]);
    maxLen = sMax(maxLen,lens[i]);
  }

  sVERIFY(minLen >= 1 && maxLen <= 24); // 24 is limit for putbits

  // assign codes
  uint32_t code = 0;
  for(int i=minLen;i<=maxLen;i++)
  {
    for(int j=0;j<count;j++)
      if(lens[j] == i)
        value[j] = code++;

    code <<= 1;
  }
}

void sBuildHuffmanCodes(uint32_t *codes,int *lens,const uint32_t *freq,int count,int maxLen)
{
  sBuildHuffmanCodeLens(lens,freq,count,maxLen);
  sBuildHuffmanCodeValues(codes,lens,count);
}

sBool sWriteHuffmanCodeLens(sBitWriter &writer,const int *lens,int count)
{
  int lastLen = -1;
  for(int i=0;i<count;i++)
  {
    int len = lens[i];
    writer.PutBits(len != 0,1);
    
    if(len)
    {
      if(lastLen == -1) // code len directly
        writer.PutBits(len,5);
      else
      {
        writer.PutBits(len != lastLen,1);
        if(len != lastLen)
        {
          writer.PutBits(len < lastLen,1);
          int diff = sAbs(len - lastLen);

          while(diff > 1)
          {
            writer.PutBits(1,1);
            diff--;
          }

          writer.PutBits(0,1);
        }
      }

      lastLen = len;
    }
  }

  return writer.IsOk();
}

sBool sReadHuffmanCodeLens(sBitReader &reader,int *lens,int count)
{
  int len = -1;
  for(int i=0;i<count;i++)
  {
    if(!reader.GetBits(1))
      lens[i] = 0;
    else
    {
      if(len == -1)
      {
        len = reader.GetBits(5);
        if(len<1 || len>24) // that doesn't make sense
          return sFALSE;
      }
      else if(reader.GetBits(1)) // len has changed
      {
        int adj = reader.GetBits(1) ? -1 : 1;
        do
        {
          len += adj;
          if(len<1 || len>24) // it doesn't make any more sense here.
            return sFALSE;
        } while (reader.GetBits(1));
      }

      lens[i] = len;
    }
  }

  return reader.IsOk();
}

/****************************************************************************/

sFastHuffmanDecoder::sFastHuffmanDecoder()
{
  CodeMap = 0;
  sClear(MaxCode);
  MaxCode[24] = ~0u;
}

sFastHuffmanDecoder::~sFastHuffmanDecoder()
{
  delete[] CodeMap;
}

sBool sFastHuffmanDecoder::Init(const int *lens,int count)
{
  sVERIFY(count <= 4096); // max because of 16bit fastpath encoding
  CodeMap = new uint16_t[count];
  uint32_t *codes = new uint32_t[count];

  // find min/max len
  int maxLen = 0;
  for(int i=0;i<count;i++)
    maxLen = sMax(maxLen,lens[i]);
  
  sVERIFY(maxLen <= 24);

  // build slow decode tables
  uint32_t code = 0;
  int syms = 0;
  for(int i=1;i<=24;i++)
  {
    Delta[i] = syms - code;
    for(int j=0;j<count;j++)
    {
      if(lens[j] == i)
      {
        CodeMap[syms++] = j;
        codes[j] = code++;
      }
    }

    if(code && code-1 >= (1u<<i)) // er, these lengths don't seem to work
    {
      delete[] CodeMap;
      delete[] codes;
      return sFALSE;
    }

    MaxCode[i] = code << (24 - i);
    code <<= 1;
  }
  MaxCode[25] = ~0u; // to catch problems

  // build fast-path decoding table
  sClear(FastPath); // clear to invalid

  for(int i=0;i<count;i++)
  {
    int len = lens[i];
    if(len && len <= FastBits)
    {
      uint32_t c = codes[i] << (FastBits - len);
      int n = 1 << (FastBits - len);
      for(int j=0;j<n;j++)
        FastPath[c+j] = (len<<12) + i;
    }
  }

  delete[] codes;
  return sTRUE;
}

int sFastHuffmanDecoder::DecodeSymbol(sBitReader &reader)
{
  sLocalBitReader localReader(reader);
  return DecodeSymbol(localReader);
}

/****************************************************************************/

