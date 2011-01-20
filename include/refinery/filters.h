#ifndef _REFINERY_FILTERS_H
#define _REFINERY_FILTERS_H

#include <cstddef>

namespace refinery {

template<typename T> class RGBPixel;
template<typename T> class TypedImage;
typedef TypedImage<RGBPixel<unsigned short> > Image;

class ScaleColorsFilter {
public:
  void filter(Image& image);
};

class ConvertToRgbFilter {
public:
  void filter(Image& image);
};

} // namespace refinery

#endif /* REFINERY_FILTERS_H */
