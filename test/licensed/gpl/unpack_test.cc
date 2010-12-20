/*
 * Copyright (c) 2010 Adam Hooper
 *
 * This file is part of refinery (GPL portion).
 *
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include "refinery/exif.h"
#include "refinery/exif_exiv2.h"
#include "refinery/unpack.h"

#include <fstream>
#include <memory>

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

#include "refinery/image.h"

namespace {

class ImageReaderTest : public ::testing::Test {
};

TEST(ImageReaderTest, NikonD5000) {
  const char* path = "./test/files/nikon-d5000-1.nef";

  Exiv2::Image::AutoPtr exivImage(Exiv2::ImageFactory::open(path));

  exivImage->readMetadata();
  Exiv2::ExifData& exiv2ExifData(exivImage->exifData());
  refinery::Exiv2ExifData exifData(exiv2ExifData);

  int width = exivImage->pixelWidth();
  int height = exivImage->pixelHeight();
  const char* mimeType = exivImage->mimeType().c_str();

  std::filebuf fb;
  fb.open(path, std::ios::in | std::ios::binary);

  refinery::ImageReader reader;

  std::auto_ptr<refinery::Image> imagePtr(
      reader.readImage(fb, mimeType, width, height, exifData));
  refinery::Image& image(*imagePtr);

  EXPECT_EQ(6, image.bytesPerPixel());
  EXPECT_EQ(height, image.height());
  EXPECT_EQ(width, image.width());

  EXPECT_EQ(image.bytesPerPixel() / 2 * image.height() * image.width(), image.pixels().size());

  // Spot-check: first pixel, last pixel, a quad in the middle
  const unsigned short (*row)[3];

  row = image.pixelsRow(0);
  EXPECT_EQ(0, row[0][0]) << "row 0";
  EXPECT_EQ(1127, row[0][1]) << "row 0";
  EXPECT_EQ(0, row[0][2]) << "row 0";

  row = image.pixelsRow(312);
  EXPECT_EQ(0, row[512][0]) << "row 312";
  EXPECT_EQ(1522, row[512][1]) << "row 312";
  EXPECT_EQ(0, row[512][2]) << "row 312";

  EXPECT_EQ(0, row[513][0]) << "row 312";
  EXPECT_EQ(0, row[513][1]) << "row 312";
  EXPECT_EQ(1704, row[513][2]) << "row 312";

  row = image.pixelsRow(313);
  EXPECT_EQ(604, row[512][0]) << "row 313";
  EXPECT_EQ(0, row[512][1]) << "row 313";
  EXPECT_EQ(0, row[512][2]) << "row 313";

  EXPECT_EQ(0, row[513][0]) << "row 313";
  EXPECT_EQ(1529, row[513][1]) << "row 313";
  EXPECT_EQ(0, row[513][2]) << "row 313";

  row = image.pixelsRow(2867);
  EXPECT_EQ(0, row[4309][0]) << "row 2867";
  EXPECT_EQ(851, row[4309][1]) << "row 2867";
  EXPECT_EQ(0, row[4309][2]) << "row 2867";
};

TEST(ImageReaderTest, Ppm16Bit) {
  std::filebuf fb;
  fb.open(
      "./test/files/nikon_d5000_225x75_sample_ahd16.ppm",
      std::ios::in | std::ios::binary);

  refinery::ImageReader reader;

  refinery::InMemoryExifData exifData;
  std::auto_ptr<refinery::Image> imagePtr(
      reader.readImage(fb, "image/x-portable-pixmap", 0, 0, exifData));
  refinery::Image& image(*imagePtr);

  EXPECT_EQ(101266, fb.pubseekoff(0, std::ios::cur));
  EXPECT_EQ(225, image.width());
  EXPECT_EQ(75, image.height());
  EXPECT_EQ(6, image.bytesPerPixel());
  EXPECT_EQ(3 * image.width() * image.height(), image.pixels().size());

  // Spot-check
  refinery::Image::PixelType pixel0_0(image.pixel(refinery::Point(0, 0)));
  EXPECT_EQ(209, pixel0_0[0]);
  EXPECT_EQ(357, pixel0_0[1]);
  EXPECT_EQ(237, pixel0_0[2]);

  refinery::Image::PixelType pixel74_224(image.pixel(refinery::Point(74, 224)));
  EXPECT_EQ(360, pixel74_224[0]);
  EXPECT_EQ(664, pixel74_224[1]);
  EXPECT_EQ(588, pixel74_224[2]);
}

} // namespace
