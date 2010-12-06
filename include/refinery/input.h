#ifndef _REFINERY_INPUT_H
#define _REFINERY_INPUT_H

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <climits> /* really we want cstdint, but it's not standard yet */

typedef unsigned short uint16_t;

namespace refinery {

# if ULONG_MAX == 0xffffffff
  typedef unsigned long   uint_fast32_t;
# elif UINT_MAX == 0xffffffff
  typedef unsigned int    uint_fast32_t;
# else
#  error could not define uint_fast32_t
# endif

class InputStream {
public:
  virtual ~InputStream();

  std::streamsize read(char* buffer, std::streamsize n);
  std::streampos seek(std::streamoff offset, std::ios::seekdir dir);
  std::streampos tell();

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n) = 0;
  virtual std::streampos doSeek(std::streamoff offset, std::ios::seekdir dir) = 0;
};

class BufferInputStream : public InputStream {
  const char* mBuffer;
  std::streampos mPos; /* number of chars already read */
  std::streamsize mSize; /* total number of chars */

public:
  BufferInputStream(const char* buffer, std::streamsize size)
      : mBuffer(buffer), mSize(size) {}

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n);
  virtual std::streampos doSeek(std::streamoff offset, std::ios::seekdir dir);
};

class FileInputStream : public InputStream {
  std::string mFilename;
  std::auto_ptr<std::streambuf> mStreambuf;

public:
  FileInputStream(const std::string& filename);

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n);
  virtual std::streampos doSeek(std::streamoff offset, std::ios::seekdir dir);
};

/**
 * Huffman decoder which reads from an input stream.
 *
 * When using a HuffmanDecoder on an InputStream, do not read from the input
 * stream as well: if you do, the next time the HuffmanDecoder reads from it
 * the bits will not match up with the earlier ones. Instead, do something like
 * this:
 *
 * inputStream.seek(100, std::ios::beg);
 * {
 *   HuffmanDecoder decoder(inputStream);
 *   uint16_t value(decoder.nextValue());
 *   ...
 * }
 * inputStream.read(buffer2, 1024);
 *
 * The destructor (invoked when decoder goes out of scope) must be called
 * before inputStream.read() or the stream may not be at the proper position.
 */
class HuffmanDecoder {
  typedef struct {
    unsigned char len;
    unsigned char leaf;
  } EntryType;
  typedef std::vector<EntryType> TableType;

  InputStream& mInputStream;
  int mMaxBits;
  TableType mTable;
  uint_fast32_t mBuffer; // some bits, in the least-significant part of the int
  int mBufferLength; // number of bits

  void init(const unsigned char initializer[])
  {
    const unsigned char* counts = &initializer[0];
    const unsigned char* leaf = &initializer[16];

    for (mMaxBits = 16; !counts[mMaxBits-1]; mMaxBits--) {}

    mTable.reserve(1 << mMaxBits);

    for (int h = 0, len = 0; len < mMaxBits; len++) {
      for (int i = 0; i < counts[len]; i++, leaf++) {
        for (int j = 0; j < 1 << (mMaxBits - len - 1); j++) {
          mTable[h].len = len + 1;
          mTable[h].leaf = *leaf;
          h++;
        }
      }
    }
  }

public:
  /**
   * Creates a Huffman decoder stream.
   *
   * initializer is a 32-uchar representation of the Huffman tree. The first
   * 16 bits specify how many codes should be 1-bit, 2-bit, etc; the last 16
   * bits are the values.
   *
   * For example, if the source is
   *
   * { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
   * 0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },
   *
   * then the code is
   *
   * 00              0x04
   * 010             0x03
   * 011             0x05
   * 100             0x06
   * 101             0x02
   * 1100            0x07
   * 1101            0x01
   * 11100           0x08
   * 11101           0x09
   * 11110           0x00
   * 111110          0x0a
   * 1111110         0x0b
   * 1111111         0xff
   */
  HuffmanDecoder(InputStream& inputStream, const unsigned char initializer[])
      : mInputStream(inputStream), mBuffer(0), mBufferLength(0)
  {
    this->init(initializer);
  }

  /**
   * Rewinds a byte or so, so the next byte the InputStream reads will be the
   * first one that wasn't decoded.
   */
  ~HuffmanDecoder()
  {
    mInputStream.seek(-mBufferLength/3, std::ios::cur);
  }

  uint16_t nextValue(int nBits = -1, bool useTable = true)
  {
    static const uint_fast32_t TRUNCATE_LEFT[] = {
      0x0000,
      0x0001, 0x0003, 0x0007, 0x000f,
      0x001f, 0x003f, 0x007f, 0x00ff,
      0x01ff, 0x03ff, 0x07ff, 0x0fff,
      0x1fff, 0x3fff, 0x7fff, 0xffff
    };

    if (nBits < 0) nBits = mMaxBits;
    if (nBits <= 0) return 0;

    while (mBufferLength < nBits) {
      char c;
      if (!mInputStream.read(&c, 1)) break;
      mBuffer = mBuffer << 8 | static_cast<unsigned char>(c);
      mBufferLength += 8;
    }

    unsigned int key =
        (mBuffer >> (mBufferLength - nBits)) & TRUNCATE_LEFT[nBits];

    uint16_t value;
    if (useTable) {
      const EntryType& entry(mTable[key]);
      value = entry.leaf;
      mBufferLength -= entry.len;
    } else {
      value = static_cast<uint16_t>(key);
      mBufferLength -= nBits;
    }

    return value;
  }
};

};

#endif /* REFINERY_INPUT_H */
