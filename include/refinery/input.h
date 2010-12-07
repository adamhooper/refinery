#ifndef _REFINERY_INPUT_H
#define _REFINERY_INPUT_H

#include <streambuf>
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

  std::streambuf& mInputStream;
  int mMaxBits;
  TableType mTable;
  uint_fast32_t mBuffer; // some bits, in the least-significant part of the int
  unsigned int mBufferLength; // number of bits
  int mEofs; // count how many EOFs we hit so we don't rewind them in the dtor

  void init(const unsigned char initializer[])
  {
    const unsigned char* counts = &initializer[0];
    const unsigned char* leaf = &initializer[16];

    for (mMaxBits = 16; !counts[mMaxBits-1]; mMaxBits--) {}

    mTable.reserve((1 << mMaxBits) - 1);

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

  inline uint16_t getBits(unsigned int nBits)
  {
    static const uint_fast32_t TRUNCATE_LEFT[] = {
      0x0000,
      0x0001, 0x0003, 0x0007, 0x000f,
      0x001f, 0x003f, 0x007f, 0x00ff,
      0x01ff, 0x03ff, 0x07ff, 0x0fff,
      0x1fff, 0x3fff, 0x7fff, 0xffff
    };

    if (mBufferLength < nBits) {
      int msbInt = mInputStream.sbumpc();
      int lsbInt = mInputStream.sbumpc();
      uint16_t msb = static_cast<unsigned char>(msbInt);
      unsigned char lsb = static_cast<unsigned char>(lsbInt);
      mBuffer = mBuffer << 16 | (msb << 8) | lsb;
      mBufferLength += 16;
      if (msbInt == std::char_traits<char>::eof()) mEofs++;
      if (lsbInt == std::char_traits<char>::eof()) mEofs++;
    }

    return (mBuffer >> (mBufferLength - nBits)) & TRUNCATE_LEFT[nBits];
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
  HuffmanDecoder(std::streambuf& inputStream, const unsigned char initializer[])
      : mInputStream(inputStream), mBuffer(0), mBufferLength(0), mEofs(0)
  {
    this->init(initializer);
  }

  /**
   * Rewinds a byte or so, so the next byte the InputStream reads will be the
   * first one that wasn't decoded.
   */
  ~HuffmanDecoder()
  {
    for (unsigned int i = mEofs; i < mBufferLength / 8; i++) {
      mInputStream.sungetc();
    }
  }

  uint16_t nextHuffmanValue()
  {
    uint16_t key = getBits(mMaxBits);

    const EntryType& entry(mTable[key]);

    mBufferLength -= entry.len;
    return entry.leaf;
  }

  uint16_t nextBitsValue(unsigned int nBits)
  {
    if (nBits <= 0) return 0;

    uint16_t value = getBits(nBits);

    mBufferLength -= nBits;

    return value;
  }
};

} // namespace refinery

#endif /* REFINERY_INPUT_H */
