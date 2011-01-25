#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <memory>

#include "refinery/interpolate.h"

#include "refinery/camera.h"
#include "refinery/exif.h"
#include "refinery/image.h"

namespace {

class InterpolatorTest : public ::testing::Test {
};

#include "files/nikon_d5000_225x75_sample.h"

TEST(InterpolatorTest, AHDInterpolate) {
  refinery::InMemoryExifData exifData;
  exifData.setString("Exif.Image.Model", "NIKON D5000");

  refinery::CameraData cameraData(
      refinery::CameraDataFactory::instance().getCameraData(exifData));

  refinery::GrayImage grayImage(cameraData, 225, 75);
  grayImage.setFilters(0x61616161);
  std::copy(
      &nikon_d5000_225x75_sample[0], &nikon_d5000_225x75_sample[225*75],
      reinterpret_cast<unsigned short*>(grayImage.pixels()));

  refinery::Interpolator interpolator(refinery::Interpolator::INTERPOLATE_AHD);
  std::auto_ptr<refinery::Image> imagePtr(interpolator.interpolate(grayImage));
  const refinery::Image& image(*imagePtr);

  refinery::Image ref(cameraData, 225, 75);
  ref.setBytesPerPixel(3);

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

      const refinery::Image::PixelType p(image.constPixelAtPoint(row, col));
      if (nFailures < 3) {
        EXPECT_EQ(refPixel.r, p.r()) << "(" << row << ", " << col << ")";
        EXPECT_EQ(refPixel.g, p.g()) << "(" << row << ", " << col << ")";
        EXPECT_EQ(refPixel.b, p.b()) << "(" << row << ", " << col << ")";
        if (refPixel.r != p.r() || refPixel.g != p.g() || refPixel.b != p.b()) {
          nFailures++;
        }
      } else {
        ASSERT_EQ(refPixel.r, p.r());
        ASSERT_EQ(refPixel.g, p.g());
        ASSERT_EQ(refPixel.b, p.b());
      }
    }
  }
}

}
