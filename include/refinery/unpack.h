#ifndef _REFINERY_UNPACK_H
#define _REFINERY_UNPACK_H

#include <cstddef>
#include <iosfwd>

namespace Exiv2 {
  class ExifData;
}

namespace refinery {

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
   * Depending on mimeType, width/height/exifData may be needed. The rule of
   * thumb: you don't need them for image/x-portable-pixmap but you need it for
   * everything else. (You can read width and height from an Exiv2::ExifImage.)
   */
  Image* readImage(
      std::streambuf& istream, const char* mimeType,
      int width = 0, int height = 0, const Exiv2::ExifData* exifData = 0);
};

}

#endif /* _REFINERY_UNPACK_H */
