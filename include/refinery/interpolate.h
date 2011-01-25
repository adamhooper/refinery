#ifndef _REFINERY_INTERPOLATE_H
#define _REFINERY_INTERPOLATE_H

#include <cstddef>

namespace refinery {

template<typename T> class TypedImage;
template<typename T> class RGBPixel;
typedef TypedImage<RGBPixel<unsigned short> > Image;
template<typename T> class GrayPixel;
typedef TypedImage<GrayPixel<unsigned short> > GrayImage;

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

  Image* interpolate(const GrayImage& image);
};

} /* namespace */

#endif /* _REFINERY_INTERPOLATE_H */
