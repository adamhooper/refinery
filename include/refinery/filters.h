#ifndef _REFINERY_FILTERS_H
#define _REFINERY_FILTERS_H

#include <cstddef>

namespace refinery {

class ScaleColorsFilter {
public:
  template<typename T> void filter(T& image);
};

class ConvertToRgbFilter {
public:
  template<typename T> void filter(T& image);
};

class GammaFilter {
public:
  // This ought to be a 1-param template but Swig can't resolve it:
  // void filter(T& image, const GammaCurve<typename T::ValueType>& gammaCurve);
  template<typename ImageType, typename GammaCurveType>
    void filter(ImageType& image, const GammaCurveType& gammaCurve);
};

} // namespace refinery

#endif /* REFINERY_FILTERS_H */
