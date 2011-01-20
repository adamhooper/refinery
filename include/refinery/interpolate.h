#ifndef _REFINERY_INTERPOLATE_H
#define _REFINERY_INTERPOLATE_H

#include <cstddef>

namespace refinery {

template<typename T> class RGBPixel;
template<typename T> class TypedImage;
typedef TypedImage<RGBPixel<unsigned short> > Image;

class Interpolator {
public:
  enum Type {
    INTERPOLATE_AHD,
    INTERPOLATE_BILINEAR
  };

private:
  Type mType;

public:
  Interpolator(const Type& type);

  void interpolate(Image& image);
};

} /* namespace */

#endif /* _REFINERY_INTERPOLATE_H */
