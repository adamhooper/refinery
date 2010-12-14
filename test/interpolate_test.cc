#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

#include "refinery/interpolate.h"

#include "refinery/camera.h"
#include "refinery/image.h"

#include <exiv2/exif.hpp>

namespace {

class InterpolatorTest : public ::testing::Test {
};

#include "files/nikon_d5000_225x75_sample.h"

TEST(InterpolatorTest, AHDInterpolate) {
  Exiv2::ExifData exifData;
  exifData["Exif.Image.Model"] = "NIKON D5000";

  refinery::CameraData cameraData(
      refinery::CameraDataFactory::instance().getCameraData(exifData));

  refinery::Image image(cameraData, 225, 75);
  image.setBytesPerPixel(3);
  image.setFilters(0x61616161);
  image.pixels().assign(
      &nikon_d5000_225x75_sample[0], &nikon_d5000_225x75_sample[225*75*3]);

  refinery::Interpolator interpolator(refinery::Interpolator::INTERPOLATE_AHD);
  interpolator.interpolate(image);

  refinery::Image ref(cameraData, 225, 75);
  ref.setBytesPerPixel(3);
  refinery::Image::PixelsType& pixels(ref.pixels());
  pixels.assign(ref.width() * ref.height() * ref.bytesPerPixel(), 0);

  std::ifstream f;
  f.open("test/files/nikon_d5000_225x75_sample_ahd16.ppm");
  std::string _ignore;
  std::getline(f, _ignore); // "P6"
  std::getline(f, _ignore); // "225 75"
  std::getline(f, _ignore); // "65535"

  struct {
    unsigned short r;
    unsigned short g;
    unsigned short b;
  } refPixel;

  int nFailures = 0;

  for (int row = 0; row < 75; row++) {
    for (int col = 0; col < 225; col++) {
      char msb, lsb;
      f.get(msb);
      f.get(lsb);
      refPixel.r =
          static_cast<unsigned short>(static_cast<unsigned char>(msb)) << 8
          | static_cast<unsigned char>(lsb);
      f.get(msb);
      f.get(lsb);
      refPixel.g =
          static_cast<unsigned short>(static_cast<unsigned char>(msb)) << 8
          | static_cast<unsigned char>(lsb);
      f.get(msb);
      f.get(lsb);
      refPixel.b =
          static_cast<unsigned short>(static_cast<unsigned char>(msb)) << 8
          | static_cast<unsigned char>(lsb);

      refinery::Image::PixelType p(image.pixel(refinery::Point(row, col)));
      if (nFailures < 3) {
        EXPECT_EQ(refPixel.r, p[0]) << "(" << row << ", " << col << ")";
        EXPECT_EQ(refPixel.g, p[1]) << "(" << row << ", " << col << ")";
        EXPECT_EQ(refPixel.b, p[2]) << "(" << row << ", " << col << ")";
        if (refPixel.r != p[0] || refPixel.g != p[1] || refPixel.b != p[2]) {
          nFailures++;
        }
      } else {
        ASSERT_EQ(refPixel.r, p[0]);
        ASSERT_EQ(refPixel.g, p[1]);
        ASSERT_EQ(refPixel.b, p[2]);
      }
    }
  }
}

}
