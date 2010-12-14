#ifndef _REFINERY_FILTERS_H
#define _REFINERY_FILTERS_H

#include <cstddef>

namespace refinery {

template<typename T, std::size_t N> class TypedImage;
typedef TypedImage<unsigned short, 3> Image;

class ScaleColorsFilter {
public:
  void filter(Image& image);
};

} // namespace refinery

#endif /* REFINERY_FILTERS_H */
