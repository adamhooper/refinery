#ifndef _REFINERY_UNPACK_H
#define _REFINERY_UNPACK_H

#include <iosfwd>
#include <cstdio>

namespace refinery {

class ExifData;

template<typename T> class TypedImage;
template<typename T> class GrayPixel;
typedef TypedImage<GrayPixel<unsigned short> > GrayImage;
template<typename T> class RGBPixel;
typedef TypedImage<RGBPixel<unsigned short> > RGBImage;

class ImageReader {
public:
  /**
   * Returns a new Image based on the input.
   *
   * When reading is done, the stream will be seeked one byte past the last
   * byte in the image.
   *
   * With most image types, you can read with and height using an
   * Exiv2::ExifImage. A notable exception is PPM: if you read an
   * image/x-portable-pixmap file the width and height will be ignored. You
   * must still pass in an ExifData& though.
   *
   * FIXME: readRgbImage() only makes sense for PPM (or some unusual RAW files).
   * Clarify API.
   */
  GrayImage* readGrayImage(
      std::streambuf& istream, const char* mimeType,
      int width, int height, const ExifData& exifData);
  GrayImage* readGrayImage(
      FILE* istream, const char* mimeType,
      int width, int height, const ExifData& exifData);

  RGBImage* readRgbImage(
      std::streambuf& istream, const char* mimeType,
      int width, int height, const ExifData& exifData);
  RGBImage* readRgbImage(
      FILE* istream, const char* mimeType,
      int width, int height, const ExifData& exifData);
};

}

#endif /* _REFINERY_UNPACK_H */
