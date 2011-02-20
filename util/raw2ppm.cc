#include "refinery/exif.h"
#include "refinery/filters.h"
#include "refinery/gamma.h"
#include "refinery/image.h"
#include "refinery/interpolate.h"
#include "refinery/histogram.h"
#include "refinery/output.h"
#include "refinery/unpack.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

using namespace refinery;

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " INFILE OUTFILE" << std::endl;
    return 1;
  }

  std::filebuf fb;
  fb.open(argv[1], std::ios::in | std::ios::binary);

  refinery::DcrawExifData exifData(fb);

  ImageReader reader;
  std::auto_ptr<GrayImage> grayImagePtr(reader.readGrayImage(fb, exifData));

  ScaleColorsFilter scaleFilter;
  scaleFilter.filter(*grayImagePtr);

  Interpolator interpolator(Interpolator::INTERPOLATE_AHD);
  std::auto_ptr<RGBImage> imagePtr(interpolator.interpolate(*grayImagePtr));

  RGBImage& image(*imagePtr);

  ConvertToRgbFilter rgbFilter;
  rgbFilter.filter(image);

  Histogram<RGBImage, 3> histogram(image);
  GammaCurve<RGBImage::ValueType> gammaCurve(histogram);
  GammaFilter gammaFilter;
  gammaFilter.filter(image, gammaCurve);

  ImageWriter writer;
  writer.writeImage(image, argv[2], 8);

  return 0;
}
