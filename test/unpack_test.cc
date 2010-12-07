#include <gtest/gtest.h>

#include "refinery/unpack.h"

#include <memory>

#include "refinery/input.h"
#include "refinery/image.h"

namespace {

class ImageReaderTest : public ::testing::Test {
};

TEST(ImageReaderTest, NikonD5000) {
  refinery::FileInputStream fis("./test/files/nikon-d5000-1.nef");
  const int offset = 1083530; // Exif.SubImage2.StripOffsets
  fis.seek(offset, std::ios::beg);

  refinery::UnpackSettings settings;
  settings.bps = 12; // Exif.SubImage2.BitsPerSample12
  settings.width = 4352; // Exif.SubImage2.ImageWidth
  settings.height = 2868; // Exif.SubImage2.ImageLength
  settings.length = 9938252; // Exif.SubImage2.StripByteCounts
  settings.format = refinery::UnpackSettings::FORMAT_NEF_COMPRESSED_LOSSY_2;
      // Exif.SubImage2.Compression, Exif.Nikon3.NEFCompression
  // First stuff in linearization table:
  // 0x4420, 0x0148, 0x0148, 0x0148, 0x0148, 0x0101
  // (see http://lclevy.free.fr/nef/ to learn what it means)
  settings.version0 = 0x44;
  settings.version1 = 0x20;
  settings.vpred[0][0] = 0x0148;
  settings.vpred[0][1] = 0x0148;
  settings.vpred[1][0] = 0x0148;
  settings.vpred[1][1] = 0x0148;
  settings.filters = 0x4b4b4b4b;
  const unsigned short linearization_table[] = {
    // Exif.Nikon3.LinearizationTable
    0x0000, 0x0010, 0x0020, 0x0030, 0x0040, 0x0050, 0x0060, 0x0070,
    0x008f, 0x00af, 0x00cf, 0x00ef, 0x0111, 0x0141, 0x0171, 0x01a1,
    0x01d1, 0x020c, 0x024c, 0x028c, 0x02cc, 0x0312, 0x0362, 0x03b2,
    0x0402, 0x0455, 0x04b5, 0x0515, 0x0575, 0x05d6, 0x0646, 0x06b6,
    0x0726, 0x0796, 0x0815, 0x0895, 0x0915, 0x0995, 0x0a24, 0x0ab4,
    0x0b44, 0x0bd4, 0x0c72, 0x0d12, 0x0db2, 0x0e52, 0x0f00, 0x0fb0,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff
  };
  std::vector<unsigned short> linearization_vector(
    &linearization_table[0], &linearization_table[257]);
  settings.linearization_table = linearization_vector;
  settings.split = 0;

  refinery::ImageReader reader;

  fis.seek(offset, std::ios::beg);

  std::auto_ptr<refinery::Image> imagePtr(reader.readImage(fis, settings));
  refinery::Image& image(*imagePtr);

  EXPECT_EQ(offset + settings.length, fis.tell()) << "whole image was read";

  EXPECT_EQ(3, image.bytesPerPixel());
  EXPECT_EQ(settings.height, image.height());
  EXPECT_EQ(settings.width, image.width());

  EXPECT_EQ(image.bytesPerPixel() * image.height() * image.width(), image.pixels().size());

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
  refinery::FileInputStream fis("./test/files/nikon_d5000_225x75_sample_ahd16.ppm");
  refinery::UnpackSettings settings; // almost ignored. FIXME improve API
  settings.format = refinery::UnpackSettings::FORMAT_PPM;

  refinery::ImageReader reader;

  std::auto_ptr<refinery::Image> imagePtr(reader.readImage(fis, settings));
  refinery::Image& image(*imagePtr);

  EXPECT_EQ(101266, fis.tell());
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
