#ifndef _REFINERY_IMAGE_H
#define _REFINERY_IMAGE_H

#include <refinery/camera.h>

#include <cstddef>
#include <vector>

namespace refinery {

class CameraData;

struct Point {
  /* Starting at top-left */
  int row;
  int col;

  Point() : row(0), col(0) {}
  Point(int row, int col) : row(row), col(col) {}
  Point(const Point& point) : row(point.row), col(point.col) {}

  Point operator+(const Point& point) const {
    return Point(row + point.row, col + point.col);
  }

  Point operator-(const Point& point) const {
    return Point(row - point.row, col - point.col);
  }

  bool operator==(const Point& rhs) const {
    return this->row == rhs.row && this->col == rhs.col;
  }

  bool operator!=(const Point& rhs) const {
    return !(this->operator==(rhs));
  }
};

template<typename T, std::size_t N = 3> class TypedImage {
public:
  typedef T ValueType;
  typedef std::vector<ValueType> PixelsType;
  typedef ValueType (*RowType)[N];
  typedef const ValueType (*ConstRowType)[N];
  typedef ValueType* PixelType;
  typedef const ValueType* ConstPixelType;
  typedef unsigned int Color;

  static const Color R = 0;
  static const Color G = 1;
  static const Color B = 2;

private:
  CameraData mCameraData;
  int mWidth;
  int mHeight;
  int mBpp; /* bytes per pixel */
  int mFilters;
  PixelsType mPixels;

public:
  TypedImage(const CameraData& cameraData, int width = 0, int height = 0)
    : mCameraData(cameraData), mWidth(width), mHeight(height), mBpp(0),
    mFilters(0)
  {
  }

  const CameraData& cameraData() const { return mCameraData; }
  int width() const { return mWidth; }
  int height() const { return mHeight; }
  int bytesPerPixel() const { return mBpp; }
  int filters() const { return mFilters; }
  void setBytesPerPixel(int bpp) { mBpp = bpp; }
  void setFilters(int filters) { mFilters = filters; }
  void setWidth(int width) { mWidth = width; }
  void setHeight(int height) { mHeight = height; }
  template<class U> void importAttributes(const TypedImage<U>& other) {
    mWidth = other.width();
    mHeight = other.height();
    mBpp = other.bytesPerPixel();
    mFilters = other.filters();
  }

  Color colorAtPoint(const Point& point) const {
    int row = point.row;
    int col = point.col;
    return (mFilters >> (((row << 1 & 14) | (col & 1)) << 1)) & 3;
  }

  const PixelsType& constPixels() const { return mPixels; }
  PixelsType& pixels() { return mPixels; }

  ConstRowType constPixelsRow(int row) const {
    return reinterpret_cast<ConstRowType>(&mPixels[row * mWidth * N]);
  }
  RowType pixelsRow(int row) {
    return reinterpret_cast<RowType>(&mPixels[row * mWidth * N]);
  }

  PixelType pixel(const Point& point) {
    int row = point.row;
    int col = point.col;
    return &mPixels[(row * mWidth + col) * N];
  }
  ConstPixelType constPixel(const Point& point) const {
    int row = point.row;
    int col = point.col;
    return &mPixels[(row * mWidth + col) * N];
  }
};

template class TypedImage<unsigned short>;
template class TypedImage<short>;

typedef TypedImage<unsigned short, 3> RGBImage;
typedef RGBImage Image;
typedef TypedImage<short, 3> LABImage;

}; /* namespace refinery */

#endif /* REFINERY_IMAGE_H */
