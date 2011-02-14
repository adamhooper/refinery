#ifndef _REFINERY_GAMMA_H
#define _REFINERY_GAMMA_H

#include <cmath>
#include <limits>
#include <vector>

namespace refinery {

template<typename T>
class GammaCurve {
public:
  typedef std::vector<T> CurveType;

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
  GammaCurve(const GammaCurve& rhs) : mCurve(rhs.mCurve) {}
  GammaCurve(double pwr, double ts, T max)
  {
    this->init(pwr, ts, max);
  }

  template<typename HistogramType> GammaCurve(const HistogramType& histogram)
  {
    typedef typename HistogramType::ColorType ColorType;
    typedef typename HistogramType::ValueType ValueType;

    // Output image will be 1% white
    unsigned int perc = histogram.nPixels() * 0.01;
    unsigned int white = 0;

    for (ColorType c = 0; c <= 2; c++) {
      unsigned int total = 0;
      unsigned int val = 0x1fff;
      while (val > 32) {
        total += histogram.count(c, val);
        if (total > perc) break;
        val--;
      }
      if (white < val) white = val;
    }

    this->init(0.45, 4.5, white << 3);
  }

  inline const T& at(const T& in) const { return mCurve.at(in); }
};

} // namespace refinery

#endif /* _REFINERY_GAMMA_H */
