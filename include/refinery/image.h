#ifndef _REFINERY_IMAGE_H
#define _REFINERY_IMAGE_H

#include <vector>

namespace refinery {

struct Point {
  /* Starting at top-left */
  int x;
  int y;

  Point() : x(0), y(0) {}
  Point(int x, int y) : x(x), y(y) {}
  Point(const Point& point) : x(point.x), y(point.y) {}

  Point operator+(const Point& point) const {
    return Point(x + point.x, y + point.y);
  }

  Point operator-(const Point& point) const {
    return Point(x - point.x, y - point.y);
  }
};

template<typename T> struct RGB {
  T r;
  T g;
  T b;

  RGB(const T& r, const T& g, const T& b) : r(r), g(g), b(b) {}
  RGB(const RGB& rgb) : r(rgb.r), g(rgb.g), b(rgb.b) {}
  RGB() : r(0), g(0), b(0) {}
};

class Image {
public:
  typedef std::vector<unsigned short> PixelsType;
  typedef unsigned short (*Row3Type)[3];
  typedef const unsigned short (*ConstRow3Type)[3];

private:
  int mWidth;
  int mHeight;
  int mBpp; /* bytes per pixel */
  PixelsType mPixels;

public:
  Image(int width, int height);
  ~Image();

  int width() const { return mWidth; }
  int height() const { return mHeight; }
  int bytesPerPixel() const { return mBpp; }
  void setBytesPerPixel(int bpp) { mBpp = bpp; }
  const unsigned short* pixels() const { return &mPixels[0]; }
  PixelsType& pixels() { return mPixels; }

  ConstRow3Type pixelsRow3(int row) const {
    return reinterpret_cast<ConstRow3Type>(&mPixels[row * mWidth * 3]);
  }
  Row3Type pixelsRow3(int row) {
    return reinterpret_cast<Row3Type>(&mPixels[row * mWidth * 3]);
  }
};

#if 0
class ImageTile {
  Image mImage;
  Point mFirstPixel;
  int mWidth;
  int mHeight;

public:
  ImageTile(const Image& image, const Point& firstPoint, int width, int height)
    : mImage(image), mFirstPixel(firstPoint), mWidth(width), mHeight(height) {}

  Point absoluteToRelative(const Point& absolute) const {
    return absolute - mFirstPixel;
  }

  Point relativeToAbsolute(const Point& relative) const {
    return relative + mFirstPixel;
  }
};
#endif

}; /* namespace refinery */

#endif /* REFINERY_IMAGE_H */
