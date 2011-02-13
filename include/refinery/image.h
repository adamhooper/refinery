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

template<typename T>
struct RGBPixel {
  typedef unsigned int ColorType;
  static const ColorType NColors = 3;
  typedef T ValueType;
  typedef T ArrayType[NColors];
  ArrayType rgb;

  RGBPixel() {
    rgb[0] = 0;
    rgb[1] = 0;
    rgb[2] = 0;
  }
  template<typename U> RGBPixel(const U (&rhs)[3]) {
    rgb[0] = rhs[0];
    rgb[1] = rhs[1];
    rgb[2] = rhs[2];
  }
  template<typename U> RGBPixel(const RGBPixel<U>& rhs) {
    rgb[0] = rhs[0];
    rgb[1] = rhs[1];
    rgb[2] = rhs[2];
  }
  inline ArrayType& array() {
    return rgb;
  }
  inline const ArrayType& constArray() const { return rgb; }
  inline const T& r() const { return rgb[0]; }
  inline const T& g() const { return rgb[1]; }
  inline const T& b() const { return rgb[2]; }
  inline T& r() { return rgb[0]; }
  inline T& g() { return rgb[1]; }
  inline T& b() { return rgb[2]; }
  inline T& operator[](const ColorType& index) { return rgb[index]; }
  inline const T& at(const ColorType& index) const { return rgb[index]; }
};

template<typename T> struct LABPixel {
  typedef unsigned int ColorType;
  typedef T ValueType;
  T l;
  T a;
  T b;

  LABPixel() : l(0), a(0), b(0) {}
  LABPixel(T l, T a, T b) : l(l), a(a), b(b) {}
  template<typename U> LABPixel(const U (&rhs)[3])
    : l(rhs[0]), a(rhs[1]), b(rhs[2]) {}
  template<typename U> LABPixel(const LABPixel<U>& rhs)
    : l(rhs.l), a(rhs.a), b(rhs.b) {}
};

template<typename T> struct GrayPixel {
  typedef unsigned int ColorType;
  typedef T ValueType;
  T value;

  GrayPixel() : value(0) {}
  GrayPixel(T value) : value(value) {}
  template<typename U> GrayPixel(const GrayPixel<U>& rhs) : value(rhs.value) {}
  GrayPixel& operator*(double rhs) {
    return GrayPixel(value * rhs);
  }
};

template<typename T> class TypedImage {
public:
  typedef T PixelType;
  typedef typename T::ValueType ValueType;
  typedef typename T::ColorType ColorType;

private:
  typedef std::vector<PixelType> PixelsType;
  CameraData mCameraData;
  int mWidth;
  int mHeight;
  int mBpp; /* bytes per pixel--BAD! Don't rely on this! */
  int mFilters;
  PixelsType mPixels;

  void allocate()
  {
    mPixels.assign(mWidth * mHeight, PixelType());
  }

public:
  TypedImage(const CameraData& cameraData, int width, int height)
    : mCameraData(cameraData), mWidth(width), mHeight(height), mBpp(0),
    mFilters(0)
  {
    this->allocate();
  }

  const CameraData& cameraData() const { return mCameraData; }
  unsigned int width() const { return mWidth; }
  unsigned int height() const { return mHeight; }
  unsigned int nPixels() const { return mWidth * mHeight; }
  int bytesPerPixel() const { return mBpp; }
  unsigned int filters() const { return mFilters; }
  void setBytesPerPixel(int bpp) { mBpp = bpp; }
  void setFilters(unsigned int filters) { mFilters = filters; }

  ColorType colorAtPoint(const Point& point) const {
    int row = point.row;
    int col = point.col;
    return (mFilters >> (((row << 1 & 14) | (col & 1)) << 1)) & 3;
  }
  ColorType colorAtPoint(unsigned int row, unsigned int col) const {
    return colorAtPoint(Point(row, col));
  }

  PixelType* pixels() { return &mPixels[0]; }
  PixelType* pixelsEnd() { return &mPixels[mWidth * mHeight]; }

  const PixelType* constPixels() const { return &mPixels[0]; }
  const PixelType* constPixelsEnd() const { return &mPixels[mWidth * mHeight]; }

  const PixelType* constPixelsAtRow(int row) const {
    return &mPixels[row * mWidth];
  }
  PixelType* pixelsAtRow(int row) {
    return &mPixels[row * mWidth];
  }

  PixelType* pixelsAtPoint(const Point& point) {
    int row = point.row;
    int col = point.col;
    return &mPixels[row * mWidth + col];
  }
  PixelType* pixelsAtPoint(unsigned int row, unsigned int col) {
    return pixelsAtPoint(Point(row, col));
  }
  const PixelType* constPixelsAtPoint(const Point& point) const {
    int row = point.row;
    int col = point.col;
    return &mPixels[row * mWidth + col];
  }
  const PixelType* constPixelsAtPoint(
      unsigned int row, unsigned int col) const {
    return constPixelsAtPoint(Point(row, col));
  }

  PixelType& pixelAtPoint(const Point& point) {
    return *pixelsAtPoint(point);
  }
  PixelType& pixelAtPoint(unsigned int row, unsigned int col) {
    return *pixelsAtPoint(row, col);
  }
  const PixelType& constPixelAtPoint(const Point& point) const {
    return *constPixelsAtPoint(point);
  }
  const PixelType& constPixelAtPoint(unsigned int row, unsigned int col) const {
    return *constPixelsAtPoint(row, col);
  }
};

typedef RGBPixel<unsigned short> u16RGBPixel;
typedef LABPixel<short> s16LABPixel;
typedef GrayPixel<unsigned short> u16GrayPixel;

typedef TypedImage<u16RGBPixel> RGBImage;
typedef TypedImage<s16LABPixel> LABImage;
typedef TypedImage<u16GrayPixel> GrayImage;

typedef RGBImage Image;

}; /* namespace refinery */

#endif /* REFINERY_IMAGE_H */
