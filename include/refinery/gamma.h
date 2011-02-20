#ifndef _REFINERY_GAMMA_H
#define _REFINERY_GAMMA_H

#include <cmath>
#include <limits>
#include <vector>

namespace refinery {

/**
 * A gamma curve, mapping input scalars to output scalars.
 *
 * The GammaCurve is pre-calculated into a lookup table during construction,
 * after which any scalar can be converted using at().
 *
 * \tparam T Color value type, see Image::ValueType.
 * \see GammaFilter
 */
template<typename T>
class GammaCurve {
public:
  typedef std::vector<T> CurveType; /**< Color value type. */

private:
  CurveType mCurve;

  void init(double pwr, double ts, T max)
  {
    double g[6] = { 0, 0, 0, 0, 0, 0 };
    const T limit = std::numeric_limits<T>::max();
    const unsigned int curveSize = static_cast<unsigned int>(limit) + 1;
    g[0] = pwr;
    g[1] = ts;

    double bnd[2] = { 0, 1 };

    for (int i = 0; i < 48; i++) {
      g[2] = (bnd[0] + bnd[1]) / 2;

      double t = std::pow(g[2]/g[1],-g[0]) - 1;
      bnd[t/g[0] - 1/g[2] > -1] = g[2];
    }

    g[3] = g[2] / g[1];
    g[4] = g[2] * (1/g[0] - 1);
    g[5] = 1 / (
        g[1] * g[3] * g[3] / 2
        + 1 - g[2] - g[3]
        - g[2] * g[3] * (std::log(g[3]) - 1)
        ) - 1;

    mCurve.reserve(curveSize);

    for (unsigned int i = 0; i < curveSize; i++) {
      double r = static_cast<double>(i) / max;
      T val;
      if (r < 1.0) {
        val = curveSize * (
            r < g[3]
            ? r * g[1]
            : (std::pow(r, g[0]) * (1 + g[4]) - g[4])
            );
      } else {
        val = limit;
      }
      mCurve.push_back(val);
    }
  }

public:
  /**
   * Constructor.
   *
   * \param[in] pwr Gamma exponent, for instance 0.45.
   * \param[in] ts I don't know.
   * \param[in] max Lowest input value corresponding to max.
   */
  GammaCurve(double pwr, double ts, T max)
  {
    this->init(pwr, ts, max);
  }
  /**
   * Constructor.
   *
   * This uses the Histogram-supplied image data to decide on GammaCurve
   * parameters so that when the GammaCurve is applied (through GammaFilter)
   * to the original image, 1% of the output image will be white.
   *
   * The supplied \p histogram doesn't need to be around for the lifetime of
   * this object.
   *
   * \param[in] histogram Histogram to analyze to calculate curve.
   * \tparam HistogramType Type of histogram (usually inferred).
   */
  template<typename HistogramType> GammaCurve(const HistogramType& histogram)
  {
    typedef typename HistogramType::ColorType ColorType;
    typedef typename HistogramType::ValueType ValueType;

    // Output image will be 1% white
    unsigned int perc = histogram.nPixels() * 0.01;
    unsigned int white = 0;

    for (ColorType c = 0; c < HistogramType::NColors; c++) {
      unsigned int total = 0;
      unsigned int val = 0x1fff;
      while (val > 32) {
        total += histogram.count(c, val);
        if (total > perc) break;
        val--;
      }
      if (white < val) white = val;
    }

    this->init(0.45, 4.5, white << HistogramType::Coarseness);
  }
  /**
   * Copy constructor.
   *
   * \param rhs Other GammaCurve, to be deep-copied.
   */
  GammaCurve(const GammaCurve& rhs) : mCurve(rhs.mCurve) {}

  /**
   * Gamma-correct a color value.
   */
  inline const T& at(const T& in) const { return mCurve.at(in); }
};

} // namespace refinery

#endif /* _REFINERY_GAMMA_H */
