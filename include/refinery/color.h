#ifndef _REFINERY_COLOR_H
#define _REFINERY_COLOR_H

#include <cstddef>

namespace refinery {

template<typename T> struct RGBColor;

template<typename T, std::size_t NFrom, std::size_t NTo>
class ColorConverter {
  T mMatrix[NTo][NFrom];

public:
  ColorConverter()
  {
    for (unsigned int i = 0; i < NTo; i++) {
      for (unsigned int j = 0; j < NFrom; j++) {
        mMatrix[i][j] = 0;
      }
    }
  }

  template<typename U> ColorConverter(const U (&matrix)[NTo][NFrom])
  {
    for (unsigned int i = 0; i < NTo; i++) {
      for (unsigned int j = 0; j < NFrom; j++) {
        mMatrix[i][j] = static_cast<T>(matrix[i][j]);
      }
    }
  }

  template<typename U, typename V> void convert(const U (&in)[NFrom], V (&out)[NTo])
  {
    for (unsigned int i = 0; i < NTo; i++) {
      out[i] = static_cast<V>(0);
    }

    for (unsigned int i = 0; i < NFrom; i++) {
      for (unsigned int j = 0; j < NTo; j++) {
        out[j] += mMatrix[j][i] * in[i];
      }
    }
  }

  /**
   * Sometimes we treat a camera's output as 3-color even when it's 4.
   *
   * For that, we need a special case.
   */
  template<typename U, typename V> void convert(const U (&in)[NFrom - 1], V (&out)[NTo])
  {
    for (unsigned int i = 0; i < NTo; i++) {
      out[i] = static_cast<V>(0);
    }

    for (unsigned int i = 0; i < NFrom - 1; i++) {
      for (unsigned int j = 0; j < NTo; j++) {
        out[j] += mMatrix[j][i] * in[i];
      }
    }
  }

  template<typename U, typename V> void convert(const U& in, V& out)
  {
    out = V();

    for (unsigned int i = 0; i < 4; i++) {
      for (unsigned int j = 0; j < 3; j++) {
        out[j] += mMatrix[j][i] * in.at(i);
      }
    }
  }
};

} // namespace refinery

#endif /* _REFINERY_COLOR_H */
