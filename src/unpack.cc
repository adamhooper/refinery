#include "refinery/unpack.h"

#include "refinery/image.h"
#include "refinery/input.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace refinery {

namespace unpack {

  class Unpacker {
  public:
    virtual int bytesPerPixel() { return 3; } // Output, not input
    virtual void unpack(
        std::streambuf& is, const UnpackSettings& settings, Image& image) = 0;

  protected:
    /* Helper methods go here */
  };

  class PpmUnpacker : public Unpacker {
  private:
    void unpackHeader(std::streambuf& is, Image& image)
    {
      const int HEADER_SIZE = 22; // usually overkill
      char buf[HEADER_SIZE];
      is.sgetn(buf, HEADER_SIZE);

      std::string header(buf, 22);

      std::istringstream headerIss(header);

      std::string p6;
      headerIss >> p6; // "P6"

      unsigned short width, height;
      headerIss >> width >> height;

      unsigned short maxValue;
      headerIss >> maxValue;

      is.pubseekoff(
          static_cast<int>(headerIss.tellg()) + 1 - HEADER_SIZE,
          std::ios::cur);

      image.setWidth(width);
      image.setHeight(height);
      image.setBytesPerPixel(maxValue == 65535 ? 6 : 3);
    }

    void copyShorts(std::streambuf& is, unsigned int nValues, Image& image)
    {
      Image::PixelsType::iterator it(image.pixels().begin());

      for (unsigned int i = 0; i < nValues; i++, it++) {
        uint16_t msb = static_cast<unsigned char>(is.sbumpc());
        unsigned char lsb = static_cast<unsigned char>(is.sbumpc());
        *it = (msb << 8) | lsb;
      }
    }

    void copyChars(std::streambuf& is, unsigned int nValues, Image& image)
    {
      Image::PixelsType::iterator it(image.pixels().begin());

      for (unsigned int i = 0; i < nValues; i++, it++)
      {
        char c = is.sbumpc();
        *it = static_cast<unsigned short>(static_cast<unsigned char>(c));
      }
    }

  public:
    virtual int bytesPerPixel() { return 3; }
    virtual void unpack(
        std::streambuf& is, const UnpackSettings& settings, Image& image)
    {
      unpackHeader(is, image);

      unsigned int nValues =
          image.width() * image.height()
          * image.bytesPerPixel() / sizeof(Image::ValueType);

      image.pixels().assign(nValues, 0);

      if (image.bytesPerPixel() == 6) {
        copyShorts(is, nValues, image);
      } else {
        copyChars(is, nValues, image);
      }
    }
  };

  class NefCompressedUnpacker : public Unpacker {
  protected:
    typedef std::vector<unsigned short> CurveType;

    /*
     * Decodes and decompresses the linearization curve.
     *
     * In comes a 12-bit (or 14-bit) value; out goes the full 16 bits.
     *
     * Many of the final entries will have the same value. "max" will be set to
     * point to the first of those max-value entries.
     */
    virtual void readCurve(
        std::streambuf& is, const UnpackSettings& settings, CurveType& curve,
        int& max) = 0;
    /*
     * Returns a Huffman decoder from the input stream.
     *
     * Seek with the input stream, then read with the decoder.
     */
    virtual HuffmanDecoder* getDecoder(
        std::streambuf& is, const UnpackSettings& settings) = 0;
    /*
     * For files with a "split" (after the linearization table in Exif data),
     * this is what to use after the split.
     */
    virtual HuffmanDecoder* getDecoder2(
        std::streambuf& is, const UnpackSettings& settings) {
      return getDecoder(is, settings);
    }
    /*
     * Nikons use six Huffman tables. getDecoder() and getDecoder2() can call
     * this method to create the right one.
     */
    HuffmanDecoder* createDecoder(std::streambuf& is, int key)
    {
      static const unsigned char nikon_tree[][32] = { // dcraw.c
        { 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,  /* 12-bit lossy */
          5,4,3,6,2,7,1,0,8,9,11,10,12 },
        { 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,  /* 12-bit lossy after split */
          0x39,0x5a,0x38,0x27,0x16,5,4,3,2,1,0,11,12,12 },
        { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,  /* 12-bit lossless */
          5,4,6,3,7,2,8,1,9,0,10,11,12 },
        { 0,1,4,3,1,1,1,1,1,2,0,0,0,0,0,0,  /* 14-bit lossy */
          5,6,4,7,8,3,9,2,1,0,10,11,12,13,14 },
        { 0,1,5,1,1,1,1,1,1,1,2,0,0,0,0,0,  /* 14-bit lossy after split */
          8,0x5c,0x4b,0x3a,0x29,7,6,5,4,3,2,1,0,13,14 },
        { 0,1,4,2,2,3,1,2,0,0,0,0,0,0,0,0,  /* 14-bit lossless */
          7,6,8,5,9,4,10,3,11,12,2,0,1,13,14 } };
      return new HuffmanDecoder(is, nikon_tree[key]);
    }

