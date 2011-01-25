#include <gtest/gtest.h>

#include "refinery/output.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include <exiv2/exif.hpp>

#include "refinery/exif.h"
#include "refinery/image.h"
#include "refinery/unpack.h"

namespace {

class ImageWriterTest : public ::testing::Test {
};

TEST(ImageWriterTest, WritePpm16Bit) {
  // Assume PPM input works and load up an Image
  std::filebuf fb;
  fb.open(
      "./test/files/nikon_d5000_225x75_sample_ahd16.ppm",
      std::ios::in | std::ios::binary);

  refinery::ImageReader reader;
  refinery::InMemoryExifData exifData;
  std::auto_ptr<refinery::Image> imagePtr(
      reader.readRgbImage(fb, "image/x-portable-pixmap", 0, 0, exifData));
  refinery::Image& image(*imagePtr);

  // Now we have "image" so the test can begin.
  std::ostringstream out(std::ios::binary | std::ios::out);

  refinery::ImageWriter writer;
  writer.writeImage(image, out, "PPM", 16);

  std::string s(out.str());
  EXPECT_EQ("P6\n", s.substr(0, 3));
  EXPECT_EQ("225 75\n", s.substr(3, 7));
  EXPECT_EQ("65535\n", s.substr(10, 6));
  EXPECT_EQ(101266, s.size());

  EXPECT_EQ(7, static_cast<unsigned char>(s.at(17))) << "first byte of data";
  EXPECT_EQ(0xa7, static_cast<unsigned char>(s.at(101265))) << "last byte of data";
}

TEST(ImageWriterTest, WritePpm8Bit) {
  // Assume PPM input works and load up an Image
  std::filebuf fb;
  fb.open(
      "./test/files/nikon_d5000_225x75_sample_ahd16.ppm",
      std::ios::in | std::ios::binary);

  refinery::ImageReader reader;

  refinery::InMemoryExifData exifData;
  std::auto_ptr<refinery::Image> imagePtr(
      reader.readRgbImage(fb, "image/x-portable-pixmap", 0, 0, exifData));
  refinery::Image& image(*imagePtr);

  // Now we have "image" so the test can begin.
  std::ostringstream out(std::ios::binary | std::ios::out);

  refinery::ImageWriter writer;
  writer.writeImage(image, out, "PPM", 8);

  std::string s(out.str());
  EXPECT_EQ("P6\n", s.substr(0, 3));
  EXPECT_EQ("225 75\n", s.substr(3, 7));
  EXPECT_EQ("255\n", s.substr(10, 4));
  EXPECT_EQ(50639, s.size());

  EXPECT_EQ(0x9e, static_cast<unsigned char>(s.at(15))) << "first byte of data";
  EXPECT_EQ(0xcc, static_cast<unsigned char>(s.at(50638))) << "last byte of data";
}

} // namespace
