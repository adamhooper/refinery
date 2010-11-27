#ifndef _REFINERY_IMAGE_H
#define _REFINERY_IMAGE_H

#include <map>
#include <string>
#include <utility>

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

template<class T> class Image {
  T* mPixels;
  int mWidth;
  int mHeight;
public:

  int width() const { return mWidth; }
  int height() const { return mHeight; }

  const T& operator[](const Point& point) const {
    return mPixels[point.y * mWidth + point.x];
  }

  T& operator[](const Point& point) {
    return mPixels[point.y * mWidth + point.x];
  }
};

class Metadata {
  std::map<std::string, std::string> mData;

public:
  const std::string& operator[](const std::string& key) const {
    return mData[key];
  }
  std::string& operator[](const std::string& key) {
    return mData[key];
  }

  std::string& insert(const std::string& key, std::string& value) {
    mData.insert(std::pair(key, value));
  }
};

template<class T> class ImageTile<T> {
  Metadata mMetadata;
  Image<T> mImage;
  Point mFirstPixel;
  int mWidth;
  int mHeight;

public:
  ImageTile(const Image<T>& image, const Point& firstPoint, int width, int height)
    : mImage(image), mFirstPixel(firstPoint), mWidth(width), mHeight(height) {}

  Point absoluteToRelative(const Point& absolute) const {
    return absolute - mFirstPixel;
  }

  Point relativeToAbsolute(const Point& relative) const {
    return relative + mFirstPixel;
  }

  const T& operator[](const Point& point) const {
    Point absolute(relativeToAbsolute(point));
    return mImage[absolute];
  }

  T& operator[](const Point& point) {
    Point absolute(relativeToAbsolute(point));
    return mImage[absolute];
  }
};

class ImageReader_private;

class ImageReader {
  ImageReader_private* mPrivate;

public:
  ImageReader();
  ~ImageReader();

  Image* readImage(InputStream& istream);
};

}; /* namespace refinery */

#endif /* REFINERY_IMAGE_H */
