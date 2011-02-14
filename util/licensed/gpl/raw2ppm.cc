/*
 * Copyright (c) 2010 Adam Hooper
 *
 * This file is part of refinery (GPL portion).
 *
 * refinery (GPL portion) is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * refinery (GPL portion) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with refinery (GPL portion).  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "refinery/exif.h"
#include "refinery/exif_exiv2.h"
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
  Exiv2::ExifData& exiv2ExifData(exivImage->exifData());
  if (exiv2ExifData.empty()) {
    throw Exiv2::Error(1, std::string(argv[1]) + " is missing Exif data");
  }

  Exiv2ExifData exifData(exiv2ExifData);

  std::filebuf fb;
  fb.open(argv[1], std::ios::in | std::ios::binary);

  ImageReader reader;

  int width = exivImage->pixelWidth();
  int height = exivImage->pixelHeight();
  const char* mimeType = exivImage->mimeType().c_str();

  std::auto_ptr<GrayImage> grayImagePtr(
      reader.readGrayImage(fb, mimeType, width, height, exifData));

  ScaleColorsFilter scaleFilter;
  scaleFilter.filter(*grayImagePtr);

  Interpolator interpolator(Interpolator::INTERPOLATE_AHD);
  std::auto_ptr<Image> imagePtr(interpolator.interpolate(*grayImagePtr));

  Image& image(*imagePtr);

  ConvertToRgbFilter rgbFilter;
  rgbFilter.filter(image);

  Histogram<Image, 3> histogram(image);
  GammaCurve<Image::ValueType> gammaCurve(histogram);
  GammaFilter gammaFilter;
  gammaFilter.filter(image, gammaCurve);

  ImageWriter writer;
  writer.writeImage(image, argv[2], "PPM", 8);

  return 0;
}
