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

class PpmImageWriter {
  std::ostream& mOutput;
  std::streambuf& mOutputStream;

  void writePixel8Bit(const Image::PixelType& rgb)
  {
    mOutputStream.sputc(rgb.r() >> 8);
    mOutputStream.sputc(rgb.g() >> 8);
    mOutputStream.sputc(rgb.b() >> 8);
  }

  void writePixel16Bit(const Image::PixelType& rgb)
  {
    mOutputStream.sputc(rgb.r() >> 8);
    mOutputStream.sputc(rgb.r() & 0xff);
    mOutputStream.sputc(rgb.g() >> 8);
    mOutputStream.sputc(rgb.g() & 0xff);
    mOutputStream.sputc(rgb.b() >> 8);
    mOutputStream.sputc(rgb.b() & 0xff);
  }

  void writeImageBytes8Bit(const Image& image)
  {
    const Image::PixelType* pixel(image.constPixels());
    const Image::PixelType* endPixel(image.constPixelsEnd());

    while (pixel < endPixel) {
      writePixel8Bit(*pixel);
      pixel++;
    }
  }

  void writeImageBytes16Bit(const Image& image)
  {
    const Image::PixelType* pixel(image.constPixels());
    const Image::PixelType* endPixel(image.constPixelsEnd());

    while (pixel < endPixel) {
      writePixel16Bit(*pixel);
      pixel++;
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

    if (colorDepth == 8) {
      writeImageBytes8Bit(image);
    } else {
      writeImageBytes16Bit(image);
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
  std::ofstream out(
      filename,
      std::ios::out | std::ios::binary | std::ios::trunc);
  writeImage(image, out, type, colorDepth);
}

} // namespace refinery
