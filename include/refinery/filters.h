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

} // namespace refinery

#endif /* REFINERY_FILTERS_H */