  public:
    void unpack(
        std::streambuf& is, const UnpackSettings& settings, Image& image) {
      CurveType curve;
      int left_margin = 0;
      int max;
      readCurve(is, settings, curve, max);
      unsigned short vpred[2][2];
      unsigned short hpred[2];

      vpred[0][0] = settings.vpred[0][0];
      vpred[0][1] = settings.vpred[0][1];
      vpred[1][0] = settings.vpred[1][0];
      vpred[1][1] = settings.vpred[1][1];

      int min = 0;
      std::auto_ptr<HuffmanDecoder> decoder(getDecoder(is, settings));

      image.pixels().assign(settings.height * settings.width * 3, 0);

      for (int row = 0; row < settings.height; row++) {
        Image::RowType rowPixels(image.pixelsRow(row));
        Image::Color rowColors[2] = {
          image.colorAtPoint(Point(row, 0)),
          image.colorAtPoint(Point(row, 1))
        };

        if (settings.split && row == settings.split) {
          decoder.reset(getDecoder2(is, settings));
          min = 16;
          max += 32;
        }

        for (int col = 0; col < settings.width; col++) {
          unsigned int colIsOdd = col & 1;
          int i = decoder->nextHuffmanValue();
          int len = i & 0xf;
          int shl = i >> 4;

          uint16_t bits = decoder->nextBitsValue(len - shl);

          int diff = ((bits << 1) | 1) << shl >> 1;

          if ((diff & 1 << (len - 1)) == 0) {
            diff -= (1 << len) - !shl;
          }
          if (col < 2) {
            hpred[col] = vpred[row & 1][col] += diff;
          } else {
            hpred[colIsOdd] += diff;
          }
          if (hpred[colIsOdd] + min >= max) throw "Error";
          if (col - left_margin < settings.width) {
            unsigned short val = std::min(
                hpred[colIsOdd], static_cast<unsigned short>(curve.size()));

            rowPixels[col][rowColors[colIsOdd]] = curve[val];
          }
        }
      }
    }
  };

  class NefCompressedLossy2Unpacker : public NefCompressedUnpacker {
  protected:
    void readCurve(
        std::streambuf& is, const UnpackSettings& settings,
        CurveType& curve, int& max)
    {
      const std::vector<unsigned short>& lin(settings.linearization_table);

      max = 1 << settings.bps;
      int stepSize = max / (lin.size() - 1);
      curve.assign(max, 0);

      for (int i = 0, curStep = 0, stepPos = 0; i < max; i++, stepPos++) {
        if (stepPos == stepSize) {
          stepPos = 0;
          curStep++;
        }

        curve[i] =
            (lin[curStep] * (stepSize-stepPos) +
             lin[curStep+1] * stepPos)
            / stepSize;
      }

      while (curve[max-2] == curve[max-1]) max--;
    }

    HuffmanDecoder* getDecoder(
        std::streambuf& is, const UnpackSettings& settings)
    {
      return createDecoder(is, 0);
    }

    HuffmanDecoder* getDecoder2(
        std::streambuf& is, const UnpackSettings& settings)
    {
      return createDecoder(is, 1);
    }
  };

  class UnpackerFactory {
  public:
    static Unpacker* createUnpacker(const UnpackSettings& settings)
    {
      switch (settings.format) {
        case UnpackSettings::FORMAT_PPM:
          return new PpmUnpacker();
        case UnpackSettings::FORMAT_NEF_COMPRESSED_LOSSY_2:
          return new NefCompressedLossy2Unpacker();
      }

      return 0;
    }
  };

}

Image* ImageReader::readImage(
    std::streambuf& istream, const UnpackSettings& settings)
{
  std::auto_ptr<Image> ret(new Image(settings.width, settings.height));

  std::auto_ptr<unpack::Unpacker> unpacker(
      unpack::UnpackerFactory::createUnpacker(settings));
  ret->setBytesPerPixel(unpacker->bytesPerPixel());
  ret->setFilters(settings.filters & (~((settings.filters & 0x55555555) << 1)));

  unpacker->unpack(istream, settings, *ret);

  return ret.release();
}

} // namespace refinery
