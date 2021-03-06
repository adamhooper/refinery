#include "refinery/unpack.h"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "refinery/exif.h"
#include "refinery/image.h"

#include "huffman_decoder.h"
#include "c_file_istreambuf.h"

namespace refinery {

namespace unpack {

  class GrayUnpacker {
  public:
    virtual GrayImage* unpackGrayImage(
        std::streambuf& is, const ExifData& exifData) const = 0;
  };

  class RgbUnpacker {
  public:
    virtual RGBImage* unpackRgbImage(
        std::streambuf& is, const ExifData& exifData) const = 0;
  };

  class PpmUnpacker : public RgbUnpacker {
  private:
    // Sets width, height, bpp and advances the file pointer to the pixel data
    void unpackHeader(
        std::streambuf& is, int& outWidth, int& outHeight, int& outBpp) const
    {
      const int HEADER_SIZE = 22; // usually overkill
      char buf[HEADER_SIZE];
      is.sgetn(buf, HEADER_SIZE);

      std::string header(buf, 22);

      std::istringstream headerIss(header);

      std::string p6;
      headerIss >> p6; // "P6"

      headerIss >> outWidth >> outHeight;

      unsigned short maxValue;
      headerIss >> maxValue;

      outBpp = maxValue == 65535 ? 6 : 3;

      is.pubseekoff(
          static_cast<int>(headerIss.tellg()) + 1 - HEADER_SIZE,
          std::ios::cur);
    }

    void copyShorts(
        std::streambuf& is, unsigned int nValues, unsigned short* out) const
    {
      while (nValues--) {
        uint16_t msb = static_cast<unsigned char>(is.sbumpc());
        unsigned char lsb = static_cast<unsigned char>(is.sbumpc());
        *out = (msb << 8) | lsb;
        out++;
      }
    }

    void copyChars(
        std::streambuf& is, unsigned int nValues, unsigned short* out) const
    {
      while (nValues--) {
        unsigned char c = static_cast<unsigned char>(is.sbumpc());
        *out = c << 8;
        out++;
      }
    }

  public:
    virtual RGBImage* unpackRgbImage(
        std::streambuf& is, const ExifData& exifData) const
    {
      // This will give a NullCamera
      CameraData cameraData(
          CameraDataFactory::instance().getCameraData(exifData));

      int bpp;
      int width;
      int height;
      unpackHeader(is, width, height, bpp);

      std::auto_ptr<RGBImage> image(new RGBImage(cameraData, width, height));

      unsigned int nValues =
          image->nPixels() * bpp / sizeof(RGBImage::ValueType);

      unsigned short* shorts(
          reinterpret_cast<unsigned short*>(image->pixels()));

      if (bpp == 6) {
        copyShorts(is, nValues, shorts);
      } else {
        copyChars(is, nValues, shorts);
      }

      return image.release();
    }
  };

  class NefCompressedUnpacker : public GrayUnpacker {
  protected:
    /*
     * The linearization curve, read from Exif data, is a lookup table. In goes
     * a 12-bit (or 14-bit) value; out comes the full 16 bits.
     *
     * Many of the final entries will have the same value. "max" will be set to
     * point to the first of those max-value entries.
     *
     * See http://lclevy.free.fr/nef/ to see how this is deciphered.
     */
    class LinearizationCurve {
    public:
      std::vector<unsigned short> table;
      unsigned char version0;
      unsigned char version1;
      unsigned short vpred[2][2];
      unsigned short split;
      int max;

    private:
      unsigned short bytesToShort(const unsigned char* bytes)
      {
        return static_cast<unsigned short>(bytes[0]) << 8 | bytes[1];
      }

