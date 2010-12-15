#include "refinery/filters.h"
#include "refinery/image.h"
#include "refinery/interpolate.h"
#include "refinery/output.h"
#include "refinery/unpack.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include <exiv2/tags.hpp>

using namespace refinery;

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " INFILE OUTFILE" << std::endl;
    return 1;
  }

  Exiv2::Image::AutoPtr exivImage(Exiv2::ImageFactory::open(argv[1]));
  assert(exivImage.get() != 0);

  exivImage->readMetadata();
  const Exiv2::ExifData& exifData(exivImage->exifData());
  if (exifData.empty()) {
    throw Exiv2::Error(1, std::string(argv[1]) + " is missing Exif data");
  }

  std::filebuf fb;
  fb.open(argv[1], std::ios::in | std::ios::binary);

  ImageReader reader;

  int width = exivImage->pixelWidth();
  int height = exivImage->pixelHeight();
  const char* mimeType = exivImage->mimeType().c_str();

  std::auto_ptr<Image> imagePtr(
      reader.readImage(fb, mimeType, width, height, exifData));
  Image& image(*imagePtr);

  ScaleColorsFilter scaleFilter;
  scaleFilter.filter(image);

  Interpolator interpolator(Interpolator::INTERPOLATE_AHD);
  interpolator.interpolate(image);

  ConvertToRgbFilter rgbFilter;
  rgbFilter.filter(image);

  std::ofstream out(argv[2], std::ios::binary | std::ios::out);

  ImageWriter writer;
  writer.writeImage(image, out, "PPM", 8);

  return 0;
}
