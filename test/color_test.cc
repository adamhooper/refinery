#include <gtest/gtest.h>

#include "refinery/color.h"

namespace {

template<typename T>
class ColorConverterTest : public ::testing::Test {
protected:
  typedef refinery::ColorConverter<T, 4, 3> ConverterType;
  ConverterType* converter;

public:
  virtual void SetUp() {
    const T matrix[3][4] = {
      { 1.0, 2.0, 3.0, 4.0 },
      { 5.0, 6.0, 7.0, 8.0 },
      { 9.0, 10.0, 11.0, 12.0 }
    };

    converter = new ConverterType(matrix);
  }

  virtual void TearDown() {
    delete converter;
  }
};

typedef ::testing::Types<int, float, double> TestTypes;
TYPED_TEST_CASE(ColorConverterTest, TestTypes);

TYPED_TEST(ColorConverterTest, DoubleInDoubleOut) {
  const double inDouble[4] = { 1.0, 2.0, 3.0, 4.0 };
  double outDouble[3] = { 1.0, 2.0, 3.0 };

  this->converter->convert(inDouble, outDouble);
  EXPECT_EQ(30.0, outDouble[0]);
  EXPECT_EQ(70.0, outDouble[1]);
  EXPECT_EQ(110.0, outDouble[2]);
}

TYPED_TEST(ColorConverterTest, DoubleInFloatOut) {
  const double inDouble[4] = { 1.0, 2.0, 3.0, 4.0 };
  float outFloat[3] = { 1.0f, 2.0f, 3.0f };

  this->converter->convert(inDouble, outFloat);
  EXPECT_EQ(30.0f, outFloat[0]);
  EXPECT_EQ(70.0f, outFloat[1]);
  EXPECT_EQ(110.0f, outFloat[2]);
}

TYPED_TEST(ColorConverterTest, ConvertOnly3) {
  const double inDouble[3] = { 1.0, 2.0, 3.0 };
  double outDouble[3] = { 1.0, 2.0, 3.0 };

  this->converter->convert(inDouble, outDouble);
  EXPECT_EQ(14.0, outDouble[0]);
  EXPECT_EQ(38.0, outDouble[1]);
  EXPECT_EQ(62.0, outDouble[2]);
}

} // namespace
