#include "refinery/output.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/shared_ptr.hpp>

#include "refinery/image.h"

typedef unsigned short uint16_t;

namespace refinery {

using boost::shared_ptr;

class GammaCurve {
  typedef std::vector<uint16_t> CurveType;
  shared_ptr<CurveType> mCurve;

  void init(double pwr, double ts, int max) {
    double g[6] = { 0, 0, 0, 0, 0, 0 };
    g[0] = pwr;
    g[1] = ts;

    double bnd[2] = { 0, 1 };

    for (int i = 0; i < 48; i++) {
      g[2] = (bnd[0] + bnd[1]) / 2;

      double t = std::pow(g[2]/g[1],-g[0]) - 1;
      bnd[t/g[0] - 1/g[2] > -1] = g[2];
    }

    g[3] = g[2] / g[1];
    g[4] = g[2] * (1/g[0] - 1);
    g[5] = 1 / (
        g[1] * g[3] * g[3] / 2
        + 1 - g[2] - g[3]
        - g[2] * g[3] * (std::log(g[3]) - 1)
        ) - 1;

    mCurve.reset(new CurveType(0x10000, 0));
    CurveType& curve(*mCurve);

    for (unsigned int i = 0; i < 0x10000; i++) {
      double r = static_cast<double>(i) / max;
      uint16_t val;
      if (r < 1.0) {
        val = 0x10000 * (
            r < g[3]
              ? r * g[1]
              : (std::pow(r, g[0]) * (1 + g[4]) - g[4])
            );
      } else {
        val = 0xffff;
      }
      curve[i] = val;
    }
  }

public:
  GammaCurve(double pwr, double ts, int max)
  {
    init(pwr, ts, max);
  }

  GammaCurve(const GammaCurve& rhs) : mCurve(rhs.mCurve) {}

  inline const uint16_t operator[](int index) const { return (*mCurve)[index]; }
};

class Histogram {
  typedef std::vector<unsigned short> CurveType;
  shared_ptr<CurveType> mRCurve;
  shared_ptr<CurveType> mGCurve;
  shared_ptr<CurveType> mBCurve;

  void init(const Image& image)
  {
    mRCurve.reset(new CurveType(0x2000, 0));
    mGCurve.reset(new CurveType(0x2000, 0));
    mBCurve.reset(new CurveType(0x2000, 0));

    CurveType& rCurve(*mRCurve);
    CurveType& gCurve(*mGCurve);
    CurveType& bCurve(*mBCurve);

    for (int row = 0; row < image.height(); row++) {
      const Image::PixelType* pixels(image.constPixelsAtRow(row));

      for (int col = 0; col < image.width(); col++, pixels++) {
        rCurve[pixels[0].r >> 3]++;
        gCurve[pixels[0].g >> 3]++;
        bCurve[pixels[0].b >> 3]++;
      }
    }
  }

public:
  Histogram(const Image& image)
  {
    init(image);
  }

  Histogram(const Histogram& rhs)
    : mRCurve(rhs.mRCurve) , mGCurve(rhs.mGCurve), mBCurve(rhs.mBCurve) {}

  inline unsigned short count(const Image::ColorType& color, unsigned int val) const
  {
    switch (color) {
      case 0: return (*mRCurve)[val];
      case 1: return (*mGCurve)[val];
      case 2: return (*mBCurve)[val];
    }

    return 0;
  }
};

class PpmImageWriter {
  std::ostream& mOutput;
  std::streambuf& mOutputStream;

  GammaCurve buildGammaCurve(const Image& image)
  {
    // Output image will be 1% white
    unsigned int perc = image.width() * image.height() * 0.01;
    unsigned int white = 0;

    Histogram histogram(image);

    for (Image::ColorType c = 0; c <= 2; c++) {
      unsigned int total = 0;
      unsigned int val = 0x1fff;
      while (val > 32) {
        total += histogram.count(c, val);
        if (total > perc) break;
        val--;
      }
      if (white < val) white = val;
    }

    return GammaCurve(0.45, 4.5, white << 3);
  }

  void gammaCorrectPixel(
      const Image::PixelType& inRgb, Image::PixelType& outRgb,
      const GammaCurve& gammaCurve)
  {
    outRgb.r = gammaCurve[inRgb.r];
    outRgb.g = gammaCurve[inRgb.g];
    outRgb.b = gammaCurve[inRgb.b];
  }

  void writePixel8Bit(const Image::PixelType& rgb)
  {
    mOutputStream.sputc(rgb.r >> 8);
    mOutputStream.sputc(rgb.g >> 8);
    mOutputStream.sputc(rgb.b >> 8);
  }

  void writePixel16Bit(const Image::PixelType& rgb)
  {
    mOutputStream.sputc(rgb.r >> 8);
    mOutputStream.sputc(rgb.r & 0xff);
    mOutputStream.sputc(rgb.g >> 8);
    mOutputStream.sputc(rgb.g & 0xff);
    mOutputStream.sputc(rgb.b >> 8);
    mOutputStream.sputc(rgb.b & 0xff);
  }

  void writeImageBytes8Bit(const Image& image, const GammaCurve& gammaCurve)
  {
    Image::PixelType rgb;

    for (int row = 0; row < image.height(); row++) {
      const Image::PixelType* pixels(image.constPixelsAtRow(row));

      for (int col = 0; col < image.width(); col++, pixels++) {
        gammaCorrectPixel(pixels[0], rgb, gammaCurve);
        writePixel8Bit(rgb);
      }
    }
  }

  void writeImageBytes16Bit(const Image& image, const GammaCurve& gammaCurve)
  {
    Image::PixelType rgb;

    for (int row = 0; row < image.height(); row++) {
      const Image::PixelType* pixels(image.constPixelsAtRow(row));

      for (int col = 0; col < image.width(); col++, pixels++) {
        gammaCorrectPixel(pixels[0], rgb, gammaCurve);
        writePixel16Bit(rgb);
      }
    }
  }

public:
  PpmImageWriter(std::ostream& output)
    : mOutput(output), mOutputStream(*(output.rdbuf())) {}

  void writeImage(const Image& image, unsigned int colorDepth)
  {
    mOutput << "P6\n";
    mOutput << image.width() << " " << image.height() << "\n";
    mOutput << ((1 << colorDepth) - 1) << "\n";

    GammaCurve gammaCurve(buildGammaCurve(image));

    if (colorDepth == 8) {
      writeImageBytes8Bit(image, gammaCurve);
    } else {
      writeImageBytes16Bit(image, gammaCurve);
    }
  }
};

#include <iostream>

void ImageWriter::writeImage(
    const Image& image, std::ostream& ostream, const char* type,
    unsigned int colorDepth)
{
  if (!type) type = "ppm";

  std::string stype(type);
  boost::algorithm::to_upper(stype);

  if (stype == "PPM") {
    PpmImageWriter(ostream).writeImage(image, colorDepth);
  } else {
    throw "FIXME";
  }
}

void ImageWriter::writeImage(
    const Image& image, const char* filename, const char* type,
    unsigned int colorDepth)
{
  std::ofstream out(filename);
  writeImage(image, out, type);
}

} // namespace refinery
