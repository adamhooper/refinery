#ifndef _REFINERY_INTERPOLATE_H
#define _REFINERY_INTERPOLATE_H

#include <cstddef>

namespace refinery {

template<typename T, std::size_t N> class TypedImage;
typedef TypedImage<unsigned short, 3> Image;

class Interpolator {
public:
  enum Type {
    INTERPOLATE_AHD
  };

private:
  Type mType;

public:
  Interpolator(const Type& type);

  void interpolate(Image& image);
};

} /* namespace */

#endif /* _REFINERY_INTERPOLATE_H */
