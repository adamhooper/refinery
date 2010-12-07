#include <gtest/gtest.h>

#include "refinery/input.h"

namespace {

class BufferInputStreamTest : public ::testing::Test {
};

class FileInputStreamTest : public ::testing::Test {
};

class HuffmanDecoderTest : public ::testing::Test {
};

TEST(BufferInputStreamTest, Read) {
  const char buf[] = "1234567890";
  refinery::BufferInputStream bis(buf, 10);

  char out[5] = { '-', '-', '-', '-', '-' };
  EXPECT_EQ(4, bis.read(out, 4));
  EXPECT_EQ('1', out[0]);
  EXPECT_EQ('2', out[1]);
  EXPECT_EQ('3', out[2]);
  EXPECT_EQ('4', out[3]);
  EXPECT_EQ('-', out[4]);

  EXPECT_EQ(4, bis.read(out, 4));
  EXPECT_EQ('5', out[0]);
  EXPECT_EQ('6', out[1]);
  EXPECT_EQ('7', out[2]);
  EXPECT_EQ('8', out[3]);
  EXPECT_EQ('-', out[4]);
}

TEST(BufferInputStreamTest, ReadMax) {
  const char buf[] = "123";
  refinery::BufferInputStream bis(buf, 3);

  char out[4] = { '-', '-', '-', '-' };
  EXPECT_EQ(3, bis.read(out, 4));
  EXPECT_EQ('1', out[0]);
  EXPECT_EQ('2', out[1]);
  EXPECT_EQ('3', out[2]);
  EXPECT_EQ('-', out[3]);
}

TEST(BufferInputStreamTest, ReadZero) {
  const char buf[] = "123";
  refinery::BufferInputStream bis(buf, 3);

  char out[2] = { '-', '-' };
  EXPECT_EQ(0, bis.read(out, 0));
  EXPECT_EQ('-', out[0]);
}

TEST(BufferInputStreamTest, Seek) {
  const char buf[] = "1234567890";
  refinery::BufferInputStream bis(buf, 10);

  char out[4] = { '-', '-', '-', '-' };
  EXPECT_EQ(7, bis.seek(7, std::ios::beg));

  EXPECT_EQ(3, bis.read(out, 4));
  EXPECT_EQ('8', out[0]);
  EXPECT_EQ('9', out[1]);
  EXPECT_EQ('0', out[2]);
  EXPECT_EQ('-', out[3]);

  bis.seek(7, std::ios::beg);
  EXPECT_EQ(4, bis.seek(-3, std::ios::cur));
  EXPECT_EQ(9, bis.seek(-1, std::ios::end));
}

TEST(FileInputStreamTest, ReadMax) {
  // input-test.txt contains "12345\n" only
  refinery::FileInputStream fis("./test/files/input-test.txt");

  char out[7] = { '-', '-', '-', '-', '-', '-', '-' };
  EXPECT_EQ(6, fis.read(out, 7));
  EXPECT_EQ('1', out[0]);
  EXPECT_EQ('2', out[1]);
  EXPECT_EQ('3', out[2]);
  EXPECT_EQ('4', out[3]);
  EXPECT_EQ('5', out[4]);
  EXPECT_EQ('\n', out[5]);
  EXPECT_EQ('-', out[6]);

  EXPECT_EQ(0, fis.read(out, 999));
}

TEST(FileInputStreamTest, Seek) {
  // input-test.txt contains "12345\n" only
  refinery::FileInputStream fis("./test/files/input-test.txt");

  char out[2] = { '-', '-' };
  EXPECT_EQ(3, fis.seek(3, std::ios::beg));
  EXPECT_EQ(1, fis.read(out, 1));
  EXPECT_EQ('4', out[0]);
  EXPECT_EQ('-', out[1]);

  EXPECT_EQ(1, fis.read(out, 1));
  EXPECT_EQ('5', out[0]);
  EXPECT_EQ('-', out[1]);

  EXPECT_EQ(5, fis.tell());

  EXPECT_EQ(2, fis.seek(-3, std::ios::cur));
  EXPECT_EQ(1, fis.read(out, 1));
  EXPECT_EQ('3', out[0]);

  EXPECT_EQ(4, fis.seek(-2, std::ios::end));
  EXPECT_EQ(1, fis.read(out, 1));
  EXPECT_EQ('5', out[0]);
}

TEST(HuffmanDecoderTest, NextValue) {
  const unsigned char treeSpec[] = { // Nikon 12-bit lossy
    0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,
    5,4,3,6,2,7,1,0,8,9,11,10,12,0
  };
  const char buf[] = {
    0xd2, 0xf5, 0x16, 0x14, 0xaa, 0xaa
  };

  refinery::BufferInputStream bis(buf, sizeof(buf));

  refinery::HuffmanDecoder decoder(bis, treeSpec);

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
