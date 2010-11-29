#include "refinery/unpack.h"

#include "refinery/image.h"
#include "refinery/input.h"

#include <algorithm>
#include <memory>

namespace refinery {

namespace unpack {

  class Unpacker {
  public:
    virtual int bytesPerPixel() { return 3; } // Output, not input
    virtual void unpack(
        InputStream& is, const UnpackSettings& settings, Image& image) = 0;

  protected:
    /* Helper methods go here */
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
        InputStream& is, const UnpackSettings& settings, CurveType& curve,
        int& max) = 0;
    /*
     * Returns a Huffman decoder from the input stream.
     *
     * Seek with the input stream, then read with the decoder.
     */
    virtual HuffmanDecoder* getDecoder(
        InputStream& is, const UnpackSettings& settings) = 0;
    /*
     * For files with a "split" (after the linearization table in Exif data),
     * this is what to use after the split.
     */
    virtual HuffmanDecoder* getDecoder2(
        InputStream& is, const UnpackSettings& settings) {
      return getDecoder(is, settings);
    }
    /*
     * Nikons use six Huffman tables. getDecoder() and getDecoder2() can call
     * this method to create the right one.
     */
    HuffmanDecoder* createDecoder(InputStream& is, int key)
    {
      static const uint16_t nikon_tree[][32] = { // dcraw.c
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
    void unpack(InputStream& is, const UnpackSettings& settings, Image& image) {
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
        Image::Row3Type rowPixels(image.pixelsRow3(row));

        if (settings.split && row == settings.split) {
          decoder.reset(getDecoder2(is, settings));
          min = 16;
          max += 32;
        }

        for (int col = 0; col < settings.width; col++) {
          int i = decoder->nextValue();
          int len = i & 0xf;
          int shl = i >> 4;

          int diff = ((decoder->nextValue(len - shl, false) << 1) | 1)
              << shl >> 1;

          if ((diff & 1 << (len - 1)) == 0) {
            diff -= (1 << len) - !shl;
          }
          if (col < 2) {
            hpred[col] = vpred[row & 1][col] += diff;
          } else {
            hpred[col & 1] += diff;
          }
          if (hpred[col & 1] + min >= max) throw "Error";
          if (col - left_margin < settings.width) {
            int color = (settings.filters >> (((row << 1 & 14) + (col & 1)) << 1)) & 3;
            if (color == 3) color = 1; // FIXME: adjust filter instead

            unsigned short val = std::min(
                hpred[col & 1], static_cast<unsigned short>(curve.size()));

            rowPixels[col][color] = curve[val];
          }
        }
      }
    }
  };

  class NefCompressedLossy2Unpacker : public NefCompressedUnpacker {
  protected:
    void readCurve(
        InputStream& is, const UnpackSettings& settings,
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

    HuffmanDecoder* getDecoder(InputStream& is, const UnpackSettings& settings)
    {
      return createDecoder(is, 0);
    }

    HuffmanDecoder* getDecoder2(InputStream& is, const UnpackSettings& settings)
    {
      return createDecoder(is, 1);
    }
  };

  class UnpackerFactory {
  public:
    static Unpacker* createUnpacker(const UnpackSettings& settings)
    {
      switch (settings.format) {
        case UnpackSettings::FORMAT_NEF_COMPRESSED_LOSSY_2:
          return new NefCompressedLossy2Unpacker();
      }

      return 0;
    }
  };

}

Image* ImageReader::readImage(
    InputStream& istream, const UnpackSettings& settings)
{
  std::auto_ptr<Image> ret(new Image(settings.width, settings.height));

  std::auto_ptr<unpack::Unpacker> unpacker(
      unpack::UnpackerFactory::createUnpacker(settings));
  ret->setBytesPerPixel(unpacker->bytesPerPixel());
  unpacker->unpack(istream, settings, *ret);

  return ret.release();
}

} // namespace refinery
