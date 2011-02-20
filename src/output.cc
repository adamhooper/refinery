#include "refinery/output.h"

#include <fstream>
#include <iostream>

#include "refinery/image.h"

namespace refinery {

class PpmImageWriter {
  std::ostream& mOutput;
  std::streambuf& mOutputStream;

  void writePixel8Bit(const RGBImage::PixelType& rgb)
  {
    mOutputStream.sputc(rgb.r() >> 8);
    mOutputStream.sputc(rgb.g() >> 8);
    mOutputStream.sputc(rgb.b() >> 8);
  }

  void writePixel16Bit(const RGBImage::PixelType& rgb)
  {
    mOutputStream.sputc(rgb.r() >> 8);
    mOutputStream.sputc(rgb.r() & 0xff);
    mOutputStream.sputc(rgb.g() >> 8);
    mOutputStream.sputc(rgb.g() & 0xff);
    mOutputStream.sputc(rgb.b() >> 8);
    mOutputStream.sputc(rgb.b() & 0xff);
  }

  void writeImageBytes8Bit(const RGBImage& image)
  {
    const RGBImage::PixelType* pixel(image.constPixels());
    const RGBImage::PixelType* endPixel(image.constPixelsEnd());

    while (pixel < endPixel) {
      writePixel8Bit(*pixel);
      pixel++;
    }
  }

  void writeImageBytes16Bit(const RGBImage& image)
  {
    const RGBImage::PixelType* pixel(image.constPixels());
    const RGBImage::PixelType* endPixel(image.constPixelsEnd());

    while (pixel < endPixel) {
      writePixel16Bit(*pixel);
      pixel++;
    }
  }

public:
  PpmImageWriter(std::ostream& output)
    : mOutput(output), mOutputStream(*(output.rdbuf())) {}

  void writeImage(const RGBImage& image, unsigned int colorDepth)
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
    const RGBImage& image, std::ostream& ostream, unsigned int colorDepth)
{
  PpmImageWriter(ostream).writeImage(image, colorDepth);
}

void ImageWriter::writeImage(
    const RGBImage& image, const char* filename, unsigned int colorDepth)
{
  std::ofstream out(
      filename,
      std::ios::out | std::ios::binary | std::ios::trunc);
  writeImage(image, out, colorDepth);
}

} // namespace refinery
