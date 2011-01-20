#ifndef _REFINERY_OUTPUT_H
#define _REFINERY_OUTPUT_H

#include <iosfwd>

namespace refinery {

template<typename T> class RGBPixel;
template<typename T> class TypedImage;
typedef TypedImage<RGBPixel<unsigned short> > Image;

class ImageWriter {
public:
  void writeImage(
      const Image& image, std::ostream& ostream, const char* type = 0,
      unsigned int colorDepth = 8);
  void writeImage(
      const Image& image, const char* filename, const char* type = 0,
      unsigned int colorDepth = 8);
};

} // namespace refinery

#endif /* REFINERY_OUTPUT_H */
