#ifndef _REFINERY_HISTOGRAM_H
#define _REFINERY_HISTOGRAM_H

#include <cstddef>
#include <limits>

namespace refinery {

/*
 * A histogram which says, for each colour, how many pixels hold each value.
 *
 * The Coarseness parameter decides to what extent pixels will be "grouped"
 * into histogram slots. For example, a Coarseness of 3 on an unsigned short
 * value means the histogram will have 2^13 slots, rather than 2^16
 * (because 16 - 3 = 13). The first green slot in the histogram will sum the
 * number of pixels with green values from 0 to 7 (7 = 2^3 - 1). A
 * Coarseness of 0 makes for an exact (but sometimes slower or less handy)
 * histogram.
 */
template<typename ImageType, unsigned int Coarseness = 0>
class Histogram {
public:
  typedef typename ImageType::ValueType ValueType;
  typedef typename ImageType::PixelType PixelType;
  typedef typename ImageType::ColorType ColorType;
  static const ColorType NColors = PixelType::NColors;

private:
  typedef std::vector<unsigned int> CurveType;

  CurveType mCurves[NColors];
  unsigned int mNPixels;

  void init(const ImageType& image)
  {
    // XXX only works for unsigned integer types
    const std::size_t nSlots
      = (std::numeric_limits<ValueType>::max() >> Coarseness) + 1;

    for (ColorType c = 0; c < NColors; c++) {
      mCurves[c].assign(nSlots, 0);
    }

    const PixelType* pixel(image.constPixels());
    const PixelType* endPixel(image.constPixelsEnd());

    for (;pixel < endPixel; pixel++) {
      for (ColorType c = 0; c < NColors; c++) {
        mCurves[c][pixel->at(c) >> Coarseness]++;
      }
    }

    this->mNPixels = image.nPixels();
  }

public:
  Histogram(const ImageType& image)
  {
    this->init(image);
  }

  inline unsigned int nSlots() const { return this->mCurves[0].size(); }

  inline unsigned int nPixels() const { return this->mNPixels; }

  inline unsigned short count(const ColorType& color, std::size_t slot) const
  {
    return mCurves[color][slot];
  }
};

} // namespace refinery

#endif /* _REFINERY_HISTOGRAM_H */
