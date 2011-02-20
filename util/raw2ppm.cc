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

  int width = exifData.getInt("Exif.SubImage2.ImageWidth");
  int height = exifData.getInt("Exif.SubImage2.ImageLength");
  const char* mimeType = exifData.mime_type();

  std::auto_ptr<GrayImage> grayImagePtr(
      reader.readGrayImage(fb, mimeType, width, height, exifData));

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
