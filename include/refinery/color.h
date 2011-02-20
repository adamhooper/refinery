#ifndef _REFINERY_COLOR_H
#define _REFINERY_COLOR_H

#include <cstddef>

namespace refinery {

template<typename T> struct RGBColor;

/**
 * Converts from one color space to another.
 *
 * Example:
 *
 * \code
 * float conversionMatrix[3][4]; // e.g. CMYK to RGB
 * // ... fill in conversionMatrix ...
 * ColorConverter<float, 4, 3> converter(conversionMatrix);
 * CMYKPixel<float>* in;
 * RGBPixel<float>* out;
 *
 * for (in = firstPixel; in < lastPixel; in++, out++) {
 *   converter.convert(in->constArray(), out->array());
 * }
 * \endcode
 *
 * \tparam T Type of conversion matrix (usually float or double).
 * \tparam NFrom Arity of source arrays.
 * \tparam NTo Arity of destination arrays.
 */
template<typename T, std::size_t NFrom, std::size_t NTo>
class ColorConverter {
  T mMatrix[NTo][NFrom];

public:
  /**
   * Constructor.
   *
   * \tparam U Datatype of the conversion matrix (usually inferred).
   * \param[in] matrix The conversion matrix (will be copied).
   */
  template<typename U> ColorConverter(const U (&matrix)[NTo][NFrom])
  {
    for (unsigned int i = 0; i < NTo; i++) {
      for (unsigned int j = 0; j < NFrom; j++) {
        mMatrix[i][j] = static_cast<T>(matrix[i][j]);
      }
    }
  }

  /**
   * Convert an input vector to an output one.
   *
   * \tparam U Datatype of input vector (usually inferred).
   * \tparam V Datatype of output vector (usually inferred).
   * \param[in] in Vector with NFrom input colors.
   * \param[out] out Vector with NTo output colors.
   */
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
   * Convert an undersized input vector to an output one.
   *
   * We need this special case, because we sometimes treat all cameras as
   * having 4-color sensor arrays when they actually have 3, but at other times
   * we optimize away the final element. Usually users won't need to directly
   * specify this method.
   *
   * \tparam U Datatype of input vector (usually inferred).
   * \tparam V Datatype of output vector (usually inferred).
   * \param[in] in Vector with NFrom input colors.
   * \param[out] out Vector with NTo output colors.
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
};

} // namespace refinery

#endif /* _REFINERY_COLOR_H */
