#include <gtest/gtest.h>

#include "refinery/input.h"

namespace {

class BufferInputStreamTest : public ::testing::Test {
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

} // namespace
