#ifndef _REFINERY_UNPACK_H
#define _REFINERY_UNPACK_H

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace refinery {

template<typename T, std::size_t N> class TypedImage;
typedef TypedImage<unsigned short, 3> Image;

struct UnpackSettings {
  enum ImageFormat {
    FORMAT_PPM,
    FORMAT_NEF_COMPRESSED_LOSSY_2
  } format;
  int width;
  int height;
  int length;
  int bps;
  char version0;
  char version1;
  int filters;
  unsigned short vpred[2][2];
  std::vector<unsigned short> linearization_table;
  unsigned short split;
};

class ImageReader {
public:
  /**
   * Returns a new Image based on the input.
   *
   * The stream must be positioned at the first character of the image. In the
   * case of a TIFF-like file (NEF, etc), that will take sleuthing. Suggesion:
   * use libexiv2 to find Exif.(Image|SubImage1|SubImage2).StripOffsets, and
   * pass that to std::streambuf::pubseekoff().
   *
   * When reading is done, the stream will be seeked one byte past the last
   * byte in the image.
   */
  Image* readImage(std::streambuf& istream, const UnpackSettings& settings);
};

}

#endif /* _REFINERY_UNPACK_H */
