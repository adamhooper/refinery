#include <gtest/gtest.h>

#include <memory>

#include "refinery/histogram.h"

#include "refinery/camera.h"
#include "refinery/exif.h"
#include "refinery/image.h"

namespace {

static const unsigned short random_4x3_sample[] = {
  0x1165, 0xb0d1, 0x018c,
  0x20e5, 0xa15f, 0x00e9,
  0x315c, 0x90d9, 0x1111,
  0x40db, 0x8186, 0x113e,
  0x51b0, 0x70f3, 0x219b,
  0x60f3, 0x61b0, 0x218f,
  0x728c, 0x51f2, 0x3288,
  0x81a7, 0x42d4, 0x31a7,
  0x92fc, 0x3240, 0x4367,
  0xa1ea, 0x2330, 0x4135,
  0xb1e3, 0x1126, 0x5238,
  0xc104, 0x027c, 0x513b
};

std::auto_ptr<refinery::RGBImage> createRgbImage() {
  refinery::InMemoryExifData exifData;
  refinery::CameraData cameraData(
      refinery::CameraDataFactory::instance().getCameraData(exifData));

  std::auto_ptr<refinery::RGBImage> rgbImage(
      new refinery::RGBImage(cameraData, 4, 3));
  const refinery::RGBImage::ValueType* sample = random_4x3_sample;
  for (refinery::RGBImage::PixelType* pixel = rgbImage->pixels();
      pixel < rgbImage->pixelsEnd(); pixel++, sample += 3) {
    (*pixel)[0] = sample[0];
    (*pixel)[1] = sample[1];
    (*pixel)[2] = sample[2];
  }

  return rgbImage;
}

class HistogramTest : public ::testing::Test {
};

TEST(HistogramTest, NPixels) {
  std::auto_ptr<refinery::RGBImage> rgbImage(createRgbImage());

  refinery::Histogram<refinery::RGBImage, 3> histogram(*rgbImage);
  ASSERT_EQ(12, histogram.nPixels());
}

TEST(HistogramTest, Coarseness15) {
  std::auto_ptr<refinery::RGBImage> rgbImage(createRgbImage());

  refinery::Histogram<refinery::RGBImage, 15> histogram(*rgbImage);
  ASSERT_EQ(2, histogram.nSlots());

  EXPECT_EQ(7, histogram.count(0, 0));
  EXPECT_EQ(8, histogram.count(1, 0));
  EXPECT_EQ(12, histogram.count(2, 0));

  EXPECT_EQ(5, histogram.count(0, 1));
  EXPECT_EQ(4, histogram.count(1, 1));
  EXPECT_EQ(0, histogram.count(2, 1));
}

}
