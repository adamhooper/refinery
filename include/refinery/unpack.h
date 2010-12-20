#ifndef _REFINERY_UNPACK_H
#define _REFINERY_UNPACK_H

#include <cstddef>
#include <iosfwd>

namespace refinery {

class ExifData;

template<typename T, std::size_t N> class TypedImage;
typedef TypedImage<unsigned short, 3> Image;

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
   */
  Image* readImage(
      std::streambuf& istream, const char* mimeType,
      int width, int height, const ExifData& exifData);
};

}

#endif /* _REFINERY_UNPACK_H */
