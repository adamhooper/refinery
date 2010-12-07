#include <gtest/gtest.h>

#include "refinery/input.h"

namespace {

class HuffmanDecoderTest : public ::testing::Test {
};

TEST(HuffmanDecoderTest, NextValue) {
  const unsigned char treeSpec[] = { // Nikon 12-bit lossy
    0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,
    5,4,3,6,2,7,1,0,8,9,11,10,12,0
  };
  const char buf[] = {
    0xd2, 0xf5, 0x16, 0x14, 0xaa, 0xaa
  };

  std::string stringBuf(buf, sizeof(buf));
  std::stringbuf stream(stringBuf);

  refinery::HuffmanDecoder decoder(stream, treeSpec);

  EXPECT_EQ(0x07, decoder.nextHuffmanValue());
  EXPECT_EQ(0x4b, decoder.nextBitsValue(7));
  EXPECT_EQ(0x07, decoder.nextHuffmanValue());
  EXPECT_EQ(0x51, decoder.nextBitsValue(7));
  EXPECT_EQ(0x03, decoder.nextHuffmanValue());
  EXPECT_EQ(0x00, decoder.nextBitsValue(3));
  EXPECT_EQ(0x04, decoder.nextHuffmanValue());
  EXPECT_EQ(0x09, decoder.nextBitsValue(4));
}

} // namespace
