#ifndef _REFINERY_OUTPUT_H
#define _REFINERY_OUTPUT_H

#include <iosfwd>

namespace refinery {

template<typename T, std::size_t N> class TypedImage;
typedef TypedImage<unsigned short, 3> Image;

class ImageWriter {
public:
  void writeImage(
      const Image& image, std::ostream& ostream, const char* type = 0);
  void writeImage(
      const Image& image, const char* filename, const char* type = 0);
};

} // namespace refinery

#endif /* REFINERY_OUTPUT_H */