      void fillTable(
          const std::vector<unsigned short>& rawTable, int bitsPerSample)
      {
        int tableSize = 1 << bitsPerSample;
        int stepSize = tableSize / (rawTable.size() - 1);
        table.assign(tableSize, 0);

        for (int i = 0, curStep = 0, stepPos = 0; i < tableSize; i++, stepPos++)
        {
          if (stepPos == stepSize) {
            stepPos = 0;
            curStep++;
          }

          table[i] =
              (rawTable[curStep] * (stepSize-stepPos) +
               rawTable[curStep+1] * stepPos)
              / stepSize;
        }

        for (max = tableSize - 1; table[max-1] == table[max-2]; max--) {}
      }

      void init(const ExifData& exifData, int bitsPerSample)
      {
        std::vector<unsigned char> bytes;
        exifData.getBytes("Exif.Nikon3.LinearizationTable", bytes);

        version0 = bytes[0];
        version1 = bytes[1];
        vpred[0][0] = bytesToShort(&bytes[2]);
        vpred[0][1] = bytesToShort(&bytes[4]);
        vpred[1][0] = bytesToShort(&bytes[6]);
        vpred[1][1] = bytesToShort(&bytes[8]);

        unsigned int nShorts = bytesToShort(&bytes[10]);

        std::vector<unsigned short> rawTable;
        rawTable.reserve(nShorts);
        for (unsigned int i = 12; i < 12 + nShorts * 2; i += 2) {
          rawTable.push_back(bytesToShort(&bytes[i]));
        }

        if (version0 == 0x44 && version1 == 0x20) {
          split = bytesToShort(&bytes[12 + nShorts * 2]);
        } else {
          split = 0;
        }

        this->fillTable(rawTable, bitsPerSample);
      }

    public:
      LinearizationCurve(const ExifData& exifData, int bitsPerSample)
      {
        this->init(exifData, bitsPerSample);
      }
    };

    unsigned int getBitsPerSample(const ExifData& exifData) const
    {
      return exifData.getInt("Exif.SubImage2.BitsPerSample");
    }

    unsigned int getDataOffset(const ExifData& exifData) const
    {
      return exifData.getInt("Exif.SubImage2.StripOffsets");
    }

    /*
     * Returns a Huffman decoder from the input stream.
     *
     * Seek with the input stream, then read with the decoder.
     */
    virtual HuffmanDecoder* getDecoder(
        std::streambuf& is, const ExifData& exifData) const = 0;

    /*
     * For files with a "split" (after the linearization table in Exif data),
     * this is what to use after the split.
     */
    virtual HuffmanDecoder* getDecoder2(
        std::streambuf& is, const ExifData& exifData) const {
      return getDecoder(is, exifData);
    }

