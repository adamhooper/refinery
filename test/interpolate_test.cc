#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

#include "refinery/interpolate.h"

#include "refinery/image.h"

namespace {

class InterpolatorTest : public ::testing::Test {
};

#include "files/nikon_d5000_225x75_sample.h"

TEST(InterpolatorTest, AHDInterpolate) {
  refinery::Image image(225, 75);
  image.setBytesPerPixel(3);
  image.setFilters(0x61616161);
  image.pixels().assign(
      &nikon_d5000_225x75_sample[0], &nikon_d5000_225x75_sample[225*75*3]);

  refinery::Interpolator interpolator(refinery::Interpolator::INTERPOLATE_AHD);
  interpolator.interpolate(image);

  using refinery::Point;
  refinery::Image::PixelType p0_0(image.pixel(Point(0, 0))); // corner
  refinery::Image::PixelType p1_1(image.pixel(Point(1, 1))); // still in border
  refinery::Image::PixelType p2_50(image.pixel(Point(2, 50))); // also in border
  refinery::Image::PixelType p9_9(image.pixel(Point(9, 9))); // run-of-the-mill
  refinery::Image::PixelType p74_224(image.pixel(Point(74, 224))); // far corner

  EXPECT_EQ(209, p0_0[0]);
  EXPECT_EQ(357, p0_0[1]);
  EXPECT_EQ(237, p0_0[2]);

  EXPECT_EQ(260, p1_1[0]);
  EXPECT_EQ(423, p1_1[1]);
  EXPECT_EQ(225, p1_1[2]);

  EXPECT_EQ(315, p2_50[0]);
  EXPECT_EQ(524, p2_50[1]);
  EXPECT_EQ(275, p2_50[2]);

  EXPECT_EQ(391, p9_9[0]);
  EXPECT_EQ(676, p9_9[1]);
  EXPECT_EQ(420, p9_9[2]);

  EXPECT_EQ(360, p74_224[0]);
  EXPECT_EQ(664, p74_224[1]);
  EXPECT_EQ(588, p74_224[2]);
}

}