    /*
     * Nikons use six Huffman tables. getDecoder() and getDecoder2() can call
     * this method to create the right one.
     */
    HuffmanDecoder* createDecoder(std::streambuf& is, int key) const
    {
      static const unsigned char NIKON_TREE[][32] = { // dcraw.c
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
      return new HuffmanDecoder(is, NIKON_TREE[key]);
    }

    inline int decodeDiff(HuffmanDecoder& decoder) const
    {
      int i = decoder.nextHuffmanValue();
      int len = i & 0xf;
      int shl = i >> 4;

      uint16_t bits = decoder.nextBitsValue(len - shl);

      int diff = ((bits << 1) | 1) << shl >> 1;

      if ((diff & 1 << (len - 1)) == 0) {
        diff -= (1 << len) - !shl;
      }

      return diff;
    }

  public:
    virtual GrayImage* unpackGrayImage(
        std::streambuf& is, const ExifData& exifData) const
    {
      CameraData cameraData(
          CameraDataFactory::instance().getCameraData(exifData));

      int bitsPerSample = getBitsPerSample(exifData);
      int width = cameraData.rawWidth();
      int height = cameraData.rawHeight();

      const LinearizationCurve curve(exifData, bitsPerSample);

      const std::vector<unsigned short>& curveTable(curve.table);
      unsigned short max(curve.max);

      unsigned short vpred[2][2];
      unsigned short hpred[2];

      vpred[0][0] = curve.vpred[0][0];
      vpred[0][1] = curve.vpred[0][1];
      vpred[1][0] = curve.vpred[1][0];
      vpred[1][1] = curve.vpred[1][1];

      is.pubseekoff(getDataOffset(exifData), std::ios::beg);

      std::auto_ptr<GrayImage> imagePtr(
          new GrayImage(cameraData, width, height));
      GrayImage& image(*imagePtr);

      std::auto_ptr<HuffmanDecoder> decoder(getDecoder(is, exifData));
      int min = 0;
      for (int row = 0; row < height; row++) {
        GrayImage::PixelType* rowPixels(image.pixelsAtRow(row));

#if 0
        /* FIXME why isn't this working? */
        if (curve.split && row == curve.split) {
          decoder.reset(getDecoder2(is, exifData));
          min = 16;
          max += 32;
        }
#endif

        int col;
        for (col = 0; col < 2; col++) {
          const int diff = this->decodeDiff(*decoder);
          hpred[col] = vpred[row & 1][col] += diff;
          if (hpred[col] >= max - min) {
            throw std::invalid_argument(
                "unpackImage: hpred[colIsOdd] + min >= max");
          }
          rowPixels[col].value() = curveTable[hpred[col]];
        }

        for (; col < width; col++) {
          const unsigned int colIsOdd = col & 1;
          const int diff = this->decodeDiff(*decoder);
          hpred[colIsOdd] += diff;
          if (hpred[colIsOdd] >= max - min) {
            throw std::invalid_argument(
                "unpackImage: hpred[colIsOdd] + min >= max");
          }
          rowPixels[col].value() = curveTable[hpred[colIsOdd]];
        }
      }

      return imagePtr.release();
    }
  };

  class NefCompressedLossy2Unpacker : public NefCompressedUnpacker {
  protected:
    virtual HuffmanDecoder* getDecoder(
        std::streambuf& is, const ExifData& exifData) const
    {
      return createDecoder(is, 0);
    }

    virtual HuffmanDecoder* getDecoder2(
        std::streambuf& is, const ExifData& exifData) const
    {
      return createDecoder(is, 1);
    }
  };

  class UnpackerFactory {
  public:
    static GrayUnpacker* createGrayUnpacker(const ExifData& exifData)
    {
      // for now...
      return new NefCompressedLossy2Unpacker();
    }
    static RgbUnpacker* createRgbUnpacker(const ExifData& exifData) {
      return new PpmUnpacker();
    }
  };

}

GrayImage* ImageReader::readGrayImage(
    std::streambuf& istream, const ExifData& exifData)
{
  std::auto_ptr<unpack::GrayUnpacker> unpacker(
      unpack::UnpackerFactory::createGrayUnpacker(exifData));

  std::auto_ptr<GrayImage> ret(
      unpacker->unpackGrayImage(istream, exifData));

  // Gotta admit, I don't know what this does :). dcraw has it.
  unsigned int filters(ret->filters());
  ret->setFilters(filters & (~((filters & 0x55555555) << 1)));

  return ret.release();
}

GrayImage* ImageReader::readGrayImage(FILE* istream, const ExifData& exifData)
{
  c_file_istreambuf istreambuf(istream);
  return readGrayImage(istreambuf, exifData);
}

RGBImage* ImageReader::readRgbImage(
    std::streambuf& istream, const ExifData& exifData)
{
  std::auto_ptr<unpack::RgbUnpacker> unpacker(
      unpack::UnpackerFactory::createRgbUnpacker(exifData));

  return unpacker->unpackRgbImage(istream, exifData);
}

RGBImage* ImageReader::readRgbImage(FILE* istream, const ExifData& exifData)
{
  c_file_istreambuf istreambuf(istream);
  return readRgbImage(istreambuf, exifData);
}

} // namespace refinery
